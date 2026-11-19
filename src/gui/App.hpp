#pragma once
// ============================================================================
// App — jendela raylib, resolusi logis 640x360 (diskalakan bulat ke jendela),
// loop utama, dan alur antar-layar. Logika permainan ada di src/core.
// ============================================================================

#include <string>
#include "raylib.h"
#include "core/world/Field.hpp"
#include "MapView.hpp"

class App {
public:
    int run(bool smoke, bool shots);

    static constexpr int VIEW_W = 640;
    static constexpr int VIEW_H = 360;

private:
    World   world_;
    Field*  field_ = nullptr;
    MapView mapView_;
    RenderTexture2D screen_{};
    double  time_ = 0.0;

    // transisi pindah peta
    float        fade_ = 1.0f;          // 1 = hitam penuh
    int          fadeDir_ = -1;         // -1 memudar masuk, +1 memudar keluar
    TravelResult pending_;
    bool         hasPending_ = false;

    // spanduk nama area
    std::string bannerText_;
    float       bannerTime_ = 0.0f;
    std::string toast_;
    float       toastTime_ = 0.0f;

    bool showInspector_ = false;
    bool showCollision_ = false;

    bool boot(std::string& error);
    void update(float dt);
    void drawFrame();
    void drawHud();
    void drawInspector();
    void enterMapBanner();
    void present();
};
