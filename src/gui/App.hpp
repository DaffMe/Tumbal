#pragma once
// ============================================================================
// App — jendela raylib, resolusi logis 640x360 (diskalakan ke jendela),
// loop utama, dan alur antar-layar (eksplorasi ↔ battle). Logika permainan
// ada di src/core; App hanya merangkai dan meneruskan input.
// ============================================================================

#include <string>
#include "raylib.h"
#include "BattleView.hpp"
#include "MapView.hpp"
#include "core/battle/BattleSystem.hpp"
#include "core/world/Field.hpp"

class App {
public:
    int run(bool smoke, bool shots);

    static constexpr int VIEW_W = 640;
    static constexpr int VIEW_H = 360;

private:
    enum Mode { ModeField, ModeBattleIntro, ModeBattle };

    World   world_;
    Field*  field_ = nullptr;
    MapView mapView_;
    RenderTexture2D screen_{};
    double  time_ = 0.0;
    Mode    mode_ = ModeField;

    // pemain (sementara di sini; nanti pindah ke GameState core)
    Battler hero_;
    int     sukma_ = 0;
    int     lastTungkuMap_ = 0;
    int     lastTungkuX_ = 0, lastTungkuY_ = 0;

    // battle
    BattleSystem battle_;
    BattleView   battleView_;
    FieldEvent   encounter_;
    float        introTime_ = 0.0f;
    unsigned     battleSeed_ = 1;

    // transisi pindah peta
    float        fade_ = 1.0f;          // 1 = hitam penuh
    int          fadeDir_ = -1;         // -1 memudar masuk, +1 memudar keluar
    TravelResult pending_;
    bool         hasPending_ = false;
    bool         pendingRespawn_ = false;

    // spanduk nama area & pesan singkat
    std::string bannerText_;
    float       bannerTime_ = 0.0f;
    std::string toast_;
    float       toastTime_ = 0.0f;

    bool showInspector_ = false;
    bool showCollision_ = false;

    bool boot(std::string& error);
    void update(float dt);
    void updateField(float dt);
    void startBattle(const FieldEvent& ev);
    void finishBattle();
    void restAtTungku(const MapObject& t);
    void toast(const std::string& s, float secs = 2.5f);
    void drawFrame();
    void drawHud();
    void drawInspector();
    void enterMapBanner();
    void present();
};
