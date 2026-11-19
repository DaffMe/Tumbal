#pragma once
// ============================================================================
// TileMap — satu peta hasil editor Tiled (.tmj, JSON).
//
// Isi peta disimpan sebagai linked list sederhana (bukan std::vector):
//   - TileLayer : layer tile (ground, wall, deco, overhead, collision)
//   - Tileset   : gambar tileset + animasi tile
//   - MapObject : objek di layer objek (spawn, pintu, tungku, loot, musuh)
// Data tile satu layer = array int mentah (new[]), ukuran width*height.
//
// Konvensi layer:
//   - nama "collision" → tidak digambar; nilai != 0 berarti tile terhalang.
//   - property bool "above" = true → digambar di atas karakter (atap, dahan).
// ============================================================================

#include <string>

struct MapProperty {
    std::string  name;
    std::string  value;     // semua nilai disimpan sebagai teks
    MapProperty* next = nullptr;
};

// Linked list property + pencarian. Dipakai peta dan objek.
struct PropertyList {
    MapProperty* head = nullptr;
    ~PropertyList();
    void        add(const std::string& name, const std::string& value);
    const MapProperty* find(const std::string& name) const;
    std::string getString(const std::string& name, const std::string& def = "") const;
    int         getInt(const std::string& name, int def = 0) const;
    bool        getBool(const std::string& name, bool def = false) const;
};

struct TileLayer {
    std::string name;
    int         width = 0, height = 0;
    int*        data = nullptr;   // gid per sel (0 = kosong)
    bool        visible = true;
    bool        above = false;    // digambar di atas karakter
    float       opacity = 1.0f;
    TileLayer*  next = nullptr;
    ~TileLayer() { delete[] data; }
    int at(int x, int y) const { return (x < 0 || y < 0 || x >= width || y >= height) ? 0 : data[y * width + x]; }
};

struct TileAnimFrame {
    int            tileId;      // id lokal di tileset
    int            durationMs;
    TileAnimFrame* next = nullptr;
};

struct TileAnim {
    int            tileId;      // tile yang dianimasikan (id lokal)
    int            totalMs = 0;
    TileAnimFrame* frames = nullptr;
    TileAnim*      next = nullptr;
    ~TileAnim();
};

struct Tileset {
    std::string name;
    std::string image;          // path relatif dari folder assets/
    int firstGid = 1;
    int tileW = 32, tileH = 32;
    int columns = 1, tileCount = 0;
    int imageW = 0, imageH = 0;
    TileAnim* anims = nullptr;
    Tileset*  next = nullptr;
    ~Tileset();
    // id lokal yang tampil saat waktu `ms` (memperhitungkan animasi).
    int animatedTile(int localId, long long ms) const;
};

struct MapObject {
    int          id = 0;
    std::string  name;
    std::string  type;          // "spawn", "door", "tungku", "loot", "enemy", "npc", ...
    float        x = 0, y = 0, w = 0, h = 0;   // piksel
    PropertyList props;
    MapObject*   next = nullptr;
    int tileX(int tw) const { return static_cast<int>(x) / tw; }
    int tileY(int th) const { return static_cast<int>(y) / th; }
};

class TileMap {
public:
    TileMap() = default;
    ~TileMap();
    TileMap(const TileMap&) = delete;
    TileMap& operator=(const TileMap&) = delete;

    // Muat berkas .tmj. Path gambar tileset dibuat relatif terhadap folder peta.
    bool load(const std::string& path, std::string* error = nullptr);

    int width() const { return width_; }
    int height() const { return height_; }
    int tileW() const { return tileW_; }
    int tileH() const { return tileH_; }
    const std::string& path() const { return path_; }

    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }
    // Tile terhalang oleh layer collision (atau di luar peta).
    bool blocked(int x, int y) const;

    const TileLayer* layers() const { return layers_; }
    const Tileset*   tilesets() const { return tilesets_; }
    const MapObject* objects() const { return objects_; }
    const PropertyList& props() const { return props_; }

    // Tileset pemilik gid (gid sudah tanpa bit flip).
    const Tileset* tilesetFor(int gid) const;
    // Objek pertama bertipe `type` di tile (x,y), atau nullptr.
    const MapObject* objectAt(int x, int y, const char* type = nullptr) const;
    // Urutan objek di antara objek bertipe sama (untuk status per-objek).
    int typeIndex(const MapObject* o) const;
    // Objek pertama dengan nama tertentu.
    const MapObject* findObject(const std::string& name) const;

    static constexpr unsigned GID_MASK = 0x1FFFFFFFu;   // buang bit flip Tiled

private:
    std::string  path_;
    int          width_ = 0, height_ = 0, tileW_ = 32, tileH_ = 32;
    TileLayer*   layers_ = nullptr;
    Tileset*     tilesets_ = nullptr;
    MapObject*   objects_ = nullptr;
    int*         collision_ = nullptr;
    PropertyList props_;

    void clear();
};
