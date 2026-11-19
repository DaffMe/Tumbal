#include "App.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include "Assets.hpp"
#include "core/util/DsTrace.hpp"

bool App::boot(std::string& error) {
    std::string worldPath = Assets::get().path("assets/maps/world.json");
    if (!world_.load(worldPath, &error)) return false;
    field_ = new Field(world_);
    if (!field_->enterAtSpawn(world_.startMap(), world_.startSpawn())) {
        error = "peta awal tidak ditemukan";
        return false;
    }
    mapView_.snapCamera(*field_);
    enterMapBanner();
    return true;
}

void App::enterMapBanner() {
    const MapInfo* mi = world_.info(field_->mapId());
    bannerText_ = mi ? mi->name : "";
    bannerTime_ = 3.0f;
}

void App::update(float dt) {
    time_ += dt;
    if (bannerTime_ > 0) bannerTime_ -= dt;
    if (toastTime_ > 0) toastTime_ -= dt;
    if (IsKeyPressed(KEY_F1)) showInspector_ = !showInspector_;
    if (IsKeyPressed(KEY_F3)) showCollision_ = !showCollision_;

    // transisi layar
    if (fadeDir_ != 0) {
        fade_ += fadeDir_ * dt * 3.0f;
        if (fadeDir_ > 0 && fade_ >= 1.0f) {
            fade_ = 1.0f;
            if (hasPending_) {
                field_->enter(pending_.toMap, pending_.x, pending_.y, pending_.dir);
                mapView_.snapCamera(*field_);
                hasPending_ = false;
                enterMapBanner();
            }
            fadeDir_ = -1;
        } else if (fadeDir_ < 0 && fade_ <= 0.0f) {
            fade_ = 0.0f;
            fadeDir_ = 0;
        }
        if (fadeDir_ > 0) return;   // saat memudar keluar, input dibekukan
    }

    int ix = 0, iy = 0;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) ix -= 1;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) ix += 1;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) iy -= 1;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) iy += 1;
    bool run = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    FieldEvent ev = field_->update(dt, ix, iy, run);
    if (ev.kind == FieldEvent::Transfer) {
        pending_ = ev.travel;
        hasPending_ = true;
        fadeDir_ = 1;
    } else if (ev.kind == FieldEvent::LockedDoor && toastTime_ <= 0) {
        toast_ = "Terkunci dari sisi lain.";
        toastTime_ = 2.0f;
    }
    mapView_.updateCamera(*field_, dt);
}

static void drawTextShadow(const char* t, int x, int y, int size, Color c) {
    DrawText(t, x + 1, y + 1, size, Fade(BLACK, 0.8f));
    DrawText(t, x, y, size, c);
}

void App::drawHud() {
    if (bannerTime_ > 0) {
        float a = std::fmin(1.0f, bannerTime_ / 0.6f) * std::fmin(1.0f, (3.0f - bannerTime_) / 0.4f);
        int w = MeasureText(bannerText_.c_str(), 20);
        int x = VIEW_W / 2 - w / 2, y = 34;
        DrawRectangleGradientH(x - 80, y - 6, w / 2 + 80, 32, Fade(BLACK, 0.0f), Fade(BLACK, 0.6f * a));
        DrawRectangleGradientH(x + w / 2, y - 6, w / 2 + 80, 32, Fade(BLACK, 0.6f * a), Fade(BLACK, 0.0f));
        DrawLine(x - 40, y + 27, x + w + 40, y + 27, Fade(Color{200, 170, 120, 255}, 0.7f * a));
        drawTextShadow(bannerText_.c_str(), x, y, 20, Fade(Color{235, 220, 190, 255}, a));
    }
    if (toastTime_ > 0) {
        int w = MeasureText(toast_.c_str(), 10);
        DrawRectangle(VIEW_W / 2 - w / 2 - 8, VIEW_H - 46, w + 16, 20, Fade(BLACK, 0.7f));
        drawTextShadow(toast_.c_str(), VIEW_W / 2 - w / 2, VIEW_H - 41, 10, RAYWHITE);
    }
}

void App::drawInspector() {
    // Panel struktur data: Graph dunia + operasi terbaru
    const int x = 6, w = 300;
    DrawRectangle(x, 6, w, VIEW_H - 12, Fade(Color{10, 8, 14, 255}, 0.85f));
    DrawRectangleLines(x, 6, w, VIEW_H - 12, Color{120, 100, 80, 255});
    int y = 12;
    drawTextShadow("INSPECTOR  (F1)", x + 8, y, 10, Color{255, 210, 140, 255});
    y += 16;
    drawTextShadow("Graph dunia (vertex = peta, edge = jalan):", x + 8, y, 10, LIGHTGRAY);
    y += 14;
    world_.graph().forEachVertex([&](World::MapGraph::Vertex* v) {
        char buf[160];
        std::snprintf(buf, sizeof buf, "%s[%d] %s%s%s", v->id == field_->mapId() ? "> " : "  ", v->id,
                      v->data.name.c_str(), v->data.visited ? "" : " (?)", v->data.hasTungku ? "  *Tungku" : "");
        drawTextShadow(buf, x + 8, y, 10, v->id == field_->mapId() ? YELLOW : RAYWHITE);
        y += 12;
        for (auto* e = v->edges; e != nullptr; e = e->next) {
            std::snprintf(buf, sizeof buf, "      -> [%d] %s", e->to->id, e->open ? "terbuka" : "TERKUNCI");
            drawTextShadow(buf, x + 8, y, 10, e->open ? Color{150, 220, 150, 255} : Color{230, 120, 120, 255});
            y += 12;
        }
    });
    y += 6;
    drawTextShadow("Operasi struktur data terbaru:", x + 8, y, 10, LIGHTGRAY);
    y += 14;
    const DsTrace& tr = DsTrace::instance();
    for (int i = 0; i < tr.count() && y < VIEW_H - 20; ++i) {
        drawTextShadow(tr.recent(i).c_str(), x + 8, y, 10, Fade(Color{190, 210, 255, 255}, 1.0f - i * 0.06f));
        y += 12;
    }
}

void App::drawFrame() {
    mapView_.prepareLighting(*field_, time_);
    BeginTextureMode(screen_);
    ClearBackground(BLACK);
    mapView_.draw(*field_, time_, showCollision_);
    drawHud();
    if (showInspector_) drawInspector();
    if (fade_ > 0.0f) DrawRectangle(0, 0, VIEW_W, VIEW_H, Fade(BLACK, fade_));
    EndTextureMode();
}

void App::present() {
    BeginDrawing();
    ClearBackground(BLACK);
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int scale = std::max(1, std::min(sw / VIEW_W, sh / VIEW_H));
    float fw = static_cast<float>(VIEW_W * scale), fh = static_cast<float>(VIEW_H * scale);
    // bila jendela tidak kelipatan bulat, skala pecahan agar tetap penuh
    float fs = std::fmin(static_cast<float>(sw) / VIEW_W, static_cast<float>(sh) / VIEW_H);
    if (fs - scale > 0.25f) { fw = VIEW_W * fs; fh = VIEW_H * fs; }
    Rectangle src{0, 0, static_cast<float>(VIEW_W), -static_cast<float>(VIEW_H)};
    Rectangle dst{(sw - fw) / 2.0f, (sh - fh) / 2.0f, fw, fh};
    DrawTexturePro(screen_.texture, src, dst, {0, 0}, 0, WHITE);
    EndDrawing();
}

int App::run(bool smoke, bool shots) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VIEW_W * 2, VIEW_H * 2, "TUMBAL");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    screen_ = LoadRenderTexture(VIEW_W, VIEW_H);
    mapView_.init(VIEW_W, VIEW_H);

    std::string err;
    if (!boot(err)) {
        std::fprintf(stderr, "Gagal memuat dunia: %s\n", err.c_str());
        while (!WindowShouldClose() && !smoke) {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText(("Gagal memuat dunia: " + err).c_str(), 20, 20, 20, RED);
            EndDrawing();
        }
        CloseWindow();
        return 1;
    }

    int frame = 0;
    while (!WindowShouldClose()) {
        float dt = std::fmin(GetFrameTime(), 1.0f / 20.0f);
        if (shots) dt = 1.0f / 60.0f;
        update(dt);
        drawFrame();
        present();
        ++frame;
        if (shots) {
            // skenario screenshot: diam di peta awal, lalu pindah ke peta 2
            if (frame == 90) { showInspector_ = false; TakeScreenshot("shot_map1.png"); }
            if (frame == 100) {
                field_->enterAtSpawn(2, "dari_sel");
                mapView_.snapCamera(*field_);
                enterMapBanner();
            }
            if (frame == 160) TakeScreenshot("shot_map2.png");
            if (frame == 165) {
                // dekat Tungku di aula bawah lorong
                if (const MapObject* t = field_->map()->findObject("tungku_lumbung"))
                    field_->enter(2, t->tileX(32) - 2, t->tileY(32), FACE_RIGHT);
                mapView_.snapCamera(*field_);
            }
            if (frame == 200) TakeScreenshot("shot_tungku.png");
            if (frame == 210) showInspector_ = true;
            if (frame == 220) { TakeScreenshot("shot_inspector.png"); break; }
        }
        if (smoke && frame > 30) break;
    }
    delete field_;
    field_ = nullptr;
    mapView_.shutdown();
    UnloadRenderTexture(screen_);
    Assets::get().unloadAll();
    CloseWindow();
    return 0;
}
