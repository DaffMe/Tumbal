#include "App.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include "Assets.hpp"
#include "Audio.hpp"
#include "Ui.hpp"
#include "core/util/DsTrace.hpp"

bool App::boot(std::string& error) {
    std::string worldPath = Assets::get().path("assets/maps/world.json");
    std::string events[] = {Assets::get().path("assets/events/lumbung.json"), Assets::get().path("assets/events/desa.json"),
                            Assets::get().path("assets/events/ladang.json"), Assets::get().path("assets/events/akhir.json")};
    if (!game_.load(worldPath, events, 4, &error)) return false;
    mapView_.setState(&game_.state());
    title_.open();
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
        if (game_.popNotice(n)) { toast_ = n; toastTime_ = 2.6f; audio::play(audio::DAPAT, 0.7f); }
    }

    updateAudio();

    if (mode_ == ModeTitle) {
        TitleView::Result r = title_.update(dt);
        if (r == TitleView::Quit) quit_ = true;
        if (r == TitleView::NewGame) {
            game_.newGame(title_.heroName(), title_.classId());
            game_.giveStartingGift(title_.gift());
            mapView_.snapCamera(game_.field());
            lastMap_ = -1;
            fade_ = 1.0f;
            fadeDir_ = -1;
            mode_ = ModeField;
        } else if (r == TitleView::LoadSlot) {
            std::string err;
            if (game_.loadFrom(TitleView::slotPath(title_.slot()), &err)) {
                mapView_.snapCamera(game_.field());
                lastMap_ = -1;
                fade_ = 1.0f;
                fadeDir_ = -1;
                mode_ = ModeField;
                toast_ = "Dimuat dari slot " + std::to_string(title_.slot()) + ".";
                toastTime_ = 2.0f;
            }
        }
        return;
    }
    if (mode_ == ModeEnding) {
        endingTime_ += dt;
        if (endingTime_ > 4.0f && (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER))) {
            title_.open();
            mode_ = ModeTitle;
        }
        return;
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

    if (game_.tungkuMenuOpen()) {
        std::string r = tungku_.update(game_);
        if (!r.empty()) { toast_ = r; toastTime_ = 2.4f; }
        if (game_.transferPending()) fadeDir_ = 1;
        mapView_.updateCamera(game_.field(), dt);
        return;
    }
    if (game_.shopMenu() != Game::SHOP_NONE) {
        std::string r = shop_.update(game_);
        if (!r.empty()) { toast_ = r; toastTime_ = 2.4f; }
        mapView_.updateCamera(game_.field(), dt);
        return;
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
    if (game_.state().flag("tamat") != 0 && !game_.interpreter().busy() && mode_ == ModeField) {
        game_.state().setFlag("tamat", 0);
        endingTime_ = 0.0f;
        mode_ = ModeEnding;
        return;
    }
    if (game_.battlePending() && mode_ == ModeField) {
        audio::play(audio::ENCOUNTER);
        mode_ = ModeBattleIntro;
        introTime_ = 0.0f;
    } else if (game_.transferPending() && fadeDir_ == 0) {
        audio::play(audio::PINTU, 0.7f);
        fadeDir_ = 1;
    }
    mapView_.updateCamera(game_.field(), dt);
}

// ------------------------------------------------------------------ audio

// Memilih musik sesuai mode/peta dan membunyikan efek suara antarmuka.
// Hanya membaca state core; tidak ada aturan permainan di sini.
void App::updateAudio() {
    audio::update(GetFrameTime());
    // musik
    std::string music;
    if (mode_ == ModeTitle || mode_ == ModeEnding) {
        music = "judul";
    } else if (mode_ == ModeBattle || mode_ == ModeBattleIntro) {
        int t = game_.battleTroop();
        music = (t == TR_PENJAGA || t == TR_MEMEDI || t == TR_BUTO) ? "boss" : "battle";
    } else if (game_.field().map() != nullptr) {
        music = game_.field().map()->props().getString("musik", "");
    }
    audio::playMusic(music);

    // efek suara menu (judul, Tungku, menu, pilihan dialog)
    bool inMenu = mode_ == ModeTitle ||
                  (mode_ == ModeField && (game_.tungkuMenuOpen() || menu_.open() || game_.shopMenu() != Game::SHOP_NONE ||
                                          game_.interpreter().state() == Interpreter::WaitChoice));
    if (inMenu) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S))
            audio::play(audio::KURSOR, 0.5f);
        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER)) audio::play(audio::PILIH, 0.6f);
        if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE)) audio::play(audio::BATAL, 0.6f);
    } else if (mode_ == ModeField && game_.interpreter().state() == Interpreter::WaitMessage &&
               (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER))) {
        audio::play(audio::KURSOR, 0.35f);
    }
    if (mode_ != ModeField) return;

    // Tungku dinyalakan / dibuka
    bool tungku = game_.tungkuMenuOpen();
    if (tungku && !wasTungku_) audio::play(audio::TUNGKU);
    wasTungku_ = tungku;

    // langkah kaki: setiap kali pemain tiba di petak baru
    const Actor& p = game_.field().player();
    if ((p.x != lastX_ || p.y != lastY_) && game_.field().mapId() == lastStepMap_) {
        stepAlt_ = !stepAlt_;
        audio::play(stepAlt_ ? audio::LANGKAH1 : audio::LANGKAH2, 0.18f);
    }
    lastX_ = p.x;
    lastY_ = p.y;
    lastStepMap_ = game_.field().mapId();
}

// ------------------------------------------------------------------ draw

void App::drawHud() {
    bool menuOpen = game_.tungkuMenuOpen() || game_.shopMenu() != Game::SHOP_NONE || menu_.open();
    if (bannerTime_ > 0 && !menuOpen) {
        float a = std::fmin(1.0f, bannerTime_ / 0.6f) * std::fmin(1.0f, (3.0f - bannerTime_) / 0.4f);
        float w = ui::measure(bannerText_, 32);
        float x = VIEW_W / 2 - w / 2, y = 30;
        // dua gradien bertemu tepat di tengah (bilangan bulat) agar tidak ada garis tumpang di sambungan
        int mid = VIEW_W / 2, half = static_cast<int>(w / 2) + 80;
        DrawRectangleGradientH(mid - half, static_cast<int>(y - 4), half, 40, Fade(BLACK, 0.0f), Fade(BLACK, 0.6f * a));
        DrawRectangleGradientH(mid, static_cast<int>(y - 4), half, 40, Fade(BLACK, 0.6f * a), Fade(BLACK, 0.0f));
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

void App::drawEnding() {
    ClearBackground(BLACK);
    float t = endingTime_;
    auto line = [&](const std::string& s, float y, float at, Color c, float size = 16) {
        float a = std::fmin(1.0f, std::fmax(0.0f, (t - at) / 1.2f));
        if (a > 0) ui::textCentered(s, VIEW_W / 2.0f, y, Fade(c, a), size);
    };
    line("Buto Ijo tenggelam untuk terakhir kalinya.", 70, 0.5f, ui::BONE);
    line("Tapi lesung di Karangtumbal masih berbunyi.", 96, 2.0f, ui::DIM);
    line("TUMBAL", 140, 3.5f, Color{196, 52, 44, 255}, 40);
    line("akhir dari versi demo", 186, 4.0f, ui::DIM);
    const GameState& gs = game_.state();
    line("Level " + std::to_string(gs.level()) + "   -   Mati " + std::to_string(gs.deaths()) + " kali   -   Bekal " +
             std::to_string(gs.count(IT_BIJI_TIMUN) + gs.count(IT_JARUM) + gs.count(IT_GARAM) + gs.count(IT_TERASI)) + "/4",
         222, 5.0f, ui::AMBER);
    line("Aset: Sevarihk, Redshrike, Nidhoggn, Kenney & lainnya (lihat CREDITS.md)", 280, 6.0f, ui::DIM, 10);
    line("Terima kasih sudah bermain.   [Z] kembali ke judul", 320, 6.5f, ui::BONE);
}

void App::drawFrame() {
    if (mode_ == ModeEnding) {
        BeginTextureMode(screen_);
        drawEnding();
        EndTextureMode();
        return;
    }
    if (mode_ == ModeTitle) {
        BeginTextureMode(screen_);
        ClearBackground(BLACK);
        title_.draw(time_);
        EndTextureMode();
        return;
    }
    if (mode_ != ModeBattle) mapView_.prepareLighting(game_.field(), time_, &game_.state().bloodstain());
    BeginTextureMode(screen_);
    ClearBackground(BLACK);
    if (mode_ == ModeBattle) {
        battleView_.draw(time_);
    } else {
        mapView_.draw(game_.field(), time_, showCollision_, &game_.state().bloodstain());
        dialog_.draw(game_.interpreter(), time_);
        menu_.draw(game_, time_);
        tungku_.draw(game_, time_);
        shop_.draw(game_, time_);
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
    audio::init();
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
    if (shots) {
        // --shots: tampilkan judul sebentar, lalu langsung permainan baru
        for (int i = 0; i < 90 && !WindowShouldClose(); ++i) {
            time_ += 1.0 / 60.0;
            title_.update(1.0f / 60.0f);
            drawFrame();
            present();
        }
        TakeScreenshot("shot_title.png");
        game_.newGame("Tumbal", KL_PENDEKAR);
        mapView_.snapCamera(game_.field());
        mode_ = ModeField;
    }
    while (!WindowShouldClose() && !quit_) {
        float dt = std::fmin(GetFrameTime(), 1.0f / 20.0f);
        if (shots) dt = 1.0f / 60.0f;
        update(dt);
        drawFrame();
        present();
        ++frame;
        if (shots) {
            // skenario screenshot otomatis: kedelapan peta, dialog, battle biasa, menu, dan Buto Ijo
            Interpreter& it = game_.interpreter();
            auto go = [&](int map, int x, int y, int dir) {
                game_.field().enter(map, x, y, dir);
                mapView_.snapCamera(game_.field());
            };
            auto skipDialog = [&](int from, int to) {
                if (frame > from && frame < to && game_.inDialog() && frame % 4 == 0) {
                    if (it.state() == Interpreter::WaitChoice) it.choose(0); else it.resume();
                }
            };
            for (const char* f : {"bambu_masuk", "laut_masuk", "rawa_masuk", "sarang_masuk", "ladang_masuk"})
                game_.state().setFlag(f, 1);
            if (frame == 50) TakeScreenshot("shot_sel.png");          // dialog pembuka
            skipDialog(55, 100);
            if (frame == 100) go(2, 30, 6, FACE_UP);
            if (frame == 140) TakeScreenshot("shot_lorong.png");
            if (frame == 150) go(3, 22, 18, FACE_UP);
            if (frame == 190) TakeScreenshot("shot_desa.png");
            if (frame == 200) go(4, 36, 12, FACE_RIGHT);
            if (frame == 240) TakeScreenshot("shot_ladang.png");
            if (frame == 250) go(5, 27, 14, FACE_UP);
            if (frame == 290) TakeScreenshot("shot_bambu.png");
            if (frame == 300) go(6, 18, 5, FACE_UP);
            if (frame == 340) TakeScreenshot("shot_laut.png");
            if (frame == 345) it.runEvent("timun_mas");
            if (frame == 362) TakeScreenshot("shot_dialog.png");   // potret pembicara
            skipDialog(365, 420);
            if (frame == 425) go(7, 22, 16, FACE_DOWN);
            if (frame == 465) TakeScreenshot("shot_rawa.png");
            if (frame == 470) go(8, 16, 12, FACE_DOWN);
            if (frame == 510) TakeScreenshot("shot_sarang.png");
            if (frame == 515) { go(5, 25, 7, FACE_UP); game_.hostStartBattle(TR_ARWAH_2); }
            if (frame == 640) TakeScreenshot("shot_battle.png");
            if (frame == 645) { mode_ = ModeField; go(3, 22, 18, FACE_UP); menu_.show(); }
            if (frame == 680) TakeScreenshot("shot_menu.png");
            if (frame == 685) { menu_.hide(); game_.state().addSukma(300); game_.hostOpenShop("dagang"); }
            if (frame == 700) TakeScreenshot("shot_toko.png");
            if (frame == 705) { game_.closeShopMenu(); game_.state().addItem(IT_BIJIH_BESI, 1); game_.hostOpenShop("tempa"); }
            if (frame == 720) TakeScreenshot("shot_tempa.png");
            if (frame == 725) { game_.closeShopMenu(); go(8, 16, 22, FACE_DOWN); it.runEvent("buto_bangun"); }
            skipDialog(725, 780);
            if (frame == 896 && mode_ == ModeBattle && battle_.count() > 3) {
                // pamerkan telegraf: tangan kanan seolah sedang merapal Hantam Gada ke Tumbal
                Battler& kanan = battle_.battler(battle_.count() - 1);
                kanan.chanting = true;
                kanan.chantSkill = SKL_HANTAM_GADA;
                kanan.chantTarget = 0;
                kanan.chantTicks = 100;
            }
            if (frame == 900) TakeScreenshot("shot_buto_battle.png");
            if (frame >= 902) break;
        }
        if (smoke && frame > 30) break;
    }
    mapView_.shutdown();
    UnloadRenderTexture(screen_);
    ui::shutdown();
    Assets::get().unloadAll();
    audio::shutdown();
    CloseWindow();
    return 0;
}
