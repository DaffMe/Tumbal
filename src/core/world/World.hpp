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

class JsonValue;

struct MapInfo {
    std::string name;
    std::string file;
    TileMap*    map = nullptr;     // dimiliki World
    bool        visited = false;
    bool        hasTungku = false;
    bool        tungkuLit = false;    // Tungku peta ini sudah dinyalakan (tujuan warp)
    int         tungkuX = 0, tungkuY = 0;
    unsigned long long defeated = 0;  // bit i = musuh ke-i di peta ini sudah kalah
    unsigned long long collected = 0; // bit i = loot ke-i sudah diambil (permanen)
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

    // M2 subprogram: istirahat di Tungku — telusuri SEMUA vertex Graph dan
    // hidupkan kembali musuh di setiap peta (seperti bonfire Black Souls).
    // Mengembalikan jumlah peta yang musuhnya dihidupkan ulang.
    int restAtTungku();
    // Catat musuh ke-i di peta mapId sudah dikalahkan.
    void markDefeated(int mapId, int enemyIndex);
    bool isDefeated(int mapId, int enemyIndex) const;
    // Loot: indeks = urutan objek bertipe "loot" di peta.
    void markCollected(int mapId, int lootIndex);
    bool isCollected(int mapId, int lootIndex) const;

    // Nyalakan Tungku di peta (setelah pemain beristirahat di sana).
    void lightTungku(int mapId);
    // Mode curang: buka setiap edge terkunci (traversal semua vertex & edge Graph) dan
    // nyalakan semua Tungku sehingga warp bisa ke mana saja. Mengembalikan jumlah edge dibuka.
    int openEverything();
    // M2 subprogram: daftar Tungku menyala yang TERJANGKAU dari fromMap
    // lewat jalan terbuka (BFS). outMap/outDepth diisi berurutan dari yang
    // terdekat; mengembalikan jumlahnya. Tungku di wilayah yang jalannya
    // masih terkunci tidak muncul: warp mengikuti keterhubungan Graph.
    int warpTargets(int fromMap, int* outMap, int* outDepth, int max);

    // Simpan/muat status dunia: peta dikunjungi, Tungku menyala, musuh kalah,
    // loot diambil, dan status buka/kunci setiap edge Graph.
    std::string toJson() const;
    bool fromJson(const JsonValue* v);

    // Tandai peta dikunjungi.
    void markVisited(int mapId);

    const MapGraph& graph() const { return graph_; }
    MapGraph& graph() { return graph_; }

private:
    MapGraph    graph_;
    int         startMap_ = 0;
    std::string startSpawn_;
};
