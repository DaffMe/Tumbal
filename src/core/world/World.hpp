#pragma once
// ============================================================================
// World — dunia TUMBAL sebagai Graph peta (Mekanik M2).
//
//   Vertex = satu peta (MapInfo: nama, berkas .tmj, TileMap yang dimuat,
//            status dikunjungi, punya Tungku atau tidak).
//   Edge   = jalan antar-peta yang ditemukan dari objek "door" di setiap
//            peta. Pintu dengan property `locked` membuat edge tertutup
//            (jalan pintas), `oneway` membuat edge satu arah.
//
// Dimuat dari assets/maps/world.json:
//   { "start": {"map": 1, "spawn": "awal"},
//     "maps": [ {"id": 1, "name": "Lumbung Buto", "file": "lumbung.tmj"}, ... ] }
// ============================================================================

#include <string>
#include "../adt/Graph.hpp"
#include "TileMap.hpp"

struct MapInfo {
    std::string name;
    std::string file;
    TileMap*    map = nullptr;     // dimiliki World
    bool        visited = false;
    bool        hasTungku = false;
};

// Hasil melewati pintu: peta tujuan + tile kedatangan.
struct TravelResult {
    bool ok = false;
    bool locked = false;           // pintu ada tapi edge masih tertutup
    int  toMap = 0;
    int  x = 0, y = 0;
    int  dir = 0;                  // arah hadap setelah tiba (0 bawah,1 kiri,2 kanan,3 atas)
};

class World {
public:
    using MapGraph = Graph<MapInfo>;

    World() = default;
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // Muat world.json + semua peta di folder yang sama.
    bool load(const std::string& worldJsonPath, std::string* error = nullptr);

    int startMap() const { return startMap_; }
    const std::string& startSpawn() const { return startSpawn_; }

    MapInfo* info(int mapId);
    const MapInfo* info(int mapId) const;
    TileMap* map(int mapId) { MapInfo* i = info(mapId); return i ? i->map : nullptr; }

    // M2 subprogram: lewati pintu `door` di peta `fromMap`.
    // Cek edge Graph (ada & terbuka), lalu tentukan titik tiba.
    TravelResult travel(int fromMap, const MapObject& door);

    // M2 subprogram: buka jalan pintas antara dua peta (edge jadi open).
    bool unlockShortcut(int a, int b);

    // M2 subprogram: cari peta ber-Tungku terdekat yang sudah dikunjungi
    // (BFS lewat edge terbuka). Mengembalikan id peta atau 0.
    int nearestTungku(int fromMap);

    // Tandai peta dikunjungi.
    void markVisited(int mapId);

    const MapGraph& graph() const { return graph_; }
    MapGraph& graph() { return graph_; }

private:
    MapGraph    graph_;
    int         startMap_ = 0;
    std::string startSpawn_;
};
