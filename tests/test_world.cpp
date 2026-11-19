// Unit test dunia: parser JSON, TileMap, Graph dunia (kunci & satu arah), Field.
#include <cstdio>
#include <string>
#include "TestFramework.hpp"
#include "../src/core/util/Json.hpp"
#include "../src/core/game/Game.hpp"
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

static void testTungkuWarp() {
    SECTION("M2: Tungku menyala & warp mengikuti Graph (BFS)");
    World world;
    std::string err;
    if (!world.load(worldPath(), &err)) { CHECK(false); return; }
    int maps[8], depth[8];
    CHECK(world.warpTargets(1, maps, depth, 8) == 0);       // belum ada Tungku menyala
    world.lightTungku(2);
    world.lightTungku(1);                                   // peta tanpa Tungku: diabaikan
    CHECK(!world.info(1)->tungkuLit && world.info(2)->tungkuLit);
    int n = world.warpTargets(1, maps, depth, 8);
    CHECK(n == 1 && maps[0] == 2 && depth[0] == 1);
    // jalan terkunci memutus warp: Tungku di wilayah yang tak terjangkau hilang
    world.graph().setOpen(1, 2, false);
    CHECK(world.warpTargets(1, maps, depth, 8) == 0);
    CHECK(world.warpTargets(2, maps, depth, 8) == 1 && depth[0] == 0);
}

static void testSaveLoad() {
    SECTION("Simpan & muat: GameState + status Graph dunia");
    Game a;
    std::string err;
    std::string root = std::string(worldPath()).substr(0, std::string(worldPath()).find("maps/world.json"));
    std::string ev[] = {root + "events/lumbung.json"};
    CHECK(a.load(worldPath(), ev, 1, &err));
    a.newGame("Tumbal", KL_MALING);
    a.giveStartingGift(2);
    a.state().addSukma(77);
    a.state().setFlag("uji", 3);
    a.state().hero().hp = 99;
    a.state().joinCompanion();
    a.world().lightTungku(2);
    a.world().markDefeated(1, 0);
    a.world().graph().setOpen(1, 2, false);
    a.state().die(1, 6, 6);                 // sukma tercecer ikut tersimpan
    std::string path = "uji_simpan.json";
    CHECK(a.saveTo(path, &err));
    CHECK(!Game::describeSave(path).empty());

    Game b;
    CHECK(b.load(worldPath(), ev, 1, &err));
    b.newGame("Lain", KL_PENDEKAR);
    CHECK(b.loadFrom(path, &err));
    CHECK(b.state().classId() == KL_MALING);
    CHECK(b.state().count(IT_BOM_MINYAK) == 3);
    CHECK(b.state().flag("uji") == 3);
    CHECK(b.state().bloodstain().active && b.state().bloodstain().sukma == 177);
    CHECK(b.state().companionJoined());
    CHECK(b.world().info(2)->tungkuLit);
    CHECK(b.world().isDefeated(1, 0));
    CHECK(!b.world().graph().isOpen(1, 2) && !b.world().graph().isOpen(2, 1));   // setOpen menutup dua arah
    CHECK(b.field().mapId() == a.field().mapId() && b.field().player().x == a.field().player().x);
    Battler party[4];
    CHECK(b.state().party(party) == 2);     // pemain + Timun Mas
    std::remove(path.c_str());
    CHECK(!b.loadFrom("tidak_ada.json", &err));
}

static void testBossGate() {
    SECTION("M2: gerbang boss - ladang ke hutan bambu terkunci sampai Memedi tumbang");
    Game g;
    std::string err;
    std::string root = std::string(worldPath()).substr(0, std::string(worldPath()).find("maps/world.json"));
    std::string ev[] = {root + "events/ladang.json"};
    CHECK(g.load(worldPath(), ev, 1, &err));
    g.newGame("Tumbal", KL_PENDEKAR);
    CHECK(!g.world().graph().isOpen(4, 5) && !g.world().graph().isOpen(5, 4));
    // jalan ke Sarang Buto hanya satu arah (Rawa -> Sarang)
    CHECK(g.world().graph().isOpen(7, 8) && !g.world().graph().isOpen(8, 7));
    const TileMap* ladang = g.world().map(4);
    CHECK(ladang != nullptr);
    const MapObject* door = ladang ? ladang->objectAt(ladang->width() - 1, 9, "door") : nullptr;
    CHECK(door != nullptr && !door->props.getString("pesan", "").empty());
    if (door) CHECK(g.world().travel(4, *door).locked);
    CHECK(g.world().unlockShortcut(4, 5));    // perintah event "unlock" setelah Memedi kalah
    if (door) CHECK(g.world().travel(4, *door).ok);
}

static void testWideEventAndNpc() {
    SECTION("Periksa objek event selebar 3 tile + NPC baru Rawa & Sarang");
    Game g;
    std::string err;
    std::string root = std::string(worldPath()).substr(0, std::string(worldPath()).find("maps/world.json"));
    std::string ev[] = {root + "events/akhir.json"};
    CHECK(g.load(worldPath(), ev, 1, &err));
    g.newGame("Tumbal", KL_PENDEKAR);
    g.state().setFlag("laut_masuk", 1);
    // pintu rumah nelayan: event lebar 3 tile di (10..12, 16); diperiksa dari ubin tengah bawahnya
    CHECK(g.field().enter(6, 11, 17, FACE_UP));
    GameInput in;
    in.action = true;
    g.update(0.016f, in);
    CHECK(g.interpreter().busy());
    // selesaikan dialog: sukma diberikan sekali saja
    for (int i = 0; i < 20 && g.interpreter().busy(); ++i) { g.interpreter().resume(); g.update(0.016f, GameInput{}); }
    CHECK(g.state().flag("rumah_nelayan") == 1);
    int sedang = g.state().count(itemIdByKey("sukma_sedang"));
    CHECK(sedang == 1);
    g.update(0.016f, in);
    for (int i = 0; i < 20 && g.interpreter().busy(); ++i) { g.interpreter().resume(); g.update(0.016f, GameInput{}); }
    CHECK(g.state().count(itemIdByKey("sukma_sedang")) == sedang);
    // ubin di luar lebar event tidak memicu apa pun
    CHECK(g.field().enter(6, 14, 17, FACE_UP));
    g.update(0.016f, in);
    CHECK(!g.interpreter().busy());
    // NPC rawa memberi jamu sekali
    g.state().setFlag("rawa_masuk", 1);
    CHECK(g.field().enter(7, 15, 16, FACE_UP));
    int jamu = g.state().count(IT_JAMU);
    g.update(0.016f, in);
    CHECK(g.interpreter().busy());
    for (int i = 0; i < 30 && g.interpreter().busy(); ++i) {
        if (g.interpreter().state() == Interpreter::WaitChoice) g.interpreter().choose(1); else g.interpreter().resume();
        g.update(0.016f, GameInput{});
    }
    CHECK(g.state().count(IT_JAMU) == jamu + 1 && g.state().flag("pemuda_beri") == 1);
}

static void testForgeAndShop() {
    SECTION("Pandai besi (tempa +0..+3) & pedagang (stok terbatas), ikut tersimpan");
    GameState gs;
    gs.newGame("Tumbal", KL_PENDEKAR);
    int atk0 = gs.hero().stats.atk;
    CHECK(gs.weaponLevel() == 0);
    CHECK(gs.canForge() == GameState::FORGE_NO_SUKMA);
    gs.addSukma(1000);
    CHECK(gs.canForge() == GameState::FORGE_NO_ORE);
    gs.addItem(IT_BIJIH_BESI, 6);
    CHECK(gs.forge() == GameState::FORGE_OK);
    CHECK(gs.weaponLevel() == 1 && gs.hero().stats.atk == atk0 + weaponDef(KL_PENDEKAR).atk[1]);
    CHECK(gs.sukma() == 1000 - 120 && gs.count(IT_BIJIH_BESI) == 5);
    CHECK(gs.forge() == GameState::FORGE_OK);
    CHECK(gs.forge() == GameState::FORGE_OK);                 // +3: total 830 sukma, 6 bijih
    CHECK(gs.weaponLevel() == 3 && gs.count(IT_BIJIH_BESI) == 0 && gs.sukma() == 170);
    CHECK(gs.forge() == GameState::FORGE_MAX);
    // bonus senjata bertahan setelah naik level
    int atk3 = gs.hero().stats.atk;
    gs.addSukma(500);
    CHECK(gs.levelUp());
    CHECK(gs.hero().stats.atk == statsAtLevel(KL_PENDEKAR, 2).atk + weaponDef(KL_PENDEKAR).atk[3]);
    CHECK(gs.hero().stats.atk > atk3);

    // pedagang: Kendi Jamu stok 2 menambah wadah jamu
    int kendi = -1, bijih = -1;
    for (int i = 0; i < shopCount(); ++i) {
        if (shopEntry(i).item == IT_KENDI_JAMU) kendi = i;
        if (shopEntry(i).item == IT_BIJIH_BESI) bijih = i;
    }
    CHECK(kendi >= 0 && bijih >= 0);
    GameState g2;
    g2.newGame("Tumbal", KL_PAWANG);
    CHECK(g2.buy(kendi) == GameState::BUY_NO_SUKMA);
    g2.addSukma(5000);
    int max0 = g2.jamuMax();
    CHECK(g2.buy(kendi) == GameState::BUY_OK && g2.buy(kendi) == GameState::BUY_OK);
    CHECK(g2.buy(kendi) == GameState::BUY_SOLD_OUT);
    CHECK(g2.jamuMax() == max0 + 2 && g2.count(IT_JAMU) == max0 + 2);
    CHECK(g2.shopStock(bijih) == shopEntry(bijih).stock);
    CHECK(g2.buy(bijih) == GameState::BUY_OK && g2.shopStock(bijih) == shopEntry(bijih).stock - 1);
    CHECK(g2.forge() == GameState::FORGE_OK);
    int mag1 = g2.hero().stats.mag;
    // simpan/muat: tingkat tempa, stok yang sudah dibeli, dan wadah jamu kembali
    std::string err;
    JsonValue* v = JsonValue::parse(g2.toJson(), &err);
    CHECK(v != nullptr);
    GameState g3;
    CHECK(g3.fromJson(v));
    delete v;
    CHECK(g3.weaponLevel() == 1 && g3.hero().stats.mag == mag1);
    CHECK(g3.shopStock(kendi) == 0 && g3.shopStock(bijih) == shopEntry(bijih).stock - 1);
    CHECK(g3.jamuMax() == max0 + 2);
}

static void testShopEvent() {
    SECTION("Perintah event \"shop\" membuka menu pedagang / pandai besi");
    Game g;
    std::string err;
    std::string root = std::string(worldPath()).substr(0, std::string(worldPath()).find("maps/world.json"));
    std::string ev[] = {root + "events/desa.json"};
    CHECK(g.load(worldPath(), ev, 1, &err));
    g.newGame("Tumbal", KL_PENDEKAR);
    CHECK(g.interpreter().runEvent("pandai_besi"));
    for (int i = 0; i < 30 && g.interpreter().busy(); ++i) {
        if (g.interpreter().state() == Interpreter::WaitChoice) g.interpreter().choose(0); else g.interpreter().resume();
        g.update(0.016f, GameInput{});
    }
    CHECK(g.shopMenu() == Game::SHOP_TEMPA);
    CHECK(g.forge().find("Sukma") != std::string::npos);    // belum punya sukma
    g.closeShopMenu();
    CHECK(g.interpreter().runEvent("pedagang"));
    for (int i = 0; i < 30 && g.interpreter().busy(); ++i) {
        if (g.interpreter().state() == Interpreter::WaitChoice) g.interpreter().choose(0); else g.interpreter().resume();
        g.update(0.016f, GameInput{});
    }
    CHECK(g.shopMenu() == Game::SHOP_DAGANG);
    g.state().addSukma(30);
    CHECK(g.buy(0).find("Dibeli") != std::string::npos);
}

void runWorldTests() {
    testForgeAndShop();
    testShopEvent();
    testWideEventAndNpc();
    testBossGate();
    testSaveLoad();
    testTungkuWarp();
    testJson();
    testGraphLocks();
    testWorldAndField();
    testSymbolEncounter();
}
