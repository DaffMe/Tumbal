// ============================================================================
// Unit test mekanik M2 (Floor/Graph) dan run loop.
// ============================================================================

#include "TestFramework.hpp"
#include "../src/core/dungeon/Floor.hpp"
#include "../src/core/run/Run.hpp"

namespace {

int countType(Floor& f, RoomType t) {
    int n = 0;
    f.graph().forEachVertex([&](Floor::Vertex* v) { if (v->data.type == t) ++n; });
    return n;
}

// Selesaikan battle aktif: menang (musuh HP 1) atau kalah (hero HP 1, musuh tebal)
void forceBattle(Run& run, bool win) {
    Battle* b = run.battle();
    for (int guard = 0; guard < 500 && b->state() == BattleState::WaitingForPlayer; ++guard) {
        int target = -1;
        for (int i = 0; i < b->unitCount(); ++i) {
            Unit& u = b->unit(i);
            if (u.side == Side::Enemy && u.alive) {
                if (win) u.hp = 1; else { u.hp = u.maxHp = 99999; u.atk = 999; }
                if (target < 0) target = i;
            }
            if (!win && u.side == Side::Hero && u.alive) u.hp = 1;
        }
        b->playerUseSkill(0, target);
    }
    run.finishBattle();
}

// Ruang bertetangga pertama yang belum selesai dengan jenis tertentu (-1 jika tidak ada)
int adjacentOfType(Run& run, RoomType t) {
    int found = -1;
    run.floor().graph().forEachNeighbor(run.floor().currentRoom(), [&](Floor::Vertex* v, int) {
        if (found < 0 && v->data.type == t && !v->data.cleared) found = v->id;
    });
    return found;
}

// -------------------------------------------------------------------------
void testGenerator() {
    SECTION("M2: generator lantai valid untuk banyak seed");
    bool allOk = true;
    for (std::uint32_t seed = 1; seed <= 200; ++seed) {
        Floor f;
        f.generateFloor(seed, 1 + seed % 3);
        bool ok = f.graph().isConnected();
        ok = ok && countType(f, RoomType::Entrance) == 1 && countType(f, RoomType::Boss) == 1;
        ok = ok && countType(f, RoomType::Shrine) == 2 && countType(f, RoomType::Merchant) == 1;
        ok = ok && f.graph().vertexCount() >= 12 && f.graph().vertexCount() <= 22;
        ok = ok && f.room(f.entrance())->col == 0 && f.room(f.boss())->col == FLOOR_COLS - 1;
        // setiap ruang bisa dicapai dari pintu masuk dengan jalur berbobot
        Stack<int> path;
        ok = ok && f.graph().dijkstra(f.entrance(), f.boss(), path) > 0;
        if (!ok) { allOk = false; std::printf("  seed %u tidak valid\n", seed); break; }
    }
    CHECK(allOk);

    // setiap lantai punya boss sendiri
    const EncounterId bosses[3] = {EncounterId::ChoirWarden, EncounterId::SisterOfAsh, EncounterId::LastFlame};
    for (int n = 1; n <= 3; ++n) {
        Floor f;
        f.generateFloor(5, n);
        CHECK(f.room(f.boss())->encounter == bosses[n - 1]);
    }

    Floor a, b;
    a.generateFloor(77, 1);
    b.generateFloor(77, 1);
    CHECK(a.graph().vertexCount() == b.graph().vertexCount() && a.graph().edgeCount() == b.graph().edgeCount());
}

// -------------------------------------------------------------------------
void testRevealAndPath() {
    SECTION("M2: BFS reveal sesuai cahaya & Dijkstra lewat ruang terungkap");
    Floor f;
    f.generateFloor(12345, 1);
    CHECK(f.revealByLight(LightLevel::Dark) == 0);           // radius 0: hanya ruang sendiri
    int dim = f.revealByLight(LightLevel::Dim);
    CHECK(dim == static_cast<int>(f.graph().degree(f.entrance())));
    int bright = f.revealByLight(LightLevel::Bright);
    CHECK(bright >= 1);

    // boss masih tersembunyi → jalur tidak boleh ditemukan
    Stack<int> path;
    CHECK(f.findPath(f.boss(), path) == -1 && path.empty());
    f.revealAll();
    int cost = f.findPath(f.boss(), path);
    CHECK(cost > 0);
    int first = -1, last = -1, steps = 0, prev = -1;
    bool adjacentSteps = true;
    int id;
    while (path.pop(id)) {
        if (first < 0) first = id;
        if (prev >= 0 && !f.isAdjacent(prev, id)) adjacentSteps = false;
        prev = last = id;
        ++steps;
    }
    CHECK(first == f.entrance() && last == f.boss() && adjacentSteps && steps >= FLOOR_COLS);

    // collapse menjaga graph tetap terhubung
    Rng rng(9);
    int before = static_cast<int>(f.graph().edgeCount());
    bool collapsed = f.collapseCorridor(rng);
    CHECK(f.graph().isConnected());
    CHECK(static_cast<int>(f.graph().edgeCount()) == before - (collapsed ? 1 : 0));
}

// -------------------------------------------------------------------------
void testTravelAndLight() {
    SECTION("Run: perjalanan menghabiskan cahaya, battle dimulai");
    Run run(2024);
    CHECK(run.state() == RunState::Exploring && run.light() == MAX_LIGHT);
    int start = run.floor().currentRoom();
    CHECK(start == run.floor().entrance() && run.checkpoint() == start);

    int target = adjacentOfType(run, RoomType::Battle);
    if (target < 0) {   // lantai tanpa battle di dekat pintu masuk: pilih tetangga mana pun
        run.floor().graph().forEachNeighbor(start, [&](Floor::Vertex* v, int) { if (target < 0) target = v->id; });
    }
    int cost = run.lightCost(start, target);
    CHECK(cost > 0);
    CHECK(!run.travelTo(run.floor().boss()));                 // tidak bertetangga
    CHECK(run.travelTo(target));
    CHECK(run.light() == MAX_LIGHT - cost);
    CHECK(run.floor().currentRoom() == target && run.floor().room(target)->visited);
    if (run.floor().room(target)->type == RoomType::Battle) {
        CHECK(run.state() == RunState::InBattle && run.battle() != nullptr);
        forceBattle(run, true);
        CHECK(run.state() == RunState::Exploring && run.floor().room(target)->cleared && run.ash() > 0);
    }
}

// -------------------------------------------------------------------------
void testDeathAndRecovery() {
    SECTION("Run: party tumbang → checkpoint, Ash tertinggal & bisa dipungut");
    // cari seed yang punya battle di sebelah pintu masuk
    for (std::uint32_t seed = 1; seed < 100; ++seed) {
        Run run(seed);
        int battleRoom = adjacentOfType(run, RoomType::Battle);
        if (battleRoom < 0) continue;

        CHECK(run.travelTo(battleRoom));
        forceBattle(run, true);                                // menang dulu → punya Ash
        int ashBefore = run.ash();
        CHECK(ashBefore > 0);

        // cari battle berikutnya di sekitar lalu kalah di sana
        int next = adjacentOfType(run, RoomType::Battle);
        if (next < 0) continue;
        CHECK(run.travelTo(next));
        forceBattle(run, false);
        CHECK(run.deaths() == 1 && run.ash() == 0);
        CHECK(run.droppedAsh() == ashBefore && run.droppedAshRoom() == next);
        CHECK(run.floor().currentRoom() == run.checkpoint());
        CHECK(run.hero(0).hp == run.hero(0).maxHp);
        CHECK(!run.floor().room(next)->cleared);

        // kembali ke ruang kematian (auto-travel / Dijkstra) dan menang → Ash kembali
        run.floor().revealAll();
        run.autoTravel(next);
        if (run.state() == RunState::InBattle && run.floor().currentRoom() == next) {
            forceBattle(run, true);
            CHECK(run.droppedAsh() == 0 && run.ash() > ashBefore);
        }
        return;
    }
    CHECK(false);   // tidak ada seed yang cocok (seharusnya tidak terjadi)
}

// -------------------------------------------------------------------------
void testBossAndDescend() {
    SECTION("Run: boss → relic → lantai berikutnya → menang");
    Run run(31337);
    for (int floorNo = 1; floorNo <= MAX_FLOORS; ++floorNo) {
        CHECK(run.floor().number() == floorNo);
        run.floor().revealAll();
        // tempuh jalur ke boss; selesaikan semua yang menghadang
        for (int guard = 0; guard < 60 && run.state() != RunState::ChoosingRelic && run.state() != RunState::RunWon; ++guard) {
            switch (run.state()) {
                case RunState::Exploring:     run.autoTravel(run.floor().boss()); break;
                case RunState::InBattle:      forceBattle(run, true); break;
                case RunState::AtShrine:      run.rest(); run.leaveRoom(); break;
                case RunState::InShop:        run.leaveRoom(); break;
                case RunState::InEvent:       run.chooseEventOption(1); break;
                default: break;
            }
        }
        if (run.state() == RunState::ChoosingRelic) {
            CHECK(run.relicOptionCount() == RELIC_CHOICES);
            Relic r = run.relicOption(0);
            CHECK(run.chooseRelic(0));
            CHECK(run.hasRelic(r));
        }
    }
    CHECK(run.state() == RunState::RunWon);
}

}  // namespace

void runDungeonTests() {
    testGenerator();
    testRevealAndPath();
    testTravelAndLight();
    testDeathAndRecovery();
    testBossAndDescend();
}
