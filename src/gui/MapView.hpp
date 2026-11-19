#pragma once
// ============================================================================
// MapView — menggambar Field (peta Tiled + karakter + objek) ke layar.
// Hanya membaca state core; tidak mengubah aturan permainan.
//
// Urutan gambar: layer bawah → objek & karakter (diurut y) → layer "above"
// → lightmap (kegelapan + cahaya) → efek.
// ============================================================================

#include "raylib.h"
#include "core/game/GameState.hpp"
#include "core/world/Field.hpp"

class MapView {
public:
    void init(int viewW, int viewH);
    void shutdown();

    // Kamera mengikuti pemain, dijepit ke batas peta.
    void updateCamera(const Field& field, float dt);
    void snapCamera(const Field& field);
    // Siapkan lightmap (kegelapan + cahaya). Panggil SEBELUM BeginTextureMode
    // layar utama: raylib tidak mendukung render-texture bersarang.
    void prepareLighting(const Field& field, double time, const Bloodstain* blood = nullptr);
    void draw(const Field& field, double time, bool showCollision, const Bloodstain* blood = nullptr);

    Vector2 camera() const { return cam_; }

private:
    int viewW_ = 640, viewH_ = 360;
    Vector2 cam_{0, 0};
    RenderTexture2D lightmap_{};

    Vector2 targetFor(const Field& field) const;
    void drawLayers(const TileMap& map, bool above, long long ms);
    void drawActor(const Actor& a, const char* sheet, float alpha = 1.0f);
    void drawObjects(const Field& field, double time, const Bloodstain* blood);
    void drawLighting(const Field& field, double time);
    bool lightReady_ = false;
};
