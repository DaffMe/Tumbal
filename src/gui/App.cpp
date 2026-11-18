#include "App.hpp"

#include <cmath>
#include <ctime>
#include "Art.hpp"
#include "Gfx.hpp"
#include "Inspector.hpp"
#include "Sfx.hpp"
#include "core/battle/Encounters.hpp"

using namespace gfx;

// ============================================================================

int App::run(bool smoke, bool shots) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(VW, VH, "Cinderwake");
    SetWindowMinSize(640, 360);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    gfx::init();
    sfx::init();

    int frame = 0;
    while (!WindowShouldClose() && !quit_) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        ++frame;
        if (smoke && frame > 90) break;
        if (shots) autoShots(frame);

        update(dt);
        draw();
    }

    battleView_ = BattleView();
    run_.reset();
    sfx::shutdown();
    gfx::shutdown();
    CloseWindow();
    return 0;
}

void App::newRun(std::uint32_t seed) {
    run_.reset(new Run(seed));
    mapView_.reset();
    dsLog_ = DsLog();
    deathsSeen_ = 0;
    go(Scene::Map);
}

void App::go(Scene s) {
    scene_ = s;
    sceneT_ = 0;
    fade_ = 1;
}

// ============================================================================

void App::update(float dt) {
    sceneT_ += dt;
    fade_ = std::fmax(0.0f, fade_ - dt * 2.5f);
    updateEffects(dt);

    if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
    if (IsKeyPressed(KEY_M)) sfx::setMuted(!sfx::muted());
    sfx::updateAmbience(true, scene_ == Scene::Battle ? 1.0f : 0.7f);
    if (IsKeyPressed(KEY_TAB) && (scene_ == Scene::Map || scene_ == Scene::Battle)) inspector_ = !inspector_;
    inspSlide_ = approach(inspSlide_, inspector_ ? 1.0f : 0.0f, 12.0f, dt);

    switch (scene_) {
        case Scene::Title:
            break;   // input di drawTitle (tombol immediate-mode)

        case Scene::Map:
            if (run_->state() == RunState::InBattle) {
                const Room* r = run_->floor().room(run_->floor().currentRoom());
                std::string title = std::string(roomTypeName(r->type)) + "  ·  " + encounterName(r->encounter);
                battleView_.begin(run_->battle(), run_->floor().number(), title, &dsLog_);
                go(Scene::Battle);
                break;
            }
            if (run_->state() == RunState::RunWon) {
                go(Scene::Won);
                break;
            }
            mapView_.update(*run_, dt, dsLog_);
            break;

        case Scene::Battle:
            battleView_.update(dt);
            if (battleView_.done()) {
                run_->finishBattle();
                if (run_->deaths() > deathsSeen_) {
                    deathsSeen_ = run_->deaths();
                    go(Scene::Death);
                } else {
                    go(Scene::Map);
                }
            }
            break;

        case Scene::Death:
            if (sceneT_ > 1.2f && (clicked() || IsKeyPressed(KEY_ENTER) || sceneT_ > 6.0f)) go(Scene::Map);
            break;

        case Scene::Won:
            break;
    }
}

// ============================================================================

void App::draw() {
    beginFrame();
    Post post;
    switch (scene_) {
        case Scene::Title:
            drawTitle();
            post.tint = Color{255, 225, 200, 255};
            break;
        case Scene::Map:
            mapView_.draw(*run_);
            if (inspSlide_ > 0.01f) inspector::drawMap(*run_, dsLog_, inspSlide_);
            post = mapView_.post(*run_);
            post.panelX = VW - 470.0f * inspSlide_;
            break;
        case Scene::Battle:
            battleView_.draw();
            if (inspSlide_ > 0.01f) inspector::drawBattle(*battleView_.battle(), dsLog_, inspSlide_);
            post.tint = run_->floor().number() == 2 ? Color{215, 225, 255, 255} : Color{255, 228, 205, 255};
            break;
        case Scene::Death:
            mapView_.draw(*run_);
            drawDeath();
            post = mapView_.post(*run_);
            break;
        case Scene::Won:
            drawWon();
            break;
    }
    if (fade_ > 0) DrawRectangle(0, 0, VW, VH, fade(BLACK, fade_));
    endFrame(post);
}

void App::drawTitle() {
    float t = gfx::time();
    art::drawTitleBackground(t);
    ambientEmbers(14.0f);
    drawParticles();

    float a = std::fmin(1.0f, sceneT_ * 0.8f);
    glow({VW / 2.0f, 250}, 340, fade(col::ember, 0.18f * a));
    textShadow(title(), "CINDERWAKE", VW / 2.0f, 180, 112, fade(col::ink, a), Center);
    text(head(), "turunilah katedral, padamkan nyala terakhir", VW / 2.0f, 304, 22, fade(col::inkDim, a), Center);

    if (sceneT_ > 0.6f) {
        if (button({VW / 2.0f - 140, 400, 280, 52}, "Ekspedisi Baru", true, false, 24) || IsKeyPressed(KEY_ENTER))
            newRun(static_cast<std::uint32_t>(std::time(nullptr)));
        if (button({VW / 2.0f - 140, 466, 280, 44}, "Keluar", true, false, 20)) quit_ = true;
    }
    text(body(), "Tugas Besar Struktur Data  ·  C++ & raylib  ·  Tab di dalam game: Inspector struktur data",
         VW / 2.0f, VH - 34, 15, col::inkFaint, Center);
}

void App::drawDeath() {
    float a = std::fmin(1.0f, sceneT_ * 1.2f);
    DrawRectangle(0, 0, VW, VH, fade(BLACK, 0.7f * a));
    DrawRectangleGradientH(0, 250, VW / 2, 170, fade(BLACK, 0), fade(Color{40, 0, 0, 255}, 0.6f * a));
    DrawRectangleGradientH(VW / 2, 250, VW / 2, 170, fade(Color{40, 0, 0, 255}, 0.6f * a), fade(BLACK, 0));
    float s = 1.0f + 0.05f * std::fmin(1.0f, sceneT_);
    textShadow(title(), "NYALA PADAM", VW / 2.0f, 262, 92 * s, fade(col::blood, a), Center);
    text(body(), std::to_string(run_->droppedAsh()) + " Ash tertinggal di ruang " + std::to_string(run_->droppedAshRoom()) +
                     ". Kalian bangkit kembali di Ember Shrine.",
         VW / 2.0f, 372, 20, fade(col::inkDim, a), Center);
    if (sceneT_ > 1.2f) text(body(), "klik untuk melanjutkan", VW / 2.0f, 410, 16, fade(col::inkFaint, a), Center);
}

void App::drawWon() {
    float t = gfx::time();
    art::drawTitleBackground(t);
    ambientEmbers(20.0f);
    drawParticles();
    float a = std::fmin(1.0f, sceneT_ * 0.6f);
    glow({VW / 2.0f, 260}, 380, fade(col::gold, 0.2f * a));
    textShadow(title(), "NYALA TERAKHIR PADAM", VW / 2.0f, 200, 72, fade(col::gold, a), Center);
    text(head(), "Ekspedisi selesai.", VW / 2.0f, 296, 26, fade(col::ink, a), Center);
    text(body(), "Kematian: " + std::to_string(run_->deaths()) + "   ·   Ash: " + std::to_string(run_->ash()),
         VW / 2.0f, 340, 20, fade(col::inkDim, a), Center);
    if (sceneT_ > 1.5f && button({VW / 2.0f - 120, 420, 240, 50}, "Kembali ke Judul")) go(Scene::Title);
}

// ============================================================================
// Mode screenshot otomatis (verifikasi tampilan tanpa input manusia)
// ============================================================================

void App::autoShots(int frame) {
    auto battleRoomNear = [&]() {
        int found = -1;
        Floor& f = run_->floor();
        f.graph().forEachNeighbor(f.currentRoom(), [&](Floor::Vertex* v, int) {
            if (found < 0 && v->data.type == RoomType::Battle) found = v->id;
        });
        if (found < 0) f.graph().forEachNeighbor(f.currentRoom(), [&](Floor::Vertex* v, int) { if (found < 0) found = v->id; });
        return found;
    };
    switch (frame) {
        case 80:  TakeScreenshot("shot_1_title.png"); newRun(4242); break;
        case 170: TakeScreenshot("shot_2_map.png"); inspector_ = true; break;
        case 220: TakeScreenshot("shot_3_map_inspector.png"); inspector_ = false; run_->travelTo(battleRoomNear()); break;
        case 400: TakeScreenshot("shot_4_battle.png"); break;
        case 401:
            if (scene_ == Scene::Battle && run_->battle() && run_->battle()->state() == BattleState::WaitingForPlayer) {
                Battle* b = run_->battle();
                for (int i = 0; i < b->unitCount(); ++i)
                    if (b->unit(i).side == Side::Enemy && b->unit(i).alive) { b->playerUseSkill(0, i); break; }
                battleView_.refresh();
            }
            break;
        case 418: TakeScreenshot("shot_5_battle_action.png"); inspector_ = true; break;
        case 470: TakeScreenshot("shot_6_battle_inspector.png"); quit_ = true; break;
        default: break;
    }
}
