#include "TungkuView.hpp"

#include <cmath>
#include <cstdio>
#include "Ui.hpp"
#include "raylib.h"

static bool confirm() { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
static bool cancel() { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }
static const char* ROOT[] = {"Naik Level", "Warp", "Pergi"};

std::string TungkuView::update(Game& game) {
    if (!game.tungkuMenuOpen()) { page_ = Root; return ""; }
    bool up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    if (page_ == Root) {
        if (up) index_ = (index_ + 2) % 3;
        if (down) index_ = (index_ + 1) % 3;
        if (cancel()) { game.closeTungkuMenu(); index_ = 0; }
        if (confirm()) {
            if (index_ == 0) page_ = Level;
            else if (index_ == 1) {
                warpCount_ = game.warpTargets(warpMap_, warpDepth_, 16);
                warpIndex_ = 0;
                page_ = Warp;
            } else { game.closeTungkuMenu(); index_ = 0; }
        }
        return "";
    }
    if (page_ == Level) {
        if (cancel()) page_ = Root;
        if (confirm()) return game.levelUp();
        return "";
    }
    // Warp
    if (warpCount_ > 0) {
        if (up) warpIndex_ = (warpIndex_ + warpCount_ - 1) % warpCount_;
        if (down) warpIndex_ = (warpIndex_ + 1) % warpCount_;
    }
    if (cancel()) page_ = Root;
    if (confirm() && warpCount_ > 0) {
        int target = warpMap_[warpIndex_];
        if (target == game.field().mapId()) return "Kamu sudah berada di sini.";
        if (game.warpTo(target)) {
            page_ = Root;
            index_ = 0;
            return "Bara Tungku menelanmu...";
        }
    }
    return "";
}

void TungkuView::draw(Game& game, double time) {
    if (!game.tungkuMenuOpen()) return;
    DrawRectangle(0, 0, 640, 360, Fade(BLACK, 0.35f));
    // judul
    ui::textCentered("TUNGKU", 320, 22, ui::AMBER, 30);
    Rectangle rw{24, 70, 150, 82};
    ui::window(rw, 0.95f);
    for (int i = 0; i < 3; ++i) {
        float y = rw.y + 8 + i * 22;
        bool sel = i == index_;
        ui::text(ROOT[i], rw.x + 26, y, sel && page_ == Root ? ui::AMBER : (sel ? ui::BONE : ui::DIM));
        if (sel && page_ == Root) ui::cursor(rw.x + 12, y + 8, time);
    }
    const GameState& gs = game.state();
    Rectangle sw{24, 158, 150, 52};
    ui::window(sw, 0.95f);
    ui::text("Sukma", sw.x + 10, sw.y + 6, ui::DIM);
    ui::textRight(std::to_string(gs.sukma()), sw.x + sw.width - 10, sw.y + 6, ui::AMBER);
    ui::text("Level", sw.x + 10, sw.y + 26, ui::DIM);
    ui::textRight(std::to_string(gs.level()), sw.x + sw.width - 10, sw.y + 26, ui::BONE);

    if (page_ == Level || (page_ == Root && index_ == 0)) {
        Rectangle lw{184, 70, 432, 196};
        ui::window(lw, 0.95f);
        int cost = gs.levelUpCost();
        char buf[96];
        std::snprintf(buf, sizeof buf, "Level %d  ->  %d", gs.level(), gs.level() + 1);
        ui::text(buf, lw.x + 16, lw.y + 10, ui::BONE);
        std::snprintf(buf, sizeof buf, "Butuh %d sukma", cost);
        ui::textRight(buf, lw.x + lw.width - 16, lw.y + 10, gs.canLevelUp() ? ui::AMBER : ui::BLOOD);
        Stats now = gs.hero().stats;
        Stats next = statsAtLevel(gs.classId(), gs.level() + 1);
        const char* names[] = {"HP", "MP", "Serang", "Tahan", "Ilmu", "Tolak", "Gesit", "Untung"};
        int a[] = {now.mhp, now.mmp, now.atk, now.def, now.mag, now.res, now.agi, now.luk};
        int b[] = {next.mhp, next.mmp, next.atk, next.def, next.mag, next.res, next.agi, next.luk};
        for (int i = 0; i < 8; ++i) {
            float x = lw.x + 16 + (i % 2) * 210, y = lw.y + 44 + (i / 2) * 24;
            ui::text(names[i], x, y, ui::DIM);
            ui::textRight(std::to_string(a[i]), x + 120, y, ui::BONE);
            std::snprintf(buf, sizeof buf, "-> %d", b[i]);
            ui::text(buf, x + 130, y, b[i] > a[i] ? Color{150, 220, 150, 255} : ui::DIM);
        }
        if (page_ == Level) {
            float blink = 0.5f + 0.5f * static_cast<float>(std::sin(time * 5));
            ui::textCentered(gs.canLevelUp() ? "[Z] tukar sukma    [X] kembali" : "Sukma tidak cukup.    [X] kembali",
                             lw.x + lw.width / 2, lw.y + lw.height - 28, Fade(ui::BONE, 0.6f + 0.4f * blink));
        }
    }
    if (page_ == Warp) {
        Rectangle ww{184, 70, 432, 196};
        ui::window(ww, 0.95f);
        ui::text("Tungku yang menyala & terjangkau (BFS Graph dunia):", ww.x + 16, ww.y + 10, ui::DIM, 10);
        if (warpCount_ == 0) ui::text("(belum ada)", ww.x + 16, ww.y + 34, ui::DIM);
        for (int i = 0; i < warpCount_; ++i) {
            const MapInfo* mi = game.world().info(warpMap_[i]);
            float y = ww.y + 30 + i * 22;
            bool sel = i == warpIndex_;
            ui::text(mi ? mi->name : "?", ww.x + 30, y, sel ? ui::AMBER : ui::BONE);
            std::string d = warpDepth_[i] == 0 ? "di sini" : std::to_string(warpDepth_[i]) + " jalan";
            ui::textRight(d, ww.x + ww.width - 16, y, ui::DIM);
            if (sel) ui::cursor(ww.x + 16, y + 8, time);
        }
    }
}
