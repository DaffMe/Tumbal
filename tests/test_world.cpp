// Unit test dunia: parser JSON, TileMap, Graph dunia (kunci & satu arah), Field.
#include <cstdio>
#include <string>
#include "TestFramework.hpp"
#include "../src/core/util/Json.hpp"
#include "../src/core/world/Field.hpp"

static void testJson() {
    SECTION("Json (pohon first-child / next-sibling)");
    std::string err;
    JsonValue* v = JsonValue::parse(R"({"a": 1, "b": [true, false, null], "s": "hi\né", "n": -2.5, "o": {"k": "v"}})", &err);
    CHECK(v != nullptr);
    if (v == nullptr) return;
    CHECK(v->getInt("a") == 1);
    CHECK(v->get("b")->size() == 3);
    CHECK(v->get("b")->at(0)->asBool() == true);
    CHECK(v->get("b")->at(2)->isNull());
    CHECK(v->getString("s") == "hi\n\xC3\xA9");
    CHECK(v->getNumber("n") == -2.5);
    CHECK(v->get("o")->getString("k") == "v");
    CHECK(v->get("tidak_ada") == nullptr);
    CHECK(v->getInt("tidak_ada", 7) == 7);
    delete v;

    CHECK(JsonValue::parse("{\"a\": }", &err) == nullptr);
    CHECK(!err.empty());
    CHECK(JsonValue::parse("[1, 2", &err) == nullptr);
}

static void testGraphLocks() {
    SECTION("Graph dunia: edge terkunci & satu arah");
    Graph<int> g;
    for (int i = 1; i <= 4; ++i) g.addVertex(i, i);
    CHECK(g.addEdge(1, 2, 1));
    CHECK(g.addEdge(2, 3, 1));
    CHECK(g.addEdge(3, 1, 1, false));       // jalan pintas terkunci
    CHECK(g.addArc(3, 4, 1));               // lompat turun: satu arah
    CHECK(g.isOpen(3, 4) && !g.isOpen(4, 3));
    CHECK(!g.isOpen(1, 3) && g.hasEdge(1, 3));

    // BFS hanya lewat edge terbuka: 1 -> 3 butuh 2 langkah lewat 2
    int depth3 = -1;
    g.bfs(1, -1, [&](Graph<int>::Vertex* v, int d) { if (v->id == 3) depth3 = d; });
    CHECK(depth3 == 2);
    // dari 4 tidak bisa ke mana-mana (satu arah)
    CHECK(g.bfs(4, -1, [](Graph<int>::Vertex*, int) {}) == 1);

    CHECK(g.setOpen(1, 3, true));           // buka jalan pintas
    g.bfs(1, -1, [&](Graph<int>::Vertex* v, int d) { if (v->id == 3) depth3 = d; });
    CHECK(depth3 == 1);
    CHECK(g.arcCount() == 7);
}

static const char* worldPath() {
    if (std::FILE* f = std::fopen("assets/maps/world.json", "rb")) { std::fclose(f); return "assets/maps/world.json"; }
    return "../assets/maps/world.json";
}

static void testWorldAndField() {
    SECTION("World + Field (peta Lumbung dari berkas Tiled)");
    World world;
    std::string err;
    bool ok = world.load(worldPath(), &err);
    if (!ok) std::printf("  (error: %s)\n", err.c_str());
    CHECK(ok);
    if (!ok) return;
    CHECK(world.graph().vertexCount() >= 2);
    CHECK(world.graph().isOpen(1, 2) && world.graph().isOpen(2, 1));
    CHECK(world.info(2)->hasTungku);

    TileMap* m = world.map(1);
    CHECK(m != nullptr && m->width() > 10 && m->tileW() == 32);
    CHECK(m->blocked(0, 0));                // pojok = tembok
    CHECK(m->blocked(-1, 5));               // luar peta

    Field f(world);
    CHECK(f.enterAtSpawn(world.startMap(), world.startSpawn()));
    const MapObject* sp = m->findObject("awal");
    CHECK(sp != nullptr);
    CHECK(!m->blocked(f.player().x, f.player().y));

    // jalan satu langkah ke bawah (lantai sel) sampai tiba
    int y0 = f.player().y;
    f.update(0.0f, 0, 1, false);
    CHECK(f.player().moving);
    for (int i = 0; i < 30 && f.player().moving; ++i) f.update(1.0f / 30.0f, 0, 0, false);
    CHECK(f.player().y == y0 + 1);

    // tembok menghalangi: dari spawn ke atas berulang tidak menembus tembok
    for (int s = 0; s < 10; ++s) {
        f.update(0.0f, 0, -1, false);
        for (int i = 0; i < 30 && f.player().moving; ++i) f.update(1.0f / 30.0f, 0, 0, false);
    }
    CHECK(!m->blocked(f.player().x, f.player().y));

    // pintu Sel -> Lorong: travel lewat Graph
    const MapObject* door = nullptr;
    for (const MapObject* o = m->objects(); o; o = o->next) if (o->type == "door") door = o;
    CHECK(door != nullptr);
    if (door) {
        TravelResult r = world.travel(1, *door);
        CHECK(r.ok && r.toMap == 2);
        CHECK(!world.map(2)->blocked(r.x, r.y));
        // kunci jalan lalu coba lagi
        world.graph().setOpen(1, 2, false);
        TravelResult r2 = world.travel(1, *door);
        CHECK(!r2.ok && r2.locked);
        CHECK(world.unlockShortcut(1, 2));
        CHECK(world.travel(1, *door).ok);
    }
    CHECK(world.nearestTungku(1) == 2);     // lorong sudah dikunjungi lewat travel
}

static void testSymbolEncounter() {
    SECTION("Musuh terlihat: arah kontak, kalah, istirahat di Tungku");
    World world;
    std::string err;
    if (!world.load(worldPath(), &err)) { CHECK(false); return; }
    Field f(world);
    CHECK(f.enter(1, 5, 5, FACE_DOWN));
    CHECK(f.enemyCount() == 1);
    const FieldEnemy& e = f.enemy(0);
    int ex = e.actor.x, ey = e.actor.y;
    CHECK(e.actor.facing == FACE_LEFT);

    // dari belakang (musuh menghadap kiri, pemain di kanannya melangkah ke kiri)
    CHECK(f.enter(1, ex + 1, ey, FACE_LEFT));
    FieldEvent ev = f.update(0.0f, -1, 0, false);
    CHECK(ev.kind == FieldEvent::Encounter);
    CHECK(ev.initiative == 1);
    CHECK(ev.troop == 1);

    // dari depan → normal
    CHECK(f.enter(1, ex - 1, ey, FACE_RIGHT));
    ev = f.update(0.0f, 1, 0, false);
    CHECK(ev.kind == FieldEvent::Encounter && ev.initiative == 0);

    // menang → musuh hilang dan tetap hilang walau keluar-masuk peta
    f.resolveEncounter(ev.enemyIndex, true);
    CHECK(f.enemyAt(ex, ey) < 0);
    CHECK(world.isDefeated(1, 0));
    CHECK(f.enter(2, 7, 13, FACE_DOWN));
    CHECK(f.enter(1, 5, 5, FACE_DOWN));
    CHECK(f.enemyCount() == 0);

    // istirahat di Tungku: traversal Graph menghidupkan semua musuh
    CHECK(world.restAtTungku() == 1);
    CHECK(!world.isDefeated(1, 0));
    f.respawnEnemies();
    CHECK(f.enemyCount() == 1);

    // musuh mendeteksi pemain dalam 3 tile lalu mengejar sampai kontak
    CHECK(f.enter(1, ex + 3, ey, FACE_RIGHT));
    f.respawnEnemies();
    bool met = false;
    for (int i = 0; i < 600 && !met; ++i) {
        ev = f.update(1.0f / 60.0f, 0, 0, false);
        if (ev.kind == FieldEvent::Encounter) met = true;
    }
    CHECK(met);
    CHECK(ev.initiative == 2);   // pemain membelakangi musuh → disergap
}

void runWorldTests() {
    testJson();
    testGraphLocks();
    testWorldAndField();
    testSymbolEncounter();
}
