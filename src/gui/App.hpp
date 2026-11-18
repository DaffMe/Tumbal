#pragma once
// ============================================================================
// App — loop utama GUI dan perpindahan scene:
//   Title → Map ⇄ Battle → (Death) → Map ... → Won
// App memegang objek core::Run; scene hanya menampilkan & meneruskan input.
// ============================================================================

#include <cstdint>
#include <memory>
#include "core/run/Run.hpp"
#include "BattleView.hpp"
#include "DsLog.hpp"
#include "MapView.hpp"

class App {
public:
    // smoke: buka sebentar lalu keluar. shots: ambil screenshot otomatis ke folder `shotDir`.
    int run(bool smoke, bool shots);

private:
    enum class Scene { Title, Map, Battle, Death, Won };

    Scene                scene_ = Scene::Title;
    std::unique_ptr<Run> run_;
    BattleView           battleView_;
    MapView              mapView_;
    DsLog                dsLog_;
    bool                 inspector_ = false;
    float                inspSlide_ = 0;
    float                sceneT_ = 0;
    float                fade_ = 1;
    int                  deathsSeen_ = 0;
    bool                 quit_ = false;

    void newRun(std::uint32_t seed);
    void go(Scene s);
    void update(float dt);
    void draw();
    void drawTitle();
    void drawDeath();
    void drawWon();
    void autoShots(int frame);
};
