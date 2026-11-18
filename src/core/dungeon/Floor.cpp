#include "Floor.hpp"
#include "../util/Format.hpp"

const char* roomTypeName(RoomType t) {
    switch (t) {
        case RoomType::Entrance: return "Pintu Masuk";
        case RoomType::Battle:   return "Pertarungan";
        case RoomType::Elite:    return "Elite";
        case RoomType::Shrine:   return "Ember Shrine";
        case RoomType::Event:    return "Peristiwa";
        case RoomType::Merchant: return "Pedagang";
        case RoomType::Treasure: return "Harta";
        case RoomType::Boss:     return "BOSS";
    }
    return "?";
}

char roomSymbol(RoomType t) {
    switch (t) {
        case RoomType::Entrance: return 'E';
        case RoomType::Battle:   return 'x';
        case RoomType::Elite:    return 'X';
        case RoomType::Shrine:   return 'S';
        case RoomType::Event:    return '?';
        case RoomType::Merchant: return '$';
        case RoomType::Treasure: return 'T';
        case RoomType::Boss:     return 'B';
    }
    return ' ';
}

int revealRadius(LightLevel lvl) {
    switch (lvl) {
        case LightLevel::Bright: return 2;
        case LightLevel::Dim:    return 1;
        case LightLevel::Dark:   return 0;
    }
    return 0;
}

// ============================================================================
// Generator lantai
// ============================================================================

bool Floor::hasNeighborInCol(int id, int col) {
    bool found = false;
    graph_.forEachNeighbor(id, [&](Vertex* v, int) {
        if (v->data.col == col) found = true;
    });
    return found;
}

void Floor::generateFloor(std::uint32_t seed, int floorNo) {
    graph_.clear();
    number_ = floorNo;
    Rng rng(seed ^ (static_cast<std::uint32_t>(floorNo) * 0x9E3779B9u));

    // ---- 1. Tempatkan ruangan per kolom (vertex)
    int colRows[FLOOR_COLS][FLOOR_ROWS];
    int colCount[FLOOR_COLS];
    for (int c = 0; c < FLOOR_COLS; ++c) {
        if (c == 0 || c == FLOOR_COLS - 1) {
            colCount[c] = 1;
            colRows[c][0] = rng.range(1, 2);
        } else {
            int rows[FLOOR_ROWS] = {0, 1, 2, 3};
            for (int i = FLOOR_ROWS - 1; i > 0; --i) {   // acak (Fisher-Yates)
                int j = rng.range(0, i);
                int t = rows[i]; rows[i] = rows[j]; rows[j] = t;
            }
            colCount[c] = rng.range(2, 4);
            for (int i = 0; i < colCount[c]; ++i) colRows[c][i] = rows[i];
            for (int i = 1; i < colCount[c]; ++i)          // urutkan baris (insertion sort)
                for (int j = i; j > 0 && colRows[c][j - 1] > colRows[c][j]; --j) {
                    int t = colRows[c][j]; colRows[c][j] = colRows[c][j - 1]; colRows[c][j - 1] = t;
                }
        }
        for (int i = 0; i < colCount[c]; ++i) {
            Room r;
            r.col = c;
            r.row = colRows[c][i];
            graph_.addVertex(roomId(c, r.row), r);
        }
    }

    // ---- 2. Koridor maju: setiap ruang terhubung ke kolom berikutnya (edge)
    auto nearestInCol = [&](int row, int col) {
        int best = colRows[col][0];
        for (int i = 1; i < colCount[col]; ++i) {
            int r = colRows[col][i];
            int d = r > row ? r - row : row - r;
            int bd = best > row ? best - row : row - best;
            if (d < bd || (d == bd && rng.chance(0.5f))) best = r;
        }
        return best;
    };
    for (int c = 0; c < FLOOR_COLS - 1; ++c) {
        for (int i = 0; i < colCount[c]; ++i) {
            int a = colRows[c][i];
            graph_.addEdge(roomId(c, a), roomId(c + 1, nearestInCol(a, c + 1)), rng.range(1, 3));
            if (rng.chance(0.35f)) {   // cabang tambahan
                int b = colRows[c + 1][rng.range(0, colCount[c + 1] - 1)];
                int d = b > a ? b - a : a - b;
                if (d <= 1) graph_.addEdge(roomId(c, a), roomId(c + 1, b), rng.range(1, 3));
            }
        }
        // pastikan setiap ruang di kolom c+1 punya jalan masuk dari kolom c
        for (int i = 0; i < colCount[c + 1]; ++i) {
            int b = colRows[c + 1][i];
            if (!hasNeighborInCol(roomId(c + 1, b), c))
                graph_.addEdge(roomId(c, nearestInCol(b, c)), roomId(c + 1, b), rng.range(1, 3));
        }
    }

    // ---- 3. Koridor vertikal di dalam kolom → membentuk siklus
    for (int c = 1; c < FLOOR_COLS - 1; ++c)
        for (int i = 0; i + 1 < colCount[c]; ++i)
            if (colRows[c][i + 1] - colRows[c][i] == 1 && rng.chance(0.3f))
                graph_.addEdge(roomId(c, colRows[c][i]), roomId(c, colRows[c][i + 1]), rng.range(1, 2));

    // ---- 4. Tentukan jenis ruangan
    entrance_ = roomId(0, colRows[0][0]);
    boss_     = roomId(FLOOR_COLS - 1, colRows[FLOOR_COLS - 1][0]);
    Room* ent = room(entrance_);
    ent->type = RoomType::Entrance;
    ent->revealed = ent->visited = ent->cleared = true;
    Room* bos = room(boss_);
    bos->type = RoomType::Boss;
    bos->encounter = EncounterId::ChoirWarden;

    bool assigned[FLOOR_COLS * FLOOR_ROWS] = {};
    auto pick = [&](int colMin, int colMax) -> Room* {
        int cands[FLOOR_COLS * FLOOR_ROWS];
        int n = 0;
        for (int c = colMin; c <= colMax; ++c)
            for (int i = 0; i < colCount[c]; ++i) {
                int id = roomId(c, colRows[c][i]);
                if (!assigned[id]) cands[n++] = id;
            }
        if (n == 0) return nullptr;
        int id = cands[rng.range(0, n - 1)];
        assigned[id] = true;
        return room(id);
    };

    if (Room* r = pick(3, 3)) r->type = RoomType::Shrine;                     // shrine tengah
    if (Room* r = pick(FLOOR_COLS - 2, FLOOR_COLS - 2)) r->type = RoomType::Shrine;  // sebelum boss
    if (Room* r = pick(2, 4)) r->type = RoomType::Merchant;
    if (Room* r = pick(4, 5)) { r->type = RoomType::Elite; r->encounter = EncounterId::Gargoyle; }
    if (Room* r = pick(1, 5)) r->type = RoomType::Treasure;
    for (int k = 0; k < 2; ++k)
        if (Room* r = pick(1, 5)) { r->type = RoomType::Event; r->eventId = rng.range(0, EVENT_COUNT - 1); }

    const EncounterId pool[] = {EncounterId::Thralls, EncounterId::BellChoir, EncounterId::Candlelight};
    graph_.forEachVertex([&](Vertex* v) {
        if (assigned[v->id] || v->id == entrance_ || v->id == boss_) return;
        v->data.type = RoomType::Battle;
        v->data.encounter = (v->data.col == 1) ? EncounterId::Thralls : pool[rng.range(0, 2)];
    });

    current_ = entrance_;
    dsOp(strf("Graph dibangun: %d vertex (ruang), %d edge (koridor), terhubung=%s",
              static_cast<int>(graph_.vertexCount()), static_cast<int>(graph_.edgeCount()),
              graph_.isConnected() ? "ya" : "TIDAK"));
}

// ============================================================================
// Eksplorasi
// ============================================================================

int Floor::revealByLight(LightLevel level) {
    int radius = revealRadius(level);
    int fresh = 0;
    int reached = graph_.bfs(current_, radius, [&fresh](Vertex* v, int) {
        if (!v->data.revealed) {
            v->data.revealed = true;
            ++fresh;
        }
    });
    dsOp(strf("Graph.bfs(start=%d, radius=%d) -> %d ruang dijangkau, %d baru terungkap",
              current_, radius, reached, fresh));
    return fresh;
}

void Floor::revealAll() {
    graph_.forEachVertex([](Vertex* v) { v->data.revealed = true; });
    dsOp("Graph.forEachVertex(): semua ruang terungkap");
}

bool Floor::moveTo(int id) {
    if (!graph_.hasEdge(current_, id)) return false;
    current_ = id;
    Room* r = room(id);
    r->visited = r->revealed = true;
    return true;
}

void Floor::setCurrent(int id) {
    if (graph_.findVertex(id) == nullptr) return;
    current_ = id;
    room(id)->visited = room(id)->revealed = true;
}

int Floor::travelCost(int a, int b) const {
    int w = graph_.edgeWeight(a, b);
    return w < 0 ? -1 : w * LIGHT_PER_WEIGHT;
}

int Floor::findPath(int targetId, Stack<int>& path) {
    const Room* t = room(targetId);
    if (t == nullptr || !t->revealed) {
        path.clear();
        return -1;
    }
    int cost = graph_.dijkstra(current_, targetId, path, [](Vertex* v) { return v->data.revealed; });
    dsOp(strf("Graph.dijkstra(%d -> %d) -> bobot %d, %d ruang di jalur (Stack)",
              current_, targetId, cost, static_cast<int>(path.size())));
    return cost;
}

int Floor::findNearestShrine(Stack<int>& path) {
    int bestId = -1, bestCost = -1;
    graph_.forEachVertex([&](Vertex* v) {
        if (v->data.type != RoomType::Shrine || !v->data.revealed || v->id == current_) return;
        Stack<int> tmp;
        int c = graph_.dijkstra(current_, v->id, tmp, [](Vertex* x) { return x->data.revealed; });
        if (c >= 0 && (bestCost < 0 || c < bestCost)) {
            bestCost = c;
            bestId = v->id;
        }
    });
    if (bestId < 0) {
        path.clear();
        dsOp("Graph.dijkstra ke setiap shrine terungkap -> tidak ada yang terjangkau");
        return -1;
    }
    findPath(bestId, path);
    return bestId;
}

// Pilih koridor acak di sekitar ruang saat ini, hapus (removeEdge), lalu cek
// isConnected (BFS). Jika lantai terputus, koridor dipulihkan dan coba yang lain.
bool Floor::collapseCorridor(Rng& rng) {
    int neigh[FLOOR_COLS * FLOOR_ROWS];
    int n = 0;
    graph_.forEachNeighbor(current_, [&](Vertex* v, int) { neigh[n++] = v->id; });
    for (int i = n - 1; i > 0; --i) {
        int j = rng.range(0, i);
        int t = neigh[i]; neigh[i] = neigh[j]; neigh[j] = t;
    }
    for (int i = 0; i < n; ++i) {
        int other = neigh[i];
        int w = graph_.edgeWeight(current_, other);
        graph_.removeEdge(current_, other);
        dsOp(strf("Graph.removeEdge(%d, %d)", current_, other));
        if (graph_.isConnected()) {
            dsOp("Graph.isConnected() -> ya, koridor runtuh permanen");
            info(strf("Koridor menuju ruang %d runtuh!", other));
            return true;
        }
        graph_.addEdge(current_, other, w);
        dsOp(strf("Graph.isConnected() -> tidak; Graph.addEdge(%d, %d) dipulihkan", current_, other));
    }
    return false;
}

Room* Floor::room(int id) {
    Vertex* v = graph_.findVertex(id);
    return v ? &v->data : nullptr;
}

const Room* Floor::room(int id) const {
    Vertex* v = graph_.findVertex(id);
    return v ? &v->data : nullptr;
}

void Floor::dsOp(const std::string& text) {
    if (log_) log_->enqueue(BattleEvent{EventType::DsOp, -1, -1, 0, text});
}

void Floor::info(const std::string& text) {
    if (log_) log_->enqueue(BattleEvent{EventType::Info, -1, -1, 0, text});
}
