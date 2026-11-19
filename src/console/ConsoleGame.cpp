#include "ConsoleGame.hpp"

#include <cstdio>
#include <iostream>
#include <string>
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
            if (x == p.x && y == p.y) c = '@';
            std::putchar(c);
        }
        std::putchar('\n');
    }
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
    std::printf("\nTUMBAL — mode console\n"
                "Perintah: w/a/s/d (+angka langkah, mis. 'd 5'), q/e/z/c = diagonal,\n"
                "          graph, tungku, buka <a> <b>, lihat, keluar\n");
    printView(field);

    std::string line;
    while (std::printf("\n> "), std::getline(std::cin, line)) {
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
        int dx = 0, dy = 0;
        if (cmd == "w") dy = -1; else if (cmd == "s") dy = 1;
        else if (cmd == "a") dx = -1; else if (cmd == "d") dx = 1;
        else if (cmd == "q") { dx = -1; dy = -1; } else if (cmd == "e") { dx = 1; dy = -1; }
        else if (cmd == "z") { dx = -1; dy = 1; } else if (cmd == "c") { dx = 1; dy = 1; }
        else { std::printf("Perintah tidak dikenal.\n"); continue; }
        int steps = 1;
        std::sscanf(line.c_str() + cmd.size(), "%d", &steps);
        for (int i = 0; i < steps; ++i) {
            // simulasikan satu langkah penuh: tekan arah lalu jalankan waktu sampai tiba
            FieldEvent ev = field.update(0.0f, dx, dy, false);
            for (int k = 0; k < 60 && field.player().moving && ev.kind == FieldEvent::None; ++k)
                ev = field.update(1.0f / 30.0f, 0, 0, false);
            if (ev.kind == FieldEvent::Transfer) {
                field.enter(ev.travel.toMap, ev.travel.x, ev.travel.y, ev.travel.dir);
                std::printf("-- pindah ke %s --\n", world.info(field.mapId())->name.c_str());
                break;
            }
            if (ev.kind == FieldEvent::LockedDoor) { std::printf("Terkunci dari sisi lain.\n"); break; }
        }
        printView(field);
    }
    return 0;
}
