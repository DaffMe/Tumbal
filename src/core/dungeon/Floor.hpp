#pragma once
// ============================================================================
// Floor — satu lantai dungeon. Mekanik M2 (Dungeon Exploration).
//
// Ruangan = vertex, koridor = edge berbobot pada Graph<Room>
// (adjacency multi linked list). Bobot koridor × LIGHT_PER_WEIGHT = biaya
// cahaya lentera untuk melewatinya.
//
// Tata letak: grid FLOOR_COLS kolom × FLOOR_ROWS baris. Kolom 0 = pintu masuk,
// kolom terakhir = ruang boss. id ruangan = col * FLOOR_ROWS + row sehingga
// GUI bisa langsung menghitung posisi gambarnya.
// ============================================================================

#include <cstdint>
#include "../adt/Graph.hpp"
#include "../adt/Queue.hpp"
#include "../adt/Stack.hpp"
#include "../battle/BattleEvent.hpp"
#include "../battle/Encounters.hpp"
#include "../util/Rng.hpp"

constexpr int FLOOR_COLS       = 7;
constexpr int FLOOR_ROWS       = 4;
constexpr int LIGHT_PER_WEIGHT = 4;
constexpr int EVENT_COUNT      = 4;

enum class RoomType { Entrance, Battle, Elite, Shrine, Event, Merchant, Treasure, Boss };
enum class LightLevel { Bright, Dim, Dark };

struct Room {
    RoomType    type      = RoomType::Battle;
    int         col       = 0;
    int         row       = 0;
    bool        revealed  = false;   // jenis ruangan sudah diketahui pemain
    bool        visited   = false;
    bool        cleared   = false;   // isi ruangan sudah selesai (musuh kalah, event dipakai, ...)
    EncounterId encounter = EncounterId::Thralls;
    int         eventId   = 0;
};

const char* roomTypeName(RoomType t);
char        roomSymbol(RoomType t);
int         revealRadius(LightLevel lvl);

inline int roomId(int col, int row) { return col * FLOOR_ROWS + row; }

class Floor {
public:
    using RoomGraph = Graph<Room>;
    using Vertex    = RoomGraph::Vertex;

    Floor() : number_(1), entrance_(-1), boss_(-1), current_(-1), log_(nullptr) {}

    void setLog(Queue<BattleEvent>* log) { log_ = log; }

    // ---------------------------------------------------- subprogram M2
    // Bangun graph lantai secara prosedural (pasti terhubung, ada siklus).
    void generateFloor(std::uint32_t seed, int floorNo);
    // Ungkap ruangan dalam radius BFS sesuai level cahaya. Kembalikan jumlah ruangan baru.
    int  revealByLight(LightLevel level);
    // Jalur termurah (Dijkstra) dari ruang saat ini ke target, hanya lewat ruang terungkap.
    int  findPath(int targetId, Stack<int>& path);
    // Shrine terungkap terdekat → id shrine (path terisi), atau -1.
    int  findNearestShrine(Stack<int>& path);
    // Runtuhkan satu koridor di sekitar ruang saat ini tanpa memutus graph.
    bool collapseCorridor(Rng& rng);

    // ---------------------------------------------------- operasi lain
    bool moveTo(int roomId);          // hanya ke ruangan bertetangga
    void setCurrent(int roomId);      // teleport (respawn di checkpoint)
    void revealAll();

    Room*       room(int id);
    const Room* room(int id) const;
    int  currentRoom() const { return current_; }
    int  entrance() const { return entrance_; }
    int  boss() const { return boss_; }
    int  number() const { return number_; }
    bool isAdjacent(int a, int b) const { return graph_.hasEdge(a, b); }
    int  travelCost(int a, int b) const;   // biaya cahaya dasar, -1 jika tidak bertetangga

    RoomGraph&       graph() { return graph_; }
    const RoomGraph& graph() const { return graph_; }

private:
    RoomGraph           graph_;
    int                 number_;
    int                 entrance_;
    int                 boss_;
    int                 current_;
    Queue<BattleEvent>* log_;

    void dsOp(const std::string& text);
    void info(const std::string& text);
    bool hasNeighborInCol(int id, int col);
};
