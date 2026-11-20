#include "ConsoleGame.hpp"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include "core/battle/BattleSystem.hpp"
#include "core/game/Game.hpp"
#include "core/util/DsTrace.hpp"

namespace {

std::string findAssets() {
    const char* candidates[] = {"assets/", "../assets/"};
    for (const char* c : candidates) {
        std::string p = std::string(c) + "maps/world.json";
        if (std::FILE* f = std::fopen(p.c_str(), "rb")) {
            std::fclose(f);
            return c;
        }
    }
    return "assets/";
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

void printView(const Field& f, const Bloodstain* blood) {
    const TileMap* m = f.map();
    const Actor& p = f.player();
    std::printf("\n%s  (posisi %d,%d)\n", f.world().info(f.mapId())->name.c_str(), p.x, p.y);
    for (int y = p.y - 6; y <= p.y + 6; ++y) {
        for (int x = p.x - 14; x <= p.x + 14; ++x) {
            char c = m->blocked(x, y) ? '#' : '.';
            if (const MapObject* o = m->objectAt(x, y)) {
                if (o->type == "door") c = 'D';
                else if (o->type == "loot" && !f.world().isCollected(f.mapId(), m->typeIndex(o))) c = '*';
                else if (o->type == "tungku") c = 'T';
                else if (o->type == "npc") c = 'N';
                else if (o->type == "event" && o->props.getString("trigger", "action") == "action") c = '?';
            }
            if (f.enemyAt(x, y) >= 0) c = 'M';
            if (blood && blood->active && blood->map == f.mapId() && blood->x == x && blood->y == y) c = '~';
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

// Battle teks memakai BattleSystem yang disiapkan Game. Mengembalikan fase akhir.
BattleSystem::Phase consoleBattle(Game& game) {
    BattleSystem bs;
    game.setupBattle(bs, 12345u + static_cast<unsigned>(game.battleTroop()) * 77u);
    std::printf("\n=== BATTLE: %s ===\n", troopDef(game.battleTroop()) ? troopDef(game.battleTroop())->name : "?");
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
        std::printf("\n  b=Bertahan  j=Ramuan Jamu(%d)  k=Kabur   pilih [jurus target]: ", game.state().count(IT_JAMU));
        if (!std::getline(std::cin, line)) break;
        if (line == "k") {
            bs.tryEscape();
            printEvents(bs);
            continue;
        }
        BattleAction a;
        if (line == "b") {
            a.skill = SKL_BERTAHAN;
        } else if (line == "j") {
            a.item = IT_JAMU;
            a.target = who;
        } else {
            int k = 1, t = -1;
            std::sscanf(line.c_str(), "%d %d", &k, &t);
            if (k < 1 || k > me.skillCount) k = 1;
            a.skill = me.skills[k - 1];
            a.target = t;
        }
        if (a.item == 0 && !bs.validTarget(who, a.skill, a.target)) a.target = bs.firstTarget(who, a.skill);
        if (!bs.submit(a)) std::printf("  (aksi tidak valid)\n");
        printEvents(bs);
    }
    if (!bs.over()) bs.tryEscape();
    game.endBattle(bs);
    return bs.phase();
}

// ------------------------------------------------------------------ dialog

void consoleDialog(Game& game) {
    Interpreter& it = game.interpreter();
    std::string line;
    while (game.inDialog()) {
        if (it.state() == Interpreter::WaitMessage) {
            if (!it.speaker().empty()) std::printf("\n  [%s]", it.speaker().c_str());
            std::printf("\n  %s  (Enter)", it.message().c_str());
            if (!std::getline(std::cin, line)) { it.abort(); return; }
            it.resume();
        } else {
            for (int i = 0; i < it.optionCount(); ++i) std::printf("\n   %d) %s", i + 1, it.option(i).c_str());
            std::printf("\n  pilih: ");
            int k = 0;
            if (!std::getline(std::cin, line)) { it.abort(); return; }
            std::sscanf(line.c_str(), "%d", &k);
            it.choose(k - 1);
        }
    }
}

void flushNotices(Game& game) {
    std::string n;
    while (game.popNotice(n)) std::printf("  >> %s\n", n.c_str());
}

// Menu Tungku versi teks: naik level, warp (BFS Graph), pergi.
void consoleTungku(Game& game) {
    std::string line;
    while (game.tungkuMenuOpen()) {
        flushNotices(game);
        std::printf("\n  == TUNGKU ==  Level %d   Sukma %d\n  1) Naik level (butuh %d sukma)\n  2) Warp\n  3) Pergi\n",
                    game.state().level(), game.state().sukma(), game.state().levelUpCost());
        if (game.state().companionJoined())
            std::printf("  4) Timun Mas naik level (Lv %d, butuh %d sukma)\n  5) Panggil / lepas Timun Mas (%s)\n",
                        game.state().companionLevel(), game.state().companionLevelUpCost(),
                        game.state().companionActive() ? "ikut" : "menunggu");
        std::printf("  pilih: ");
        if (!std::getline(std::cin, line)) { game.closeTungkuMenu(); return; }
        if (line == "1") {
            std::printf("  %s\n", game.levelUp().c_str());
        } else if (line == "2") {
            int maps[16], depth[16];
            int n = game.warpTargets(maps, depth, 16);
            for (int i = 0; i < n; ++i)
                std::printf("   %d) %s  (%d jalan)\n", i + 1, game.world().info(maps[i])->name.c_str(), depth[i]);
            std::printf("  tujuan (0 = batal): ");
            int k = 0;
            if (!std::getline(std::cin, line)) return;
            std::sscanf(line.c_str(), "%d", &k);
            if (k >= 1 && k <= n && maps[k - 1] != game.field().mapId()) game.warpTo(maps[k - 1]);
        } else if (line == "4" && game.state().companionJoined()) {
            std::printf("  %s\n", game.companionLevelUp().c_str());
        } else if (line == "5" && game.state().companionJoined()) {
            std::printf("  %s\n", game.toggleCompanion().c_str());
        } else {
            game.closeTungkuMenu();
        }
    }
}

// Menu pedagang / pandai besi versi teks.
void consoleShop(Game& game) {
    std::string line;
    while (game.shopMenu() != Game::SHOP_NONE) {
        flushNotices(game);
        const GameState& gs = game.state();
        if (game.shopMenu() == Game::SHOP_DAGANG) {
            std::printf("\n  == PEDAGANG ==  Sukma %d\n", gs.sukma());
            for (int i = 0; i < shopCount(); ++i) {
                const ShopEntry& e = shopEntry(i);
                int st = gs.shopStock(i);
                std::string sisa = st < 0 ? "" : (st == 0 ? "  (habis)" : "  (sisa " + std::to_string(st) + ")");
                std::printf("  %d) %-20s %4d sukma%s\n", i + 1, itemDef(e.item)->name, e.price, sisa.c_str());
            }
            std::printf("  beli nomor (0 = pergi): ");
            int k = 0;
            if (!std::getline(std::cin, line)) { game.closeShopMenu(); return; }
            std::sscanf(line.c_str(), "%d", &k);
            if (k >= 1 && k <= shopCount()) std::printf("  %s\n", game.buy(k - 1).c_str());
            else game.closeShopMenu();
        } else {
            const WeaponDef& w = weaponDef(gs.classId());
            int lv = gs.weaponLevel();
            std::printf("\n  == PANDAI BESI ==  %s +%d   Sukma %d   Bijih Besi %d\n", w.name, lv, gs.sukma(),
                        gs.count(IT_BIJIH_BESI));
            if (lv < WEAPON_MAX)
                std::printf("  1) Tempa ke +%d (%d sukma, %d bijih)\n", lv + 1, w.costSukma[lv], w.costOre[lv]);
            std::printf("  0) Pergi\n  pilih: ");
            if (!std::getline(std::cin, line)) { game.closeShopMenu(); return; }
            if (line == "1") std::printf("  %s\n", game.forge().c_str());
            else game.closeShopMenu();
        }
    }
}

// Tuntaskan permintaan Game setelah setiap langkah (dialog, battle, pindah peta).
void settle(Game& game) {
    for (int guard = 0; guard < 50; ++guard) {
        flushNotices(game);
        if (game.tungkuMenuOpen()) {
            consoleTungku(game);
            continue;
        }
        if (game.shopMenu() != Game::SHOP_NONE) {
            consoleShop(game);
            continue;
        }
        if (game.inDialog()) {
            consoleDialog(game);
            continue;
        }
        if (game.battlePending()) {
            if (consoleBattle(game) == BattleSystem::Defeat) std::printf("\n  KAMU MATI. Sukmamu tercecer di tempat itu.\n");
            continue;
        }
        if (game.state().flag("tamat") != 0 && !game.interpreter().busy()) break;
        if (game.transferPending()) {
            game.finishTransfer();
            std::printf("-- %s --\n", game.world().info(game.field().mapId())->name.c_str());
            continue;
        }
        break;
    }
    flushNotices(game);
}

}  // namespace

int runConsole() {
    DsTrace::instance().setEcho(true);
    Game game;
    std::string err;
    std::string root = findAssets();
    std::string events[] = {root + "events/lumbung.json", root + "events/desa.json", root + "events/ladang.json",
                            root + "events/akhir.json"};
    if (!game.load(root + "maps/world.json", events, 4, &err)) {
        std::printf("Gagal memuat dunia: %s\n", err.c_str());
        return 1;
    }
    game.newGame("Tumbal", KL_PENDEKAR);
    std::printf("\nTUMBAL - mode console\n"
                "Perintah: w/a/s/d (+angka langkah, mis. 'd 5'), q/e/z/c = diagonal, x = periksa/bicara,\n"
                "          jamu, graph, tungku, buka <a> <b>, lihat, simpan <1-3>, muat <1-3>, cheat, keluar\n"
                "Peta: @ kamu, M musuh, N orang, ? periksa, * barang, T Tungku, D pintu, ~ sukma tercecer\n");
    settle(game);
    printView(game.field(), &game.state().bloodstain());

    std::string line;
    while (game.state().flag("tamat") == 0 &&
           std::printf("\n[HP %d/%d  Jamu %d  Sukma %d] > ", game.state().hero().hp, game.state().hero().stats.mhp,
                       game.state().count(IT_JAMU), game.state().sukma()),
           std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::string cmd = line.substr(0, line.find(' '));
        if (cmd == "keluar") break;
        if (cmd == "simpan" || cmd == "muat") {
            int slot = 1;
            std::sscanf(line.c_str() + cmd.size(), "%d", &slot);
            if (slot < 1 || slot > 3) slot = 1;
            std::filesystem::create_directories(root + "../saves");
            std::string path = root + "../saves/slot" + std::to_string(slot) + ".json";
            std::string e;
            bool ok = cmd == "simpan" ? game.saveTo(path, &e) : game.loadFrom(path, &e);
            if (ok) std::printf("  %s slot %d.\n", cmd == "simpan" ? "Tersimpan di" : "Dimuat dari", slot);
            else std::printf("  Gagal: %s\n", e.c_str());
            if (ok && cmd == "muat") printView(game.field(), &game.state().bloodstain());
            continue;
        }
        if (cmd == "cheat") {
            int id = -1, arg = 2;
            int got = std::sscanf(line.c_str() + cmd.size(), "%d %d", &id, &arg);
            if (got < 1 || id < 0 || id >= Game::CH_COUNT) {
                std::printf("  cheat <nomor> [stat]   (stat: 0 HP, 1 MP, 2 Serang, 3 Tahan, 4 Ilmu, 5 Tolak, 6 Gesit, 7 Untung)\n");
                for (int i = 0; i < Game::CH_COUNT; ++i) std::printf("   %d) %s\n", i, Game::cheatName(i));
            } else {
                std::printf("  %s\n", game.cheat(id, arg).c_str());
            }
            continue;
        }
        if (cmd == "graph") { printGraph(game.world(), game.field().mapId()); continue; }
        if (cmd == "lihat") { printView(game.field(), &game.state().bloodstain()); continue; }
        if (cmd == "tungku") {
            int t = game.world().nearestTungku(game.field().mapId());
            std::printf("Tungku terdekat yang sudah dikunjungi: %s\n", t ? game.world().info(t)->name.c_str() : "(belum ada)");
            continue;
        }
        if (cmd == "buka") {
            int a = 0, b = 0;
            if (std::sscanf(line.c_str(), "buka %d %d", &a, &b) == 2) game.world().unlockShortcut(a, b);
            continue;
        }
        if (cmd == "jamu") {
            std::string r = game.useItem(IT_JAMU);
            std::printf("  %s\n", r.empty() ? "Tidak ada gunanya sekarang." : r.c_str());
            continue;
        }
        if (cmd == "x") {
            GameInput in;
            in.action = true;
            game.update(0.0f, in);
            settle(game);
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
            // satu langkah penuh: tekan arah, lalu jalankan waktu sampai tiba
            GameInput in;
            in.dx = dx;
            in.dy = dy;
            game.update(0.0f, in);
            for (int k = 0; k < 60 && game.field().player().moving; ++k) game.update(1.0f / 30.0f, GameInput{});
            if (game.battlePending() || game.transferPending() || game.inDialog()) {
                settle(game);
                break;
            }
            flushNotices(game);
        }
        printView(game.field(), &game.state().bloodstain());
    }
    if (game.state().flag("tamat") != 0) {
        std::printf("\n\n        T A M A T\n\n  Buto Ijo runtuh. Desa Karangtumbal tidak lagi memberi tumbal.\n"
                    "  Kematian: %d   Level: %d\n\n", game.state().deaths(), game.state().level());
    }
    return 0;
}
