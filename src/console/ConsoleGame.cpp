#include "ConsoleGame.hpp"

#include <cstdio>
#include <iostream>
#include <string>
#include "core/battle/BattleSystem.hpp"
#include "core/util/DsTrace.hpp"
#include "core/world/Field.hpp"

namespace {

std::string findWorld() {
    const char* candidates[] = {"assets/maps/world.json", "../assets/maps/world.json"};
    for (const char* c : candidates) {
        if (std::FILE* f = std::fopen(c, "rb")) {
            std::fclose(f);
            return c;
        }
    }
    return "assets/maps/world.json";
}

void printGraph(World& world, int current) {
    std::printf("\n== Graph dunia (adjacency multi linked list) ==\n");
    world.graph().forEachVertex([&](World::MapGraph::Vertex* v) {
        std::printf("%s[%d] %-16s %s%s\n", v->id == current ? ">" : " ", v->id, v->data.name.c_str(),
                    v->data.visited ? "dikunjungi" : "belum", v->data.hasTungku ? ", ada Tungku" : "");
        for (auto* e = v->edges; e != nullptr; e = e->next)
            std::printf("      -> [%d] %s\n", e->to->id, e->open ? "terbuka" : "TERKUNCI");
    });
}

void printView(const Field& f) {
    const TileMap* m = f.map();
    const Actor& p = f.player();
    std::printf("\n%s  (posisi %d,%d)\n", f.world().info(f.mapId())->name.c_str(), p.x, p.y);
    for (int y = p.y - 6; y <= p.y + 6; ++y) {
        for (int x = p.x - 14; x <= p.x + 14; ++x) {
            char c = m->blocked(x, y) ? '#' : '.';
            if (const MapObject* o = m->objectAt(x, y)) {
                if (o->type == "door") c = 'D';
                else if (o->type == "loot") c = '*';
                else if (o->type == "tungku") c = 'T';
            }
            if (f.enemyAt(x, y) >= 0) c = 'M';
            if (x == p.x && y == p.y) c = '@';
            std::putchar(c);
        }
        std::putchar('\n');
    }
}

// ------------------------------------------------------------------ battle

void printEvents(BattleSystem& bs) {
    BattleEvent e;
    while (bs.events().dequeue(e)) {
        const char* dn = (e.dst >= 0) ? bs.battler(e.dst).name.c_str() : "";
        if (e.kind == BattleEvent::TurnStart) continue;
        if (e.kind == BattleEvent::Damage) {
            if (!e.text.empty()) std::printf("  %s (-%d)\n", e.text.c_str(), e.value);
            else std::printf("  %s menerima %d damage%s\n", dn, e.value, e.critical ? " (KRITIS!)" : "");
        } else if (e.kind == BattleEvent::Heal) {
            std::printf("  %s pulih %d HP\n", dn, e.value);
        } else if (e.kind == BattleEvent::StatusRemove) {
            std::printf("  %s: %s hilang\n", dn, statusDef(e.status).name);
        } else if (!e.text.empty()) {
            std::printf("  %s\n", e.text.c_str());
        }
    }
}

void printBattle(BattleSystem& bs) {
    std::printf("\n");
    for (int i = 0; i < bs.count(); ++i) {
        const Battler& b = bs.battler(i);
        if (!b.alive()) {
            std::printf("  [%d] %-18s (tumbang)\n", i, b.name.c_str());
            continue;
        }
        int bar = static_cast<int>(bs.gauge(i) * 10);
        std::printf("  [%d] %-18s HP %4d/%-4d MP %3d  %s[%-10.*s]", i, b.name.c_str(), b.hp, b.stats.mhp, b.mp,
                    b.chanting ? "RAPAL" : "AP   ", bar, "##########");
        for (int s = 1; s < ST_COUNT; ++s)
            if (b.status[s] > 0) std::printf(" %s", statusDef(s).shortName);
        std::printf("\n");
    }
    int order[8];
    int n = bs.predictTurns(order, 8);
    std::printf("  Antrean giliran (Timeline DLL):");
    for (int i = 0; i < n; ++i) std::printf(" %s%s", i ? "-> " : "", bs.battler(order[i]).name.c_str());
    std::printf("\n");
}

// Battle teks. Mengembalikan fase akhir; HP/MP pemain disalin balik ke `hero`.
BattleSystem::Phase consoleBattle(Battler& hero, int troopId, Initiative init, int* sukmaOut) {
    BattleSystem bs;
    bs.setup(&hero, 1, troopId, init, 12345u + static_cast<unsigned>(troopId) * 77u);
    std::printf("\n=== BATTLE: %s ===\n", troopDef(troopId) ? troopDef(troopId)->name : "?");
    printEvents(bs);
    std::string line;
    while (!bs.over()) {
        int who = bs.advanceToNextActor();
        printEvents(bs);
        if (bs.over() || who < 0 || bs.phase() != BattleSystem::Input) continue;
        Battler& me = bs.battler(who);
        printBattle(bs);
        std::printf("Giliran %s. Jurus:", me.name.c_str());
        for (int i = 0; i < me.skillCount; ++i) {
            const SkillDef* s = skillDef(me.skills[i]);
            std::printf("  %d=%s(%dMP)%s", i + 1, s->name, s->mpCost, bs.canUse(who, s->id) ? "" : "x");
        }
        std::printf("  b=Bertahan k=Kabur\n  pilih [jurus target]: ");
        if (!std::getline(std::cin, line)) return BattleSystem::Escaped;
        if (line == "k") {
            bs.tryEscape();
            printEvents(bs);
            continue;
        }
        BattleAction a;
        if (line == "b") {
            a.skill = SKL_BERTAHAN;
        } else {
            int k = 1, t = -1;
            std::sscanf(line.c_str(), "%d %d", &k, &t);
            if (k < 1 || k > me.skillCount) k = 1;
            a.skill = me.skills[k - 1];
            a.target = t;
        }
        if (!bs.validTarget(who, a.skill, a.target)) a.target = bs.firstTarget(who, a.skill);
        if (!bs.submit(a)) std::printf("  (aksi tidak valid)\n");
        printEvents(bs);
    }
    hero.hp = bs.battler(0).hp;
    hero.mp = bs.battler(0).mp;
    for (int s = 1; s < ST_COUNT; ++s) hero.status[s] = statusDef(s).battleOnly ? 0 : bs.battler(0).status[s];
    if (sukmaOut) *sukmaOut = bs.sukmaReward();
    return bs.phase();
}

}  // namespace

int runConsole() {
    DsTrace::instance().setEcho(true);
    World world;
    std::string err;
    if (!world.load(findWorld(), &err)) {
        std::printf("Gagal memuat dunia: %s\n", err.c_str());
        return 1;
    }
    Field field(world);
    field.enterAtSpawn(world.startMap(), world.startSpawn());
    Battler hero = makeHeroBattler("Tumbal", KL_PENDEKAR, 1);
    int sukma = 0;
    std::printf("\nTUMBAL - mode console\n"
                "Perintah: w/a/s/d (+angka langkah, mis. 'd 5'), q/e/z/c = diagonal,\n"
                "          graph, tungku, buka <a> <b>, lihat, battle <troop 1-5>, keluar\n"
                "Peta: @ kamu, M musuh, * barang, T Tungku, D pintu\n");
    printView(field);

    std::string line;
    while (std::printf("\n[HP %d/%d  Sukma %d] > ", hero.hp, hero.stats.mhp, sukma), std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::string cmd = line.substr(0, line.find(' '));
        if (cmd == "keluar" || cmd == "x") break;
        if (cmd == "graph") { printGraph(world, field.mapId()); continue; }
        if (cmd == "lihat") { printView(field); continue; }
        if (cmd == "tungku") {
            int t = world.nearestTungku(field.mapId());
            std::printf("Tungku terdekat yang sudah dikunjungi: %s\n", t ? world.info(t)->name.c_str() : "(belum ada)");
            continue;
        }
        if (cmd == "buka") {
            int a = 0, b = 0;
            if (std::sscanf(line.c_str(), "buka %d %d", &a, &b) == 2) world.unlockShortcut(a, b);
            continue;
        }
        if (cmd == "battle") {
            int t = 1;
            std::sscanf(line.c_str(), "battle %d", &t);
            int got = 0;
            BattleSystem::Phase r = consoleBattle(hero, t, INIT_NORMAL, &got);
            if (r == BattleSystem::Victory) sukma += got;
            if (r == BattleSystem::Defeat) { hero.hp = hero.stats.mhp; std::printf("(kamu bangkit kembali)\n"); }
            continue;
        }
        int dx = 0, dy = 0;
        if (cmd == "w") dy = -1; else if (cmd == "s") dy = 1;
        else if (cmd == "a") dx = -1; else if (cmd == "d") dx = 1;
        else if (cmd == "q") { dx = -1; dy = -1; } else if (cmd == "e") { dx = 1; dy = -1; }
        else if (cmd == "z") { dx = -1; dy = 1; } else if (cmd == "c") { dx = 1; dy = 1; }
        else { std::printf("Perintah tidak dikenal.\n"); continue; }
        int steps = 1;
        std::sscanf(line.c_str() + cmd.size(), "%d", &steps);
        for (int i = 0; i < steps; ++i) {
            // satu langkah penuh: tekan arah lalu jalankan waktu sampai tiba
            FieldEvent ev = field.update(0.0f, dx, dy, false);
            for (int k = 0; k < 60 && field.player().moving && ev.kind == FieldEvent::None; ++k)
                ev = field.update(1.0f / 30.0f, 0, 0, false);
            if (ev.kind == FieldEvent::Transfer) {
                field.enter(ev.travel.toMap, ev.travel.x, ev.travel.y, ev.travel.dir);
                std::printf("-- pindah ke %s --\n", world.info(field.mapId())->name.c_str());
                break;
            }
            if (ev.kind == FieldEvent::LockedDoor) { std::printf("Terkunci dari sisi lain.\n"); break; }
            if (ev.kind == FieldEvent::Encounter) {
                static const char* initName[] = {"", " (kamu menyerang dari belakang)", " (kamu disergap!)"};
                std::printf("-- bertemu musuh%s --\n", initName[ev.initiative]);
                int got = 0;
                BattleSystem::Phase r = consoleBattle(hero, ev.troop, static_cast<Initiative>(ev.initiative), &got);
                field.resolveEncounter(ev.enemyIndex, r == BattleSystem::Victory);
                if (r == BattleSystem::Victory) sukma += got;
                if (r == BattleSystem::Defeat) { hero.hp = hero.stats.mhp; std::printf("(kamu bangkit kembali)\n"); }
                break;
            }
        }
        printView(field);
    }
    return 0;
}
