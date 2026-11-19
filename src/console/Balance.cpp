#include "Balance.hpp"

#include <cstdio>
#include "core/battle/Battle.hpp"
#include "core/battle/Encounters.hpp"
#include "core/run/Run.hpp"

namespace {

// ---------------------------------------------------------------------------
// Pemain heuristik: kira-kira seperti pemain yang paham dasar-dasarnya
// ---------------------------------------------------------------------------

int lowestHp(Battle& b, Side side, bool ratio) {
    int best = -1;
    float bv = 1e9f;
    for (int i = 0; i < b.unitCount(); ++i) {
        const Unit& u = b.unit(i);
        if (!u.alive || u.side != side) continue;
        float v = ratio ? static_cast<float>(u.hp) / u.maxHp : static_cast<float>(u.hp);
        if (v < bv) { bv = v; best = i; }
    }
    return best;
}

int countAlive(Battle& b, Side side) {
    int n = 0;
    for (int i = 0; i < b.unitCount(); ++i)
        if (b.unit(i).alive && b.unit(i).side == side) ++n;
    return n;
}

// Musuh yang paling dekat bertindak (node terdepan di Timeline)
int nextEnemy(Battle& b) {
    int found = -1;
    b.timeline().forEach([&](const Timeline<Unit>::Node& n) {
        if (found < 0 && n.item->side == Side::Enemy) found = b.indexOf(n.item);
    });
    return found;
}

int slotOf(const Unit& u, SkillId id) {
    for (int s = 0; s < u.skillCount; ++s)
        if (u.skills[s] == &getSkill(id)) return s;
    return -1;
}

void heroTurn(Battle& b) {
    // Interject bila ada hero lain dengan Resolve penuh
    for (int i = 0; i < b.unitCount(); ++i)
        if (i != b.currentIndex() && b.canInterject(i)) { b.interject(i); break; }

    int cur = b.currentIndex();
    const Unit& u = b.unit(cur);
    int weakEnemy = lowestHp(b, Side::Enemy, false);
    int weakAlly = lowestHp(b, Side::Hero, true);
    float weakRatio = weakAlly >= 0 ? static_cast<float>(b.unit(weakAlly).hp) / b.unit(weakAlly).maxHp : 1.0f;

    auto use = [&](int slot, int target) { return b.canUseSkill(slot) && b.playerUseSkill(slot, target); };

    if (u.empowered) {
        if (!use(ULT_SLOT, weakEnemy)) use(ULT_SLOT, -1);
        return;
    }
    switch (static_cast<HeroKind>(u.kind)) {
        case HeroKind::Ione:
            if (weakRatio < 0.45f && use(slotOf(u, SkillId::Mend), weakAlly)) return;
            break;
        case HeroKind::Varga:
            if (weakRatio < 0.4f && !u.hasStatus(StatusType::Taunt) && use(slotOf(u, SkillId::Bulwark), -1)) return;
            if (b.ember() >= 3 && use(slotOf(u, SkillId::ShieldBash), nextEnemy(b))) return;
            break;
        case HeroKind::Sable:
            if (countAlive(b, Side::Enemy) >= 2 && b.ember() >= 2 && use(slotOf(u, SkillId::CinderCurse), -1)) return;
            if (b.ember() >= 3 && use(slotOf(u, SkillId::Unravel), nextEnemy(b))) return;
            break;
        default:
            break;
    }
    use(0, weakEnemy);
}

// Mainkan battle sampai selesai. Kembalikan true jika menang.
bool autoplay(Battle& b) {
    BattleEvent e;
    for (int guard = 0; guard < 400 && b.state() == BattleState::WaitingForPlayer; ++guard) {
        heroTurn(b);
        while (b.pollEvent(e)) {}
    }
    while (b.pollEvent(e)) {}
    return b.state() == BattleState::Victory;
}

// ---------------------------------------------------------------------------

void encounterReport(int runs) {
    std::printf("\n== Encounter tunggal (party penuh, skala lantai) ==\n");
    std::printf("%-26s %6s %6s %8s %8s %8s\n", "Encounter", "Lantai", "Menang", "Giliran", "HP sisa", "Timeout");
    struct Case { EncounterId id; int floor; };
    const Case cases[] = {
        {EncounterId::Thralls, 1}, {EncounterId::BellChoir, 1}, {EncounterId::Candlelight, 1},
        {EncounterId::Gargoyle, 1}, {EncounterId::ChoirWarden, 1},
        {EncounterId::BellChoir, 2}, {EncounterId::Gargoyle, 2}, {EncounterId::SisterOfAsh, 2},
        {EncounterId::Candlelight, 3}, {EncounterId::Gargoyle, 3}, {EncounterId::LastFlame, 3},
    };
    for (const Case& c : cases) {
        int wins = 0, timeouts = 0;
        long turns = 0;
        double hpLeft = 0;
        for (int r = 0; r < runs; ++r) {
            Unit heroes[3];
            buildParty(heroes, 3);
            Unit enemies[MAX_UNITS];
            int n = buildEncounter(c.id, enemies, MAX_UNITS);
            float scale = 1.0f + 0.3f * (c.floor - 1);
            for (int i = 0; i < n; ++i) {
                enemies[i].maxHp = enemies[i].hp = static_cast<int>(enemies[i].maxHp * scale);
                enemies[i].atk = static_cast<int>(enemies[i].atk * scale);
            }
            Battle b(heroes, 3, enemies, n, 1000u + r * 7919u, 0, false);
            b.start();
            if (autoplay(b)) {
                ++wins;
                int hp = 0, mx = 0;
                for (int i = 0; i < 3; ++i) { hp += b.unit(i).hp; mx += b.unit(i).maxHp; }
                hpLeft += static_cast<double>(hp) / mx;
            }
            if (!b.isOver()) ++timeouts;
            turns += b.turnNumber();
        }
        std::printf("%-26s %6d %5d%% %8.1f %7.0f%% %7d%%\n", encounterName(c.id), c.floor, wins * 100 / runs,
                    static_cast<double>(turns) / runs, wins ? hpLeft * 100 / wins : 0.0, timeouts * 100 / runs);
    }
}

// Pilih langkah berikutnya di peta: utamakan shrine jika lemah, lalu maju ke kolom lebih jauh.
int chooseStep(Run& run) {
    Floor& f = run.floor();
    int hp = 0, mx = 0;
    for (int i = 0; i < PARTY_SIZE; ++i) { hp += run.hero(i).hp; mx += run.hero(i).maxHp; }
    bool weak = hp * 2 < mx;
    int best = -1, bestScore = -1000;
    f.graph().forEachNeighbor(f.currentRoom(), [&](Floor::Vertex* v, int) {
        const Room& r = v->data;
        int score = r.col * 10 + (r.cleared ? -25 : 0);
        if (r.revealed && r.type == RoomType::Shrine && !r.cleared) score += weak ? 60 : 15;
        if (r.revealed && (r.type == RoomType::Treasure || r.type == RoomType::Merchant)) score += 6;
        if (r.revealed && r.type == RoomType::Elite && weak) score -= 30;
        score += (v->id * 7) % 5;   // pemecah seri deterministik
        if (score > bestScore) { bestScore = score; best = v->id; }
    });
    return best;
}

void runReport(int runs) {
    std::printf("\n== Run penuh 3 lantai (pemain heuristik) ==\n");
    int won = 0, deathsTotal = 0, steps = 0;
    int reachedFloor[MAX_FLOORS + 1] = {};
    for (int r = 0; r < runs; ++r) {
        Run run(5000u + r * 104729u);
        int guard = 0;
        while (run.state() != RunState::RunWon && guard < 3000 && run.deaths() < 15) {
            ++guard;
            switch (run.state()) {
                case RunState::Exploring:
                    run.travelTo(chooseStep(run));
                    ++steps;
                    break;
                case RunState::InBattle:
                    autoplay(*run.battle());
                    run.finishBattle();
                    break;
                case RunState::AtShrine:
                    run.rest();
                    run.leaveRoom();
                    break;
                case RunState::InShop:
                    if (run.light() < 50) run.buy(ShopItem::LampOil);
                    run.buy(ShopItem::Elixir);
                    run.leaveRoom();
                    break;
                case RunState::InEvent:
                    run.chooseEventOption(run.currentEvent().title[0] == 'L' ? 0 : 1);
                    break;
                case RunState::ChoosingRelic:
                    run.chooseRelic(0);
                    break;
                case RunState::RunWon:
                    break;
            }
            BattleEvent e;
            while (run.pollEvent(e)) {}
        }
        reachedFloor[run.state() == RunState::RunWon ? MAX_FLOORS : run.floor().number() - 1]++;
        if (run.state() == RunState::RunWon) ++won;
        deathsTotal += run.deaths();
    }
    std::printf("Tamat: %d/%d (%d%%)   rata-rata kematian: %.1f   langkah/run: %.0f\n", won, runs, won * 100 / runs,
                static_cast<double>(deathsTotal) / runs, static_cast<double>(steps) / runs);
    std::printf("Gagal di lantai 1: %d, lantai 2: %d, lantai 3: %d (batas 15 kematian)\n", reachedFloor[0], reachedFloor[1],
                reachedFloor[2]);
}

}  // namespace

int runBalance(int runs) {
    if (runs < 1) runs = 200;
    std::printf("Simulasi balancing, %d percobaan per baris.\n", runs);
    encounterReport(runs);
    runReport(runs / 4 > 0 ? runs / 4 : 1);
    return 0;
}
