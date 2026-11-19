// ============================================================================
// Balance (--balance): simulasi battle otomatis untuk menyetel tingkat kesulitan.
//
// Untuk setiap kelas x troop x level, battle dijalankan berkali-kali (seed
// berbeda) dengan BattleSystem yang SAMA dengan game, memakai "pemain bot"
// sederhana: minum Ramuan Jamu saat HP < 35%, menunda musuh yang sedang
// merapal bila punya jurus tunda, memakai jurus serang terkuat yang terjangkau
// MP, menyerang inti boss lebih dulu, lalu musuh dengan HP terkecil. Timun Mas ikut sejak Laut Garam (seperti di cerita).
//
// Keluaran: level terendah dengan peluang menang >= 50% dan >= 90% per troop,
// ditambah kurva Buto Ijo dengan senjata +0 dan +3 (pandai besi).
// Alat pengembangan; tidak dipakai saat bermain.
// ============================================================================

#include "Balance.hpp"

#include <cstdio>
#include <cstring>
#include "core/battle/BattleSystem.hpp"
#include "core/data/GameData.hpp"

namespace {

constexpr int RUNS = 40;
constexpr int MAX_LEVEL = 16;

struct SimBag : ItemBag {
    int jamu = 3;
    int count(int id) const override { return id == IT_JAMU ? jamu : 0; }
    bool removeItem(int id, int n) override {
        if (id != IT_JAMU || jamu < n) return false;
        jamu -= n;
        return true;
    }
};

Battler hero(int cls, int level, int weapon) {
    Battler b = makeHeroBattler("Tumbal", cls, level);
    const WeaponDef& w = weaponDef(cls);
    b.stats.atk += w.atk[weapon];
    b.stats.mag += w.mag[weapon];
    return b;
}

// Pilih aksi "pemain bot" untuk battler party `me`.
BattleAction decide(const BattleSystem& bs, int me, const SimBag& bag) {
    const Battler& b = bs.battler(me);
    // 1) darurat: jamu
    if (b.hp * 100 < b.stats.mhp * 35 && bag.jamu > 0 && bs.canUseItem(me, IT_JAMU))
        return BattleAction{SKL_SERANG, bs.firstItemTarget(me, IT_JAMU), IT_JAMU};
    // 2) Timun Mas menyembuhkan kawan yang sekarat
    for (int p = 0; p < bs.partyCount(); ++p) {
        const Battler& k = bs.battler(p);
        if (k.alive() && k.hp * 100 < k.stats.mhp * 40 && bs.canUse(me, SKL_SEMBUH)) return BattleAction{SKL_SEMBUH, p, 0};
    }
    // 3) buyarkan rapal musuh dengan jurus tunda
    for (int i = bs.partyCount(); i < bs.count(); ++i) {
        const Battler& e = bs.battler(i);
        if (!e.alive() || !e.chanting) continue;
        for (int s = 0; s < b.skillCount; ++s) {
            const SkillDef* d = skillDef(b.skills[s]);
            if (d && d->delayPct > 0 && d->scope == SC_MUSUH_SATU && bs.canUse(me, d->id)) return BattleAction{d->id, i, 0};
        }
    }
    // 4) serang: inti boss dulu, lalu HP terkecil
    int target = -1;
    for (int i = bs.partyCount(); i < bs.count(); ++i) {
        const Battler& e = bs.battler(i);
        if (!e.alive()) continue;
        if (e.def && e.def->core) { target = i; break; }
        if (target < 0 || e.hp < bs.battler(target).hp) target = i;
    }
    // jurus serang terkuat yang terjangkau MP (tanpa rapal), selain itu serangan biasa
    int best = SKL_SERANG;
    float bestMul = 1.0f;
    for (int s = 0; s < b.skillCount; ++s) {
        const SkillDef* d = skillDef(b.skills[s]);
        if (d == nullptr || d->chant > 0 || d->formula == FM_PULIH || d->formula == FM_NONE) continue;
        if (d->scope != SC_MUSUH_SATU && d->scope != SC_MUSUH_SEMUA) continue;
        if (!bs.canUse(me, d->id)) continue;
        float m = d->mul * d->hits * (d->formula == FM_ILMU ? (b.stats.mag * 5.0f + d->base) / (b.stats.atk * 4.0f + 1)
                                                            : 1.0f);
        if (m > bestMul) { bestMul = m; best = d->id; }
    }
    return BattleAction{best, target, 0};
}

// true bila party menang.
bool simulate(int cls, int level, int weapon, int troop, bool withCompanion, unsigned seed) {
    Battler party[2] = {hero(cls, level, weapon), makeHeroBattler("Timun Mas", KL_TIMUN_MAS, level > 1 ? level - 1 : 1)};
    BattleSystem bs;
    SimBag bag;
    bs.setup(party, withCompanion ? 2 : 1, troop, INIT_NORMAL, seed);
    bs.setInventory(&bag);
    BattleEvent ev;
    for (int guard = 0; guard < 3000 && !bs.over(); ++guard) {
        while (bs.events().dequeue(ev)) {}
        if (bs.phase() == BattleSystem::Input) {
            int me = bs.inputActor();
            if (!bs.submit(decide(bs, me, bag))) bs.submit(BattleAction{SKL_BERTAHAN, -1, 0});
        } else {
            bs.advanceToNextActor();
        }
    }
    while (bs.events().dequeue(ev)) {}
    return bs.phase() == BattleSystem::Victory;
}

int winRate(int cls, int level, int weapon, int troop, bool companion) {
    int wins = 0;
    for (int r = 0; r < RUNS; ++r)
        if (simulate(cls, level, weapon, troop, companion, 1000u + 7919u * r + level * 31u + troop * 101u)) ++wins;
    return wins * 100 / RUNS;
}

}  // namespace

int runBalance() {
    const char* classes[] = {"Pendekar", "Maling", "Pawang"};
    std::printf("TUMBAL --balance: %d battle per sel, pemain bot (jamu, tunda rapal, fokus inti boss)\n", RUNS);
    std::printf("Kolom: level terendah dengan peluang menang >=50%% / >=90%% (\"-\" = tidak tercapai sampai L%d)\n\n",
                MAX_LEVEL);
    for (int cls = KL_PENDEKAR; cls <= KL_PAWANG; ++cls) {
        std::printf("== %s (senjata +0) ==\n", classes[cls]);
        for (int t = 1; t < TR_COUNT; ++t) {
            const TroopDef* td = troopDef(t);
            if (td == nullptr) continue;
            bool companion = t >= TR_HANTU_LAUT;   // Timun Mas bergabung di Laut Garam
            int l50 = 0, l90 = 0;
            for (int lv = 1; lv <= MAX_LEVEL && l90 == 0; ++lv) {
                int w = winRate(cls, lv, 0, t, companion);
                if (l50 == 0 && w >= 50) l50 = lv;
                if (w >= 90) l90 = lv;
            }
            char a[8], b[8];
            std::snprintf(a, sizeof a, l50 ? "L%d" : "-", l50);
            std::snprintf(b, sizeof b, l90 ? "L%d" : "-", l90);
            std::printf("  %-22s %s  %5s / %-5s\n", td->name, companion ? "+Timun" : "      ", a, b);
        }
        std::printf("\n");
    }
    std::printf("== Buto Ijo: peluang menang per level, senjata +0 vs +3 (dengan Timun Mas) ==\n");
    std::printf("  level     ");
    for (int lv = 6; lv <= 14; ++lv) std::printf("%5d", lv);
    std::printf("\n");
    for (int cls = KL_PENDEKAR; cls <= KL_PAWANG; ++cls) {
        for (int w = 0; w <= WEAPON_MAX; w += WEAPON_MAX) {
            std::printf("  %-8s +%d", classes[cls], w);
            for (int lv = 6; lv <= 14; ++lv) std::printf("%4d%%", winRate(cls, lv, w, TR_BUTO, true));
            std::printf("\n");
        }
    }
    return 0;
}
