#pragma once
// ============================================================================
// App — jendela raylib, resolusi logis 640x360 (diskalakan ke jendela),
// loop utama, dan perpindahan tampilan (peta, dialog, menu, battle).
// SEMUA aturan permainan ada di core (kelas Game); App hanya meneruskan
// input dan menggambar state.
// ============================================================================

#include <string>
#include "raylib.h"
#include "BattleView.hpp"
#include "DialogView.hpp"
#include "MapView.hpp"
#include "MenuView.hpp"
#include "TungkuView.hpp"
#include "core/game/Game.hpp"

class App {
public:
    int run(bool smoke, bool shots);

    static constexpr int VIEW_W = 640;
    static constexpr int VIEW_H = 360;

private:
    enum Mode { ModeField, ModeBattleIntro, ModeBattle };

    Game       game_;
    MapView    mapView_;
    DialogView dialog_;
    MenuView   menu_;
    TungkuView tungku_;
    BattleSystem battle_;
    BattleView battleView_;
    RenderTexture2D screen_{};
    double     time_ = 0.0;
    Mode       mode_ = ModeField;
    float      introTime_ = 0.0f;
    unsigned   battleSeed_ = 1;

    // transisi layar (pindah peta / bangkit)
    float fade_ = 1.0f;      // 1 = hitam penuh
    int   fadeDir_ = -1;     // -1 memudar masuk, +1 memudar keluar

    // spanduk nama area & notifikasi
    std::string bannerText_;
    float       bannerTime_ = 0.0f;
    std::string toast_;
    float       toastTime_ = 0.0f;
    int         lastMap_ = -1;

    bool showInspector_ = false;
    bool showCollision_ = false;

    bool boot(std::string& error);
    void update(float dt);
    void updateField(float dt);
    void drawFrame();
    void drawHud();
    void drawInspector();
    void present();
};
