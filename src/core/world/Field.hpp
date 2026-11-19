#pragma once
// ============================================================================
// Field — keadaan eksplorasi di peta yang sedang aktif.
//
// Gerak berbasis grid seperti RPG Maker: karakter selalu berdiri di satu
// tile, berpindah ke tile tetangga dengan interpolasi halus. Delapan arah
// (seperti skrip gerak 8 arah Black Souls); gerak diagonal hanya boleh jika
// kedua tile ortogonal juga bisa dilewati (tidak memotong sudut tembok).
//
// GUI hanya membaca posisi (x, y, offset) dan memanggil update() dengan
// input arah; semua aturan gerak dan pemicu pintu ada di sini.
// ============================================================================

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
};

// Kejadian yang dihasilkan update() untuk ditangani lapisan atas.
struct FieldEvent {
    enum Kind { None, Transfer, LockedDoor, Arrived } kind = None;
    TravelResult travel;
    const MapObject* object = nullptr;
};

class Field {
public:
    explicit Field(World& world) : world_(world) {}

    // Masuk ke peta `mapId` di tile (x,y).
    bool enter(int mapId, int x, int y, int facing);
    // Masuk ke peta lewat nama objek spawn.
    bool enterAtSpawn(int mapId, const std::string& spawnName);

    // inX/inY ∈ {-1,0,1}. run = tombol lari. dt detik.
    FieldEvent update(float dt, int inX, int inY, bool run);

    int mapId() const { return mapId_; }
    TileMap* map() const { return map_; }
    const Actor& player() const { return player_; }
    Actor& player() { return player_; }
    World& world() { return world_; }
    const World& world() const { return world_; }

    // Bisakah actor berpindah dari (x,y) sejauh (sx,sy)?
    bool canStep(int x, int y, int sx, int sy) const;

private:
    World&   world_;
    int      mapId_ = 0;
    TileMap* map_ = nullptr;
    Actor    player_;

    FieldEvent arrive();
};
