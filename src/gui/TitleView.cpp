#include "TitleView.hpp"

#include <cmath>
#include "Assets.hpp"
#include "Settings.hpp"
#include "Ui.hpp"
#include "core/util/Lang.hpp"
#include "core/data/GameData.hpp"
#include "core/game/Game.hpp"
#include "raylib.h"

static bool confirm() { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
static bool cancel() { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE); }
static bool up() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
static bool down() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
static bool left() { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
static bool right() { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }

static const char* ROOT[] = {"Mulai Baru", "Lanjutkan", "Pengaturan", "Keluar"};
static const char* GIFTS[] = {"Dua Ramuan Jamu", "Sukma Penjaga", "Tiga Bom Minyak Jarak", "Tiga Abu Tungku"};
static const char* GIFT_DESC[] = {
    "Dua ramuan tambahan. Ramuan bawaan tetap terisi ulang di Tungku.",
    "Sukma pekat seorang penjaga: 200 sukma untuk naik level lebih awal.",
    "Kendi minyak jarak yang menyala. Kuat melawan makhluk jerami dan serangga.",
    "Abu yang membawamu pulang ke Tungku terakhir kapan saja.",
};

std::string TitleView::slotPath(int slot) {
    return Assets::get().savesDir() + "/slot" + std::to_string(slot) + ".json";
}

void TitleView::refreshSlots() {
    for (int i = 0; i < 3; ++i) slotInfo_[i] = Game::describeSave(slotPath(i + 1));
}

void TitleView::open() {
    page_ = Root;
    index_ = 0;
    fadeIn_ = 1.0f;
    refreshSlots();
}

TitleView::Result TitleView::update(float dt) {
    if (fadeIn_ > 0) fadeIn_ -= dt * 0.8f;
    switch (page_) {
        case Root:
            if (up()) index_ = (index_ + 3) % 4;
            if (down()) index_ = (index_ + 1) % 4;
            if (confirm()) {
                if (index_ == 0) { page_ = Name; }
                else if (index_ == 1) { refreshSlots(); page_ = Slots; slotIndex_ = 0; }
                else if (index_ == 2) { page_ = Options; settingsView_.open(); }
                else return Quit;
            }
            break;
        case Options:
            if (settingsView_.update()) { page_ = Root; refreshSlots(); }
            break;
        case Name: {
            int c = GetCharPressed();
            while (c > 0) {
                if (c >= 32 && c < 127 && name_.size() < 12) name_ += static_cast<char>(c);
                c = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && !name_.empty()) name_.pop_back();
            if (IsKeyPressed(KEY_ENTER) && !name_.empty()) page_ = Class;
            if (IsKeyPressed(KEY_ESCAPE)) page_ = Root;
            break;
        }
        case Class:
            // kartu kelas tersusun mendatar: kiri/kanan (atas/bawah tetap bisa)
            if (left() || up()) classIndex_ = (classIndex_ + 2) % 3;
            if (right() || down()) classIndex_ = (classIndex_ + 1) % 3;
            if (cancel()) page_ = Name;
            if (confirm()) page_ = Gift;
            break;
        case Gift:
            if (up()) giftIndex_ = (giftIndex_ + 3) % 4;
            if (down()) giftIndex_ = (giftIndex_ + 1) % 4;
            if (cancel()) page_ = Class;
            if (confirm()) return NewGame;
            break;
        case Slots:
            if (up()) slotIndex_ = (slotIndex_ + 2) % 3;
            if (down()) slotIndex_ = (slotIndex_ + 1) % 3;
            if (cancel()) page_ = Root;
            if (confirm() && !slotInfo_[slotIndex_].empty()) return LoadSlot;
            break;
    }
    return None;
}

void TitleView::draw(double time) {
    Texture2D bg = Assets::get().texture("assets/battlebacks/lumbung.png");
    if (bg.id) {
        float drift = static_cast<float>(std::sin(time * 0.15) * 8);
        DrawTexturePro(bg, {0, 0, static_cast<float>(bg.width), static_cast<float>(bg.height)},
                       {-20 + drift, -12, 680, 384}, {0, 0}, 0, Color{150, 130, 120, 255});
    }
    DrawRectangleGradientV(0, 0, 640, 360, Fade(BLACK, 0.2f), Fade(BLACK, 0.85f));
    // bara melayang
    for (int i = 0; i < 24; ++i) {
        float t = static_cast<float>(std::fmod(time * (0.05 + (i % 5) * 0.012) + i * 0.137, 1.0));
        float x = static_cast<float>(std::fmod(i * 97.3 + std::sin(time * 0.7 + i) * 14, 640));
        float y = 370 - t * 380;
        DrawCircleV({x, y}, 1.2f + (i % 3) * 0.5f, Fade(Color{255, 140, 60, 255}, (1 - t) * 0.8f));
    }

    if (page_ == Root || page_ == Slots) {
        // judul besar: font 40 px digambar dua kali lipat (kelipatan bulat, tetap tajam)
        const char* title = "TUMBAL";
        float tw = MeasureTextEx(ui::font(), title, 20, 0).x * 4.0f;
        Font big = ui::font();
        Vector2 pos{320 - tw / 2, 54};
        DrawTextEx(big, title, {pos.x + 3, pos.y + 3}, 80, 0, Fade(BLACK, 0.8f));
        DrawTextEx(big, title, pos, 80, 0, Color{196, 52, 44, 255});
        ui::textCentered("sebuah dongeng tumbal dari tanah Jawa", 320, 140, ui::DIM);
    }

    if (page_ == Options) {
        settingsView_.draw(Rectangle{90, 34, 460, 292}, time);
        ui::textCentered("atas/bawah pilih    kiri/kanan ganti    X kembali", 320, 334, ui::DIM, 10);
    }
    if (page_ == Root) {
        for (int i = 0; i < 4; ++i) {
            float y = 190 + i * 26;
            bool sel = i == index_;
            ui::textCentered(ROOT[i], 320, y, sel ? ui::AMBER : ui::BONE);
            if (sel) ui::cursor(320 - ui::measure(ROOT[i]) / 2 - 18, y + 8, time);
        }
        ui::text("Z pilih   X batal   panah gerak", 12, 340, Fade(ui::DIM, 0.7f), 10);
        ui::textRight("Aset: lihat CREDITS.md", 628, 340, Fade(ui::DIM, 0.7f), 10);
    } else if (page_ == Slots) {
        Rectangle w{140, 180, 360, 106};
        ui::window(w, 0.94f);
        for (int i = 0; i < 3; ++i) {
            float y = w.y + 10 + i * 30;
            bool sel = i == slotIndex_;
            std::string label = trf("Slot %d", i + 1) + "   " + (slotInfo_[i].empty() ? tr("(kosong)") : slotInfo_[i]);
            ui::text(label, w.x + 30, y, slotInfo_[i].empty() ? ui::DIM : (sel ? ui::AMBER : ui::BONE));
            if (sel) ui::cursor(w.x + 14, y + 8, time);
        }
    } else if (page_ == Name) {
        ui::textCentered("Siapa namamu, tumbal?", 320, 110, ui::BONE, 30);
        Rectangle w{200, 160, 240, 40};
        ui::window(w, 0.94f);
        bool blink = static_cast<int>(time * 2) % 2 == 0;
        ui::textCentered(name_ + (blink ? "_" : " "), 320, w.y + 10, ui::AMBER);
        ui::textCentered("ketik nama lalu tekan Enter", 320, 214, ui::DIM);
    } else if (page_ == Class) {
        ui::textCentered("Dulu, sebelum ditanam, kamu adalah...", 320, 36, ui::BONE, 30);
        for (int i = 0; i < 3; ++i) {
            const ClassDef& c = classDef(i);
            Rectangle w{40.0f + i * 190, 84, 180, 222};
            bool sel = i == classIndex_;
            ui::window(w, sel ? 0.96f : 0.75f);
            if (sel) DrawRectangleLinesEx(w, 2, ui::AMBER);
            ui::textCentered(c.name, w.x + w.width / 2, w.y + 10, sel ? ui::AMBER : ui::BONE, 30);
            std::vector<std::string> lines = ui::wrap(c.desc, w.width - 20);
            for (size_t k = 0; k < lines.size() && k < 3; ++k) ui::text(lines[k], w.x + 10, w.y + 44 + k * 18.0f, ui::DIM);
            const char* names[] = {"HP", "MP", "Serang", "Tahan", "Ilmu", "Gesit"};
            int vals[] = {c.base.mhp, c.base.mmp, c.base.atk, c.base.def, c.base.mag, c.base.agi};
            for (int k = 0; k < 6; ++k) {
                float y = w.y + 106 + k * 18;
                ui::text(names[k], w.x + 14, y, ui::DIM);
                ui::textRight(std::to_string(vals[k]), w.x + w.width - 14, y, ui::BONE);
            }
        }
        ui::textCentered("kiri/kanan pilih    Z lanjut    X kembali", 320, 326, ui::DIM);
    } else if (page_ == Gift) {
        ui::textCentered("Satu benda yang kau genggam saat ditanam:", 320, 50, ui::BONE, 30);
        Rectangle w{120, 110, 400, 110};
        ui::window(w, 0.94f);
        for (int i = 0; i < 4; ++i) {
            float y = w.y + 10 + i * 24;
            bool sel = i == giftIndex_;
            ui::text(GIFTS[i], w.x + 30, y, sel ? ui::AMBER : ui::BONE);
            if (sel) ui::cursor(w.x + 14, y + 8, time);
        }
        Rectangle d{120, 230, 400, 50};
        ui::window(d, 0.94f);
        std::vector<std::string> lines = ui::wrap(GIFT_DESC[giftIndex_], d.width - 24);
        for (size_t k = 0; k < lines.size() && k < 2; ++k) ui::text(lines[k], d.x + 12, d.y + 6 + k * 20.0f, ui::DIM);
    }
    if (fadeIn_ > 0) DrawRectangle(0, 0, 640, 360, Fade(BLACK, fadeIn_));
}
