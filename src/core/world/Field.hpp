#pragma once
// ============================================================================
// Field — keadaan eksplorasi di peta yang sedang aktif.
//
// Gerak berbasis grid seperti RPG Maker: karakter selalu berdiri di satu
// tile, berpindah ke tile tetangga dengan interpolasi halus. Delapan arah
// (seperti skrip gerak 8 arah Black Souls); gerak diagonal hanya boleh jika
// kedua tile ortogonal juga bisa dilewati (tidak memotong sudut tembok).
//
// Musuh terlihat (symbol encounter, replikasi skrip シンボル Black Souls):
//   - berkeliaran pelan di sekitar titik asalnya;
//   - bereaksi bila pemain dalam 3 tile (5 tile bila pemain berlari),
//     menampilkan "!" lalu mengejar lebih cepat;
//   - menyentuh musuh dari BELAKANG = pemain menyerang duluan (preemptive);
//     disentuh musuh dari belakang = disergap (surprise).
//   - musuh yang dikalahkan hilang sampai pemain beristirahat di Tungku.
//
// GUI hanya membaca posisi dan memanggil update() dengan input arah; semua
// aturan gerak, pemicu pintu, dan kontak musuh ada di sini.
// ============================================================================

#include "../util/Rng.hpp"
#include "World.hpp"

enum Facing { FACE_DOWN = 0, FACE_LEFT = 1, FACE_RIGHT = 2, FACE_UP = 3 };

struct Actor {
    int   x = 0, y = 0;          // tile saat ini (asal, bila sedang bergerak)
    int   dx = 0, dy = 0;        // arah langkah yang sedang berjalan
    int   facing = FACE_DOWN;
    bool  moving = false;
    float progress = 0.0f;       // 0..1 menuju tile berikutnya
    float speed = 4.0f;          // tile per detik (jalan)
    float animTime = 0.0f;       // untuk frame animasi jalan

    // Posisi tampil dalam satuan tile (pecahan saat bergerak).
    float drawX() const { return x + dx * progress; }
    float drawY() const { return y + dy * progress; }
    int   walkFrame() const;     // 0..3
    // Tile tujuan (sama dengan x,y bila diam).
    int   destX() const { return moving ? x + dx : x; }
    int   destY() const { return moving ? y + dy : y; }
};

struct FieldEnemy {
    int         index = 0;        // urutan objek musuh di peta (untuk status kalah)
    int         troop = 0;
    std::string walker;           // sprite peta
    Actor       actor;
    int         homeX = 0, homeY = 0;
    bool        alive = true;
    bool        alerted = false;
    float       alertFlash = 0.0f;  // > 0: tampilkan "!"
    float       think = 0.0f;       // jeda sebelum langkah acak berikutnya
    float       cooldown = 0.0f;    // > 0: tidak mengejar (setelah pemain kabur)
};

// Kejadian yang dihasilkan update() untuk ditangani lapisan atas.
struct FieldEvent {
    enum Kind { None, Transfer, LockedDoor, Encounter, Touch, Arrived } kind = None;
    TravelResult travel;
    const MapObject* object = nullptr;
    // Encounter
    int troop = 0;
    int enemyIndex = -1;          // indeks di Field::enemy()
    int initiative = 0;           // 0 normal, 1 pemain duluan, 2 disergap
};

class Field {
public:
    static constexpr int MAX_ENEMIES = 32;

    explicit Field(World& world) : world_(world) {}

    // Masuk ke peta `mapId` di tile (x,y).
    bool enter(int mapId, int x, int y, int facing);
    // Masuk ke peta lewat nama objek spawn.
    bool enterAtSpawn(int mapId, const std::string& spawnName);

    // inX/inY ∈ {-1,0,1}. run = tombol lari. dt detik.
    FieldEvent update(float dt, int inX, int inY, bool run);

    // Setelah battle: menang → musuh hilang (tercatat di World sampai
    // istirahat); kabur → musuh diam sebentar.
    void resolveEncounter(int enemyIndex, bool won);
    // Muat ulang musuh peta ini (dipakai setelah istirahat di Tungku).
    void respawnEnemies();

    int mapId() const { return mapId_; }
    TileMap* map() const { return map_; }
    const Actor& player() const { return player_; }
    Actor& player() { return player_; }
    World& world() { return world_; }
    const World& world() const { return world_; }

    int enemyCount() const { return enemyCount_; }
    const FieldEnemy& enemy(int i) const { return enemies_[i]; }
    // Indeks musuh hidup yang menempati / menuju tile (x,y), atau -1.
    int enemyAt(int x, int y) const;

    // Bisakah pemain berpindah dari (x,y) sejauh (sx,sy)? (tembok saja)
    bool canStep(int x, int y, int sx, int sy) const;

private:
    World&     world_;
    int        mapId_ = 0;
    TileMap*   map_ = nullptr;
    Actor      player_;
    bool       running_ = false;
    FieldEnemy enemies_[MAX_ENEMIES];
    int        enemyCount_ = 0;
    Rng        rng_{0xC0FFEEu};

    FieldEvent arrive();
    void loadEnemies();
    bool tryPlayerStep(int sx, int sy, float leftover, FieldEvent& ev);
    bool enemyCanStep(int self, int sx, int sy) const;
    void updateEnemy(int i, float dt, FieldEvent& ev);
    void startEnemyStep(int i, int sx, int sy, FieldEvent& ev);
};
