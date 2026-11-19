#include "App.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include "Assets.hpp"
#include "Ui.hpp"
#include "core/util/DsTrace.hpp"

bool App::boot(std::string& error) {
    std::string worldPath = Assets::get().path("assets/maps/world.json");
    std::string events[] = {Assets::get().path("assets/events/lumbung.json")};
    if (!game_.load(worldPath, events, 1, &error)) return false;
    game_.newGame("Tumbal", KL_PENDEKAR);
    mapView_.snapCamera(game_.field());
    return true;
}

// ------------------------------------------------------------------ update

void App::update(float dt) {
    time_ += dt;
    if (bannerTime_ > 0) bannerTime_ -= dt;
    if (toastTime_ > 0) toastTime_ -= dt;
    if (IsKeyPressed(KEY_F1)) showInspector_ = !showInspector_;
    if (IsKeyPressed(KEY_F3)) showCollision_ = !showCollision_;

    // notifikasi dari core (Queue): tampil satu per satu
    if (toastTime_ <= 0.3f) {
        std::string n;
        if (game_.popNotice(n)) { toast_ = n; toastTime_ = 2.6f; }
    }

    if (mode_ == ModeBattleIntro) {
        introTime_ += dt;
        if (introTime_ >= 0.55f) {
            const TileMap* m = game_.field().map();
            std::string bb = m->props().getString("battleback", "assets/battlebacks/lumbung.png");
            game_.setupBattle(battle_, battleSeed_++ * 2654435761u);
            battleView_.begin(&battle_, bb);
            mode_ = ModeBattle;
        }
        return;
    }
    if (mode_ == ModeBattle) {
        if (battleView_.update(dt)) {
            game_.endBattle(battle_);
            mode_ = ModeField;
            if (game_.transferPending()) { fade_ = 1.0f; fadeDir_ = 1; }
        }
        return;
    }
    updateField(dt);
}

void App::updateField(float dt) {
    // transisi layar
    if (fadeDir_ != 0) {
        fade_ += fadeDir_ * dt * 3.0f;
        if (fadeDir_ > 0 && fade_ >= 1.0f) {
            fade_ = 1.0f;
            game_.finishTransfer();
            mapView_.snapCamera(game_.field());
            fadeDir_ = -1;
        } else if (fadeDir_ < 0 && fade_ <= 0.0f) {
            fade_ = 0.0f;
            fadeDir_ = 0;
        }
        if (fadeDir_ > 0) return;
    }
    if (game_.field().mapId() != lastMap_) {
        lastMap_ = game_.field().mapId();
        const MapInfo* mi = game_.world().info(lastMap_);
        bannerText_ = mi ? mi->name : "";
        bannerTime_ = 3.0f;
    }

    if (menu_.open()) {
        std::string r = menu_.update(game_);
        if (!r.empty()) { toast_ = r; toastTime_ = 2.2f; }
        if (game_.transferPending()) fadeDir_ = 1;
        return;
    }
    if (game_.inDialog()) {
        dialog_.update(game_.interpreter(), dt);
        game_.update(dt, GameInput{});
    } else {
        GameInput in;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) in.dx -= 1;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) in.dx += 1;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) in.dy -= 1;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) in.dy += 1;
        in.run = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        in.action = IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
        bool openMenu = (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE)) && !game_.interpreter().busy() &&
                        !game_.field().player().moving;
        if (openMenu) { menu_.show(); return; }
        game_.update(dt, in);
    }
    if (game_.battlePending() && mode_ == ModeField) {
        mode_ = ModeBattleIntro;
        introTime_ = 0.0f;
    } else if (game_.transferPending() && fadeDir_ == 0) {
        fadeDir_ = 1;
    }
    mapView_.updateCamera(game_.field(), dt);
}

// ------------------------------------------------------------------ draw

void App::drawHud() {
    if (bannerTime_ > 0) {
        float a = std::fmin(1.0f, bannerTime_ / 0.6f) * std::fmin(1.0f, (3.0f - bannerTime_) / 0.4f);
        float w = ui::measure(bannerText_, 32);
        float x = VIEW_W / 2 - w / 2, y = 30;
        DrawRectangleGradientH(static_cast<int>(x - 80), static_cast<int>(y - 4), static_cast<int>(w / 2 + 80), 40,
                               Fade(BLACK, 0.0f), Fade(BLACK, 0.6f * a));
        DrawRectangleGradientH(static_cast<int>(x + w / 2), static_cast<int>(y - 4), static_cast<int>(w / 2 + 80), 40,
                               Fade(BLACK, 0.6f * a), Fade(BLACK, 0.0f));
        DrawLine(static_cast<int>(x - 40), static_cast<int>(y + 36), static_cast<int>(x + w + 40), static_cast<int>(y + 36),
                 Fade(Color{200, 170, 120, 255}, 0.7f * a));
        ui::text(bannerText_, x, y, Fade(ui::BONE, a), 32);
    }
    if (toastTime_ > 0) {
        float a = std::fmin(1.0f, toastTime_ / 0.4f);
        float w = ui::measure(toast_);
        float y = game_.inDialog() ? 12.0f : VIEW_H - 50.0f;
        DrawRectangle(static_cast<int>(VIEW_W / 2 - w / 2 - 12), static_cast<int>(y), static_cast<int>(w + 24), 22, Fade(BLACK, 0.75f * a));
        DrawRectangleLines(static_cast<int>(VIEW_W / 2 - w / 2 - 12), static_cast<int>(y), static_cast<int>(w + 24), 22,
                           Fade(Color{120, 98, 76, 255}, a));
        ui::textCentered(toast_, VIEW_W / 2.0f, y + 3, Fade(ui::BONE, a));
    }
}

void App::drawInspector() {
    // Panel struktur data: Timeline (battle), Stack interpreter, Graph dunia
    const float x = 6, w = 300;
    DrawRectangle(static_cast<int>(x), 6, static_cast<int>(w), VIEW_H - 12, Fade(Color{10, 8, 14, 255}, 0.88f));
    DrawRectangleLines(static_cast<int>(x), 6, static_cast<int>(w), VIEW_H - 12, Color{120, 100, 80, 255});
    float y = 12;
    char buf[160];
    ui::text("INSPECTOR  (F1)", x + 8, y, ui::AMBER);
    y += 20;
    if (mode_ == ModeBattle) {
        ui::text("Timeline (Doubly Linked List), head = giliran:", x + 8, y, LIGHTGRAY, 10);
        y += 12;
        int k = 0;
        battle_.timeline().forEach([&](const Timeline<Battler>::Node& n) {
            std::snprintf(buf, sizeof buf, "%s%s  sisa %d tick%s", k == 0 ? "HEAD " : "  <-> ", n.item->name.c_str(),
                          static_cast<int>(n.av), n.item->chanting ? "  [rapal]" : "");
            ui::text(buf, x + 8, y, k == 0 ? YELLOW : RAYWHITE, 10);
            y += 11;
            ++k;
        });
    } else {
        Interpreter& it = game_.interpreter();
        std::snprintf(buf, sizeof buf, "Interpreter event: Stack kedalaman %d %s", it.depth(), it.busy() ? "(berjalan)" : "(diam)");
        ui::text(buf, x + 8, y, it.busy() ? YELLOW : LIGHTGRAY, 10);
        y += 14;
        ui::text("Graph dunia (vertex = peta, edge = jalan):", x + 8, y, LIGHTGRAY, 10);
        y += 12;
        game_.world().graph().forEachVertex([&](World::MapGraph::Vertex* v) {
            std::snprintf(buf, sizeof buf, "%s[%d] %s%s%s", v->id == game_.field().mapId() ? "> " : "  ", v->id,
                          v->data.name.c_str(), v->data.visited ? "" : " (?)", v->data.hasTungku ? "  *Tungku" : "");
            ui::text(buf, x + 8, y, v->id == game_.field().mapId() ? YELLOW : RAYWHITE, 10);
            y += 11;
            for (auto* e = v->edges; e != nullptr; e = e->next) {
                std::snprintf(buf, sizeof buf, "      -> [%d] %s", e->to->id, e->open ? "terbuka" : "TERKUNCI");
                ui::text(buf, x + 8, y, e->open ? Color{150, 220, 150, 255} : Color{230, 120, 120, 255}, 10);
                y += 11;
            }
        });
    }
    y += 8;
    ui::text("Operasi struktur data terbaru:", x + 8, y, LIGHTGRAY, 10);
    y += 12;
    const DsTrace& tr = DsTrace::instance();
    for (int i = 0; i < tr.count() && y < VIEW_H - 16; ++i) {
        ui::text(tr.recent(i), x + 8, y, Fade(Color{190, 210, 255, 255}, 1.0f - i * 0.05f), 10);
        y += 11;
    }
}

void App::drawFrame() {
    if (mode_ != ModeBattle) mapView_.prepareLighting(game_.field(), time_, &game_.state().bloodstain());
    BeginTextureMode(screen_);
    ClearBackground(BLACK);
    if (mode_ == ModeBattle) {
        battleView_.draw(time_);
    } else {
        mapView_.draw(game_.field(), time_, showCollision_, &game_.state().bloodstain());
        dialog_.draw(game_.interpreter(), time_);
        menu_.draw(game_, time_);
        drawHud();
        if (mode_ == ModeBattleIntro) {
            // transisi masuk battle: kilat putih lalu tirai hitam dari tengah
            float t = introTime_ / 0.55f;
            if (t < 0.35f) DrawRectangle(0, 0, VIEW_W, VIEW_H, Fade(WHITE, 0.7f * (1.0f - t / 0.35f)));
            float h = VIEW_H / 2.0f * std::fmin(1.0f, std::fmax(0.0f, (t - 0.2f) / 0.8f));
            DrawRectangle(0, 0, VIEW_W, static_cast<int>(h), BLACK);
            DrawRectangle(0, static_cast<int>(VIEW_H - h), VIEW_W, static_cast<int>(h) + 1, BLACK);
        }
    }
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
    ui::init();
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
            // skenario screenshot otomatis
            Interpreter& it = game_.interpreter();
            if (frame == 100) TakeScreenshot("shot_dialog.png");          // teks pembuka
            if (frame > 100 && frame < 140 && it.state() == Interpreter::WaitMessage && frame % 4 == 0) it.resume();
            if (frame == 150) {
                // bicara dengan Lelaki Tua
                game_.field().enter(1, 23, 6, FACE_UP);
                mapView_.snapCamera(game_.field());
                it.runEvent("lelaki_tua");
            }
            if (frame == 200 || frame == 205) it.resume();
            if (frame == 240) TakeScreenshot("shot_choice.png");
            if (frame == 250) it.choose(1);
            if (frame > 250 && frame < 300 && it.state() == Interpreter::WaitMessage && frame % 5 == 0) it.resume();
            if (frame == 310) menu_.show();
            if (frame == 330) TakeScreenshot("shot_menu.png");
            if (frame == 335) menu_ = MenuView();
            if (frame == 340) {
                // mati di lorong: ceceran sukma muncul
                game_.state().addSukma(120);
                game_.field().enter(1, 12, 13, FACE_RIGHT);
                game_.state().die(1, 14, 13);
                mapView_.snapCamera(game_.field());
            }
            if (frame == 380) TakeScreenshot("shot_bloodstain.png");
            if (frame >= 382) break;
        }
        if (smoke && frame > 30) break;
    }
    mapView_.shutdown();
    UnloadRenderTexture(screen_);
    ui::shutdown();
    Assets::get().unloadAll();
    CloseWindow();
    return 0;
}
