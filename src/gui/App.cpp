#include "App.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include "Assets.hpp"
#include "Ui.hpp"
#include "core/util/DsTrace.hpp"

bool App::boot(std::string& error) {
    std::string worldPath = Assets::get().path("assets/maps/world.json");
    if (!world_.load(worldPath, &error)) return false;
    field_ = new Field(world_);
    if (!field_->enterAtSpawn(world_.startMap(), world_.startSpawn())) {
        error = "peta awal tidak ditemukan";
        return false;
    }
    hero_ = makeHeroBattler("Tumbal", KL_PENDEKAR, 1);
    lastTungkuMap_ = field_->mapId();
    lastTungkuX_ = field_->player().x;
    lastTungkuY_ = field_->player().y;
    mapView_.snapCamera(*field_);
    enterMapBanner();
    return true;
}

void App::enterMapBanner() {
    const MapInfo* mi = world_.info(field_->mapId());
    bannerText_ = mi ? mi->name : "";
    bannerTime_ = 3.0f;
}

void App::toast(const std::string& s, float secs) {
    toast_ = s;
    toastTime_ = secs;
}

// ------------------------------------------------------------------ update

void App::update(float dt) {
    time_ += dt;
    if (bannerTime_ > 0) bannerTime_ -= dt;
    if (toastTime_ > 0) toastTime_ -= dt;
    if (IsKeyPressed(KEY_F1)) showInspector_ = !showInspector_;
    if (IsKeyPressed(KEY_F3)) showCollision_ = !showCollision_;

    if (mode_ == ModeBattleIntro) {
        introTime_ += dt;
        if (introTime_ >= 0.55f) {
            const MapInfo* mi = world_.info(field_->mapId());
            std::string bb = mi && mi->map ? mi->map->props().getString("battleback", "assets/battlebacks/lumbung.png")
                                           : "assets/battlebacks/lumbung.png";
            battle_.setup(&hero_, 1, encounter_.troop, static_cast<Initiative>(encounter_.initiative), battleSeed_++ * 2654435761u);
            battleView_.begin(&battle_, bb);
            mode_ = ModeBattle;
        }
        return;
    }
    if (mode_ == ModeBattle) {
        if (battleView_.update(dt)) finishBattle();
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
            if (hasPending_) {
                field_->enter(pending_.toMap, pending_.x, pending_.y, pending_.dir);
                if (pendingRespawn_) field_->respawnEnemies();
                pendingRespawn_ = false;
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

    // tombol aksi: periksa objek di depan pemain (Tungku)
    if ((IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) && !field_->player().moving) {
        const Actor& p = field_->player();
        int fx = p.x + (p.facing == FACE_LEFT ? -1 : p.facing == FACE_RIGHT ? 1 : 0);
        int fy = p.y + (p.facing == FACE_UP ? -1 : p.facing == FACE_DOWN ? 1 : 0);
        const MapObject* t = field_->map()->objectAt(fx, fy, "tungku");
        if (t == nullptr) t = field_->map()->objectAt(p.x, p.y, "tungku");
        if (t != nullptr) restAtTungku(*t);
    }

    FieldEvent ev = field_->update(dt, ix, iy, run);
    if (ev.kind == FieldEvent::Transfer) {
        pending_ = ev.travel;
        hasPending_ = true;
        fadeDir_ = 1;
    } else if (ev.kind == FieldEvent::LockedDoor && toastTime_ <= 0) {
        toast("Terkunci dari sisi lain.", 2.0f);
    } else if (ev.kind == FieldEvent::Encounter) {
        startBattle(ev);
    }
    mapView_.updateCamera(*field_, dt);
}

void App::restAtTungku(const MapObject& t) {
    hero_.hp = hero_.stats.mhp;
    hero_.mp = hero_.stats.mmp;
    for (int s = 1; s < ST_COUNT; ++s) hero_.status[s] = 0;
    world_.restAtTungku();
    field_->respawnEnemies();
    lastTungkuMap_ = field_->mapId();
    lastTungkuX_ = t.tileX(32) - 1;
    lastTungkuY_ = t.tileY(32);
    if (field_->map()->blocked(lastTungkuX_, lastTungkuY_)) lastTungkuX_ += 2;
    toast("Kamu beristirahat di Tungku. Musuh bangkit kembali...", 3.0f);
}

void App::startBattle(const FieldEvent& ev) {
    encounter_ = ev;
    mode_ = ModeBattleIntro;
    introTime_ = 0.0f;
}

void App::finishBattle() {
    BattleSystem::Phase r = battle_.phase();
    const Battler& b = battle_.battler(0);
    hero_.hp = b.hp;
    hero_.mp = b.mp;
    for (int s = 1; s < ST_COUNT; ++s) hero_.status[s] = statusDef(s).battleOnly ? 0 : b.status[s];
    mode_ = ModeField;
    if (r == BattleSystem::Victory) {
        sukma_ += battle_.sukmaReward();
        field_->resolveEncounter(encounter_.enemyIndex, true);
        toast("Sukma +" + std::to_string(battle_.sukmaReward()) + "   (dibawa: " + std::to_string(sukma_) + ")");
    } else if (r == BattleSystem::Escaped) {
        field_->resolveEncounter(encounter_.enemyIndex, false);
    } else {
        // mati: sukma tercecer, bangkit di Tungku terakhir, musuh hidup lagi
        sukma_ = 0;
        hero_.hp = hero_.stats.mhp;
        hero_.mp = hero_.stats.mmp;
        for (int s = 1; s < ST_COUNT; ++s) hero_.status[s] = 0;
        world_.restAtTungku();
        pending_ = TravelResult{true, false, lastTungkuMap_, lastTungkuX_, lastTungkuY_, FACE_DOWN};
        hasPending_ = true;
        pendingRespawn_ = true;
        fade_ = 1.0f;
        fadeDir_ = 1;
    }
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
        DrawRectangle(static_cast<int>(VIEW_W / 2 - w / 2 - 10), VIEW_H - 50, static_cast<int>(w + 20), 22, Fade(BLACK, 0.7f * a));
        ui::textCentered(toast_, VIEW_W / 2.0f, VIEW_H - 47, Fade(ui::BONE, a));
    }
}

void App::drawInspector() {
    // Panel struktur data: Graph dunia (eksplorasi) atau Timeline (battle)
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
        ui::text("Graph dunia (vertex = peta, edge = jalan):", x + 8, y, LIGHTGRAY, 10);
        y += 12;
        world_.graph().forEachVertex([&](World::MapGraph::Vertex* v) {
            std::snprintf(buf, sizeof buf, "%s[%d] %s%s%s", v->id == field_->mapId() ? "> " : "  ", v->id,
                          v->data.name.c_str(), v->data.visited ? "" : " (?)", v->data.hasTungku ? "  *Tungku" : "");
            ui::text(buf, x + 8, y, v->id == field_->mapId() ? YELLOW : RAYWHITE, 10);
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
    if (mode_ != ModeBattle) mapView_.prepareLighting(*field_, time_);
    BeginTextureMode(screen_);
    ClearBackground(BLACK);
    if (mode_ == ModeBattle) {
        battleView_.draw(time_);
    } else {
        mapView_.draw(*field_, time_, showCollision_);
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
            if (frame == 40) {
                // dekat musuh di lorong sel
                field_->enter(1, 12, 12, FACE_RIGHT);
                mapView_.snapCamera(*field_);
            }
            if (frame == 90) TakeScreenshot("shot_map1.png");
            if (frame == 100) {
                // dekat Tungku di aula bawah lorong
                field_->enterAtSpawn(2, "dari_sel");
                if (const MapObject* t = field_->map()->findObject("tungku_lumbung"))
                    field_->enter(2, t->tileX(32) - 2, t->tileY(32), FACE_RIGHT);
                mapView_.snapCamera(*field_);
                enterMapBanner();
            }
            if (frame == 150) TakeScreenshot("shot_tungku.png");
            if (frame == 160) {
                FieldEvent ev;
                ev.kind = FieldEvent::Encounter;
                ev.troop = TR_PENJAGA;
                ev.initiative = INIT_NORMAL;
                startBattle(ev);
            }
            if (frame == 290) TakeScreenshot("shot_battle.png");
            if (frame > 290 && frame < 600 && battle_.phase() == BattleSystem::Input && frame % 10 == 0) {
                static bool opened = false;
                if (!opened) {
                    battleView_.debugOpenSkills();
                    opened = true;
                } else {
                    TakeScreenshot("shot_battle_skill.png");
                    showInspector_ = true;
                    frame = 600;
                }
            }
            if (frame == 610) TakeScreenshot("shot_battle_inspector.png");
            if (frame == 612) showInspector_ = false;
            if (frame > 612 && frame < 900 && battle_.phase() == BattleSystem::Input) {
                BattleAction a{SKL_SERANG, 1};
                battle_.submit(a);
                frame = 900;
            }
            if (frame == 932) TakeScreenshot("shot_battle_hit.png");
            if (frame >= 934) break;
        }
        if (smoke && frame > 30) break;
    }
    delete field_;
    field_ = nullptr;
    mapView_.shutdown();
    UnloadRenderTexture(screen_);
    ui::shutdown();
    Assets::get().unloadAll();
    CloseWindow();
    return 0;
}
