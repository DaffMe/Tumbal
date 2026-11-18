// ============================================================================
// Unit test ADT manual (Queue, Stack, Timeline, Graph).
// Framework test minimal buatan sendiri: CHECK(kondisi) mencatat gagal/lulus.
// Jalankan: mingw32-make test
// ============================================================================

#include <cstdio>
#include <string>
#include "../src/core/adt/Queue.hpp"
#include "../src/core/adt/Stack.hpp"
#include "../src/core/adt/Timeline.hpp"
#include "../src/core/adt/Graph.hpp"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) { ++g_pass; }                                              \
        else { ++g_fail; std::printf("  GAGAL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

#define SECTION(name) std::printf("[%s]\n", name)

// Unit tiruan untuk Timeline
struct FakeUnit { std::string name; };

// Gabungkan nama unit di timeline jadi string "A B C" agar mudah dibandingkan
static std::string order(const Timeline<FakeUnit>& t) {
    std::string s;
    t.forEach([&s](const Timeline<FakeUnit>::Node& n) {
        if (!s.empty()) s += ' ';
        s += n.item->name;
    });
    return s;
}

static std::string orderReverse(const Timeline<FakeUnit>& t) {
    std::string s;
    t.forEachReverse([&s](const Timeline<FakeUnit>::Node& n) {
        if (!s.empty()) s += ' ';
        s += n.item->name;
    });
    return s;
}

// ---------------------------------------------------------------------------
static void testQueue() {
    SECTION("Queue");
    Queue<int> q;
    CHECK(q.empty());
    int x = -1;
    CHECK(!q.dequeue(x));
    for (int i = 1; i <= 5; ++i) q.enqueue(i);
    CHECK(q.size() == 5);
    CHECK(q.peek(x) && x == 1);
    CHECK(q.dequeue(x) && x == 1);
    CHECK(q.dequeue(x) && x == 2);
    q.enqueue(6);
    int sum = 0;
    q.forEach([&sum](int v) { sum += v; });
    CHECK(sum == 3 + 4 + 5 + 6);
    while (q.dequeue(x)) {}
    CHECK(q.empty() && q.size() == 0);
    q.enqueue(9);                          // tail harus ter-reset dengan benar
    CHECK(q.dequeue(x) && x == 9 && q.empty());
}

// ---------------------------------------------------------------------------
static void testStack() {
    SECTION("Stack");
    Stack<int> s;
    int x = -1;
    CHECK(s.empty() && !s.pop(x));
    s.push(1); s.push(2); s.push(3);
    CHECK(s.size() == 3);
    CHECK(s.peek(x) && x == 3);
    CHECK(s.pop(x) && x == 3);
    CHECK(s.pop(x) && x == 2);
    s.push(7);
    CHECK(s.pop(x) && x == 7);
    CHECK(s.pop(x) && x == 1);
    CHECK(s.empty());
}

// ---------------------------------------------------------------------------
static void testTimeline() {
    SECTION("Timeline (Doubly Linked List)");
    FakeUnit a{"A"}, b{"B"}, c{"C"}, d{"D"}, e{"E"};
    Timeline<FakeUnit> t;

    // insertSorted menjaga urutan menaik
    t.insertSorted(&b, 50);
    t.insertSorted(&a, 20);
    t.insertSorted(&c, 80);
    t.insertSorted(&d, 50);               // AV sama dengan B → setelah B
    CHECK(order(t) == "A B D C");
    CHECK(orderReverse(t) == "C D B A");  // pointer prev konsisten
    CHECK(t.size() == 4 && t.isValid());

    // search
    CHECK(t.search(&d) != nullptr && t.search(&d)->av == 50);
    CHECK(t.search(&e) == nullptr);

    // DELAY: A (20) → 70, harus pindah ke antara D(50) dan C(80)
    CHECK(t.updateAV(&a, 70));
    CHECK(order(t) == "B D A C" && t.isValid());

    // ADVANCE: C (80) → 10, harus jadi head
    CHECK(t.updateAV(&c, 10));
    CHECK(order(t) == "C B D A" && t.isValid());

    // ADVANCE ke nilai sama dengan B: tidak boleh menyalip B
    CHECK(t.updateAV(&a, 50));
    CHECK(order(t) == "C B D A" && t.isValid());

    // DELAY ke ekor
    CHECK(t.updateAV(&b, 999));
    CHECK(order(t) == "C D A B" && t.back()->item == &b && t.isValid());

    // subtractAll = waktu berjalan sebesar AV head
    t.subtractAll(t.front()->av);
    CHECK(t.front()->av == 0.0f && t.isValid());

    // popFront = unit yang bertindak
    FakeUnit* who = nullptr; float av = -1;
    CHECK(t.popFront(who, av) && who == &c && av == 0.0f);
    CHECK(order(t) == "D A B" && t.isValid());

    // insertFront = interject (AV dipaksa ≤ head)
    t.insertFront(&e, 500);
    CHECK(order(t) == "E D A B" && t.front()->av <= t.front()->next->av && t.isValid());

    // remove di tengah, head, dan tail
    CHECK(t.remove(&a));
    CHECK(order(t) == "E D B" && t.isValid());
    CHECK(t.remove(&e));
    CHECK(t.remove(&b));
    CHECK(order(t) == "D" && t.front() == t.back() && t.isValid());
    CHECK(!t.remove(&a));                 // sudah tidak ada

    // deep copy tidak berbagi node
    t.insertSorted(&a, 5);
    Timeline<FakeUnit> copy(t);
    copy.updateAV(&a, 900);
    CHECK(order(t) == "A D" && order(copy) == "D A");
    CHECK(copy.isValid() && t.isValid());

    t.clear();
    CHECK(t.empty() && t.front() == nullptr && t.back() == nullptr && t.isValid());
}

// ---------------------------------------------------------------------------
static void testGraph() {
    SECTION("Graph (adjacency multi linked list)");
    //   0 --1-- 1 --1-- 2
    //   |               |
    //   5               1
    //   |               |
    //   3 ------1------ 4      5 (terisolasi)
    Graph<std::string> g;
    CHECK(g.addVertex(0, "Gerbang"));
    g.addVertex(1, "Lorong");
    g.addVertex(2, "Kapel");
    g.addVertex(3, "Krypta");
    g.addVertex(4, "Shrine");
    CHECK(g.addVertex(0, "duplikat") == nullptr);
    CHECK(g.vertexCount() == 5);

    CHECK(g.addEdge(0, 1, 1));
    CHECK(g.addEdge(1, 2, 1));
    CHECK(g.addEdge(2, 4, 1));
    CHECK(g.addEdge(0, 3, 5));
    CHECK(g.addEdge(3, 4, 1));
    CHECK(!g.addEdge(1, 0, 3));           // edge ganda ditolak
    CHECK(!g.addEdge(2, 2, 1));           // self loop ditolak
    CHECK(!g.addEdge(0, 99, 1));          // vertex tak ada
    CHECK(g.edgeCount() == 5);
    CHECK(g.hasEdge(4, 2) && g.edgeWeight(0, 3) == 5);
    CHECK(g.degree(0) == 2 && g.degree(4) == 2);
    CHECK(g.findVertex(2)->data == "Kapel");
    CHECK(g.isConnected());

    // BFS dengan batas kedalaman (radius cahaya)
    int seen = 0;
    g.bfs(0, 1, [&seen](Graph<std::string>::Vertex*, int) { ++seen; });
    CHECK(seen == 3);                     // 0, 1, 3
    int maxD = 0;
    CHECK(g.bfs(0, -1, [&maxD](Graph<std::string>::Vertex*, int d) { if (d > maxD) maxD = d; }) == 5);
    CHECK(maxD == 2);

    // Dijkstra: 0→4 lewat 1,2 (biaya 3) lebih murah daripada lewat 3 (biaya 6)
    Stack<int> path;
    CHECK(g.dijkstra(0, 4, path) == 3);
    std::string p; int id;
    while (path.pop(id)) p += std::to_string(id);
    CHECK(p == "0124");

    // Koridor runtuh: hapus 1—2 → jalur terpaksa lewat 3
    CHECK(g.removeEdge(1, 2));
    CHECK(!g.hasEdge(2, 1) && g.edgeCount() == 4);
    CHECK(g.dijkstra(0, 4, path) == 6);
    p.clear();
    while (path.pop(id)) p += std::to_string(id);
    CHECK(p == "034");
    CHECK(g.isConnected());

    // Vertex terisolasi → tidak terhubung, Dijkstra -1
    g.addVertex(5, "Ruang Rahasia");
    CHECK(!g.isConnected());
    CHECK(g.dijkstra(0, 5, path) == -1 && path.empty());

    // removeVertex membersihkan edge yang menunjuk ke vertex itu
    CHECK(g.removeVertex(4));
    CHECK(g.vertexCount() == 5 && !g.hasEdge(3, 4) && g.degree(2) == 0);
    CHECK(g.edgeCount() == 2);            // tersisa 0—1 dan 0—3
    CHECK(!g.removeVertex(4));

    g.clear();
    CHECK(g.vertexCount() == 0 && g.edgeCount() == 0 && g.isConnected());
}

// ---------------------------------------------------------------------------
int main() {
    testQueue();
    testStack();
    testTimeline();
    testGraph();
    std::printf("\n%d lulus, %d gagal\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
