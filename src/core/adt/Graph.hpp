#pragma once
// ============================================================================
// Graph tak berarah berbobot — representasi adjacency MULTI LINKED LIST.
// Struktur data utama Mekanik M2 (Dungeon Exploration).
//
//   Vertex list (linked):  [R0] → [R1] → [R2] → ...
//                           |      |      |
//   Edge list per vertex:  (→R1,w2) (→R0,w2) (→R0,w1)
//                          (→R2,w1) (→R2,w3)  ...
//
// - Vertex = ruangan dungeon (payload T = data ruangan).
// - Edge   = koridor; weight = biaya cahaya lentera untuk melewatinya.
// - Setiap edge menunjuk langsung ke Vertex tujuan (pointer), bukan indeks,
//   sehingga relasi antar-node terlihat jelas (multi linked list).
// - Graph tak berarah → setiap koridor disimpan sebagai DUA edge (a→b, b→a).
//
// Kenapa Graph (bukan Tree): ruangan bisa punya banyak tetangga dan ada
// siklus (lebih dari satu jalan menuju ruangan yang sama).
//
// Algoritma memakai field "scratch" di Vertex (visited, dist, parent) —
// pendekatan klasik buku teks — sehingga tidak butuh array/map tambahan.
// ============================================================================

#include <cstddef>
#include "Queue.hpp"
#include "Stack.hpp"

template <typename T>
class Graph {
public:
    struct Vertex;

    struct Edge {
        Vertex* to;
        int     weight;
        Edge*   next;     // edge berikutnya milik vertex yang sama
    };

    struct Vertex {
        int     id;
        T       data;
        Edge*   edges;    // head edge list
        Vertex* next;     // vertex berikutnya di vertex list
        // ---- scratch untuk algoritma traversal ----
        bool    visited;
        int     dist;
        Vertex* parent;
    };

    static constexpr int INF = 1 << 29;

    Graph() : vertices_(nullptr), vertexCount_(0), edgeCount_(0) {}
    ~Graph() { clear(); }

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // ---------------------------------------------------------------- vertex

    // Tambah vertex baru. Gagal (nullptr) jika id sudah dipakai.
    Vertex* addVertex(int id, const T& data) {
        if (findVertex(id) != nullptr) return nullptr;
        Vertex* v = new Vertex{id, data, nullptr, nullptr, false, INF, nullptr};
        // sisip di akhir agar urutan iterasi = urutan penambahan
        if (vertices_ == nullptr) {
            vertices_ = v;
        } else {
            Vertex* last = vertices_;
            while (last->next != nullptr) last = last->next;
            last->next = v;
        }
        ++vertexCount_;
        return v;
    }

    Vertex* findVertex(int id) const {
        for (Vertex* v = vertices_; v != nullptr; v = v->next)
            if (v->id == id) return v;
        return nullptr;
    }

    // Hapus vertex beserta semua edge yang menuju/keluar darinya.
    bool removeVertex(int id) {
        Vertex* target = findVertex(id);
        if (target == nullptr) return false;
        // hapus edge dari vertex lain yang menunjuk ke target
        for (Vertex* v = vertices_; v != nullptr; v = v->next)
            if (v != target) removeDirected(v, target);
        // hapus edge list milik target
        while (target->edges != nullptr) {
            Edge* e = target->edges;
            target->edges = e->next;
            delete e;
            --edgeCount_;
        }
        // lepas dari vertex list
        if (vertices_ == target) {
            vertices_ = target->next;
        } else {
            Vertex* p = vertices_;
            while (p->next != target) p = p->next;
            p->next = target->next;
        }
        delete target;
        --vertexCount_;
        return true;
    }

    // ------------------------------------------------------------------ edge

    // Tambah koridor tak berarah a—b dengan bobot w.
    bool addEdge(int a, int b, int weight) {
        if (a == b) return false;
        Vertex* va = findVertex(a);
        Vertex* vb = findVertex(b);
        if (va == nullptr || vb == nullptr) return false;
        if (findEdge(va, vb) != nullptr) return false;   // tanpa edge ganda
        va->edges = new Edge{vb, weight, va->edges};
        vb->edges = new Edge{va, weight, vb->edges};
        edgeCount_ += 2;
        return true;
    }

    // Hapus koridor a—b (mis. event "koridor runtuh").
    bool removeEdge(int a, int b) {
        Vertex* va = findVertex(a);
        Vertex* vb = findVertex(b);
        if (va == nullptr || vb == nullptr) return false;
        bool r1 = removeDirected(va, vb);
        bool r2 = removeDirected(vb, va);
        return r1 && r2;
    }

    bool hasEdge(int a, int b) const {
        Vertex* va = findVertex(a);
        Vertex* vb = findVertex(b);
        return va != nullptr && vb != nullptr && findEdge(va, vb) != nullptr;
    }

    // Bobot koridor a—b, atau -1 jika tidak ada.
    int edgeWeight(int a, int b) const {
        Vertex* va = findVertex(a);
        Vertex* vb = findVertex(b);
        if (va == nullptr || vb == nullptr) return -1;
        Edge* e = findEdge(va, vb);
        return e != nullptr ? e->weight : -1;
    }

    // ------------------------------------------------------------- traversal

    // Kunjungi tetangga vertex id. fn(Vertex* tetangga, int bobot)
    template <typename Fn>
    void forEachNeighbor(int id, Fn fn) const {
        Vertex* v = findVertex(id);
        if (v == nullptr) return;
        for (Edge* e = v->edges; e != nullptr; e = e->next) fn(e->to, e->weight);
    }

    template <typename Fn>
    void forEachVertex(Fn fn) const {
        for (Vertex* v = vertices_; v != nullptr; v = v->next) fn(v);
    }

    // Breadth-First Search dari start sampai kedalaman maxDepth (jumlah
    // koridor, bukan bobot). fn(Vertex*, int depth) dipanggil untuk setiap
    // vertex yang terjangkau, termasuk start (depth 0).
    // maxDepth < 0 berarti tanpa batas. Mengembalikan jumlah vertex dikunjungi.
    template <typename Fn>
    int bfs(int startId, int maxDepth, Fn fn) {
        Vertex* start = findVertex(startId);
        if (start == nullptr) return 0;
        resetScratch();

        Queue<Vertex*> q;
        start->visited = true;
        start->dist = 0;
        q.enqueue(start);
        int count = 0;

        Vertex* v = nullptr;
        while (q.dequeue(v)) {
            fn(v, v->dist);
            ++count;
            if (maxDepth >= 0 && v->dist >= maxDepth) continue;
            for (Edge* e = v->edges; e != nullptr; e = e->next) {
                if (!e->to->visited) {
                    e->to->visited = true;
                    e->to->dist = v->dist + 1;
                    e->to->parent = v;
                    q.enqueue(e->to);
                }
            }
        }
        return count;
    }

    // Graph terhubung jika BFS dari vertex pertama menjangkau semua vertex.
    bool isConnected() {
        if (vertices_ == nullptr) return true;
        int reached = bfs(vertices_->id, -1, [](Vertex*, int) {});
        return reached == static_cast<int>(vertexCount_);
    }

    // Dijkstra: jalur dengan total bobot terkecil dari start ke target.
    // Jalur (id vertex, start → target) dimasukkan ke pathOut lewat Stack:
    // rekonstruksi berjalan mundur via parent, di-push ke Stack, lalu
    // pemanggil cukup pop untuk mendapat urutan maju.
    // Mengembalikan total bobot, atau -1 jika tidak terjangkau.
    // Kompleksitas O(V^2) — cukup untuk dungeon ±20 ruangan, tanpa
    // priority queue siap pakai.
    int dijkstra(int startId, int targetId, Stack<int>& pathOut) {
        pathOut.clear();
        Vertex* start  = findVertex(startId);
        Vertex* target = findVertex(targetId);
        if (start == nullptr || target == nullptr) return -1;
        resetScratch();
        start->dist = 0;

        while (true) {
            // pilih vertex belum final dengan dist terkecil
            Vertex* u = nullptr;
            for (Vertex* v = vertices_; v != nullptr; v = v->next)
                if (!v->visited && v->dist < INF && (u == nullptr || v->dist < u->dist))
                    u = v;
            if (u == nullptr || u == target) break;
            u->visited = true;
            // relaksasi edge
            for (Edge* e = u->edges; e != nullptr; e = e->next) {
                Vertex* w = e->to;
                if (!w->visited && u->dist + e->weight < w->dist) {
                    w->dist = u->dist + e->weight;
                    w->parent = u;
                }
            }
        }

        if (target->dist >= INF) return -1;
        for (Vertex* v = target; v != nullptr; v = v->parent) pathOut.push(v->id);
        return target->dist;
    }

    // ------------------------------------------------------------------ info

    std::size_t vertexCount() const { return vertexCount_; }
    std::size_t edgeCount()   const { return edgeCount_ / 2; }   // koridor
    std::size_t degree(int id) const {
        std::size_t d = 0;
        forEachNeighbor(id, [&d](Vertex*, int) { ++d; });
        return d;
    }
    Vertex* firstVertex() const { return vertices_; }

    void clear() {
        while (vertices_ != nullptr) {
            Vertex* v = vertices_;
            vertices_ = v->next;
            while (v->edges != nullptr) {
                Edge* e = v->edges;
                v->edges = e->next;
                delete e;
            }
            delete v;
        }
        vertexCount_ = 0;
        edgeCount_ = 0;
    }

private:
    Vertex*     vertices_;
    std::size_t vertexCount_;
    std::size_t edgeCount_;   // jumlah edge berarah (2 × koridor)

    Edge* findEdge(Vertex* from, Vertex* to) const {
        for (Edge* e = from->edges; e != nullptr; e = e->next)
            if (e->to == to) return e;
        return nullptr;
    }

    bool removeDirected(Vertex* from, Vertex* to) {
        Edge* prev = nullptr;
        for (Edge* e = from->edges; e != nullptr; prev = e, e = e->next) {
            if (e->to == to) {
                if (prev == nullptr) from->edges = e->next; else prev->next = e->next;
                delete e;
                --edgeCount_;
                return true;
            }
        }
        return false;
    }

    void resetScratch() {
        for (Vertex* v = vertices_; v != nullptr; v = v->next) {
            v->visited = false;
            v->dist = INF;
            v->parent = nullptr;
        }
    }
};
