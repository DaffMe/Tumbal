#include "MenuView.hpp"

#include <cstdio>
#include "TitleView.hpp"
#include "Ui.hpp"
#include "raylib.h"

static bool confirm() { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
static bool cancel() { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }
static const char* ROOT[] = {"Barang", "Status", "Simpan", "Tutup"};
static constexpr int ROOT_N = 4;

void MenuView::refreshItems(const Game& game) {
    itemCount_ = 0;
    for (int id = 1; id < IT_COUNT; ++id)
        if (game.state().count(id) > 0) items_[itemCount_++] = id;
    if (itemIndex_ >= itemCount_) itemIndex_ = itemCount_ > 0 ? itemCount_ - 1 : 0;
}

std::string MenuView::update(Game& game) {
    if (!open_) return "";
    bool up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    if (page_ == Root) {
        if (up) index_ = (index_ + ROOT_N - 1) % ROOT_N;
        if (down) index_ = (index_ + 1) % ROOT_N;
        if (cancel()) open_ = false;
        if (confirm()) {
            if (index_ == 0) { page_ = Items; refreshItems(game); }
            else if (index_ == 1) page_ = Status;
            else if (index_ == 2) {
                page_ = Save;
                for (int i = 0; i < 3; ++i) slotInfo_[i] = Game::describeSave(TitleView::slotPath(i + 1));
            } else open_ = false;
        }
        return "";
    }
    if (page_ == Save) {
        if (up) slotIndex_ = (slotIndex_ + 2) % 3;
        if (down) slotIndex_ = (slotIndex_ + 1) % 3;
        if (cancel()) page_ = Root;
        if (confirm()) {
            std::string err;
            bool ok = game.saveTo(TitleView::slotPath(slotIndex_ + 1), &err);
            for (int i = 0; i < 3; ++i) slotInfo_[i] = Game::describeSave(TitleView::slotPath(i + 1));
            return ok ? "Tersimpan di slot " + std::to_string(slotIndex_ + 1) + "." : "Gagal menyimpan: " + err;
        }
        return "";
    }
    if (page_ == Status) {
        if (cancel() || confirm()) page_ = Root;
        return "";
    }
    // Items
    refreshItems(game);
    if (itemCount_ > 0) {
        if (up) itemIndex_ = (itemIndex_ + itemCount_ - 1) % itemCount_;
        if (down) itemIndex_ = (itemIndex_ + 1) % itemCount_;
    }
    if (cancel()) page_ = Root;
    if (confirm() && itemCount_ > 0) {
        std::string r = game.useItem(items_[itemIndex_]);
        if (game.transferPending()) open_ = false;
        refreshItems(game);
        return r.empty() ? "Tidak ada gunanya sekarang." : r;
    }
    return "";
}

void MenuView::draw(const Game& game, double time) {
    if (!open_) return;
    DrawRectangle(0, 0, 640, 360, Fade(BLACK, 0.45f));
    Rectangle rw{16, 16, 130, 104};
    ui::window(rw, 0.95f);
    for (int i = 0; i < ROOT_N; ++i) {
        float y = rw.y + 8 + i * 22;
        bool sel = page_ == Root && i == index_;
        ui::text(ROOT[i], rw.x + 26, y, sel ? ui::AMBER : ui::BONE);
        if (sel) ui::cursor(rw.x + 12, y + 8, time);
    }
    // ringkasan selalu tampil: sukma & HP
    const GameState& gs = game.state();
    const Battler& h = gs.hero();
    Rectangle sw{16, 126, 130, 72};
    ui::window(sw, 0.95f);
    ui::text("Sukma", sw.x + 10, sw.y + 6, ui::DIM);
    ui::textRight(std::to_string(gs.sukma()), sw.x + sw.width - 10, sw.y + 6, ui::AMBER);
    ui::text("HP", sw.x + 10, sw.y + 28, ui::DIM);
    ui::textRight(std::to_string(h.hp) + "/" + std::to_string(h.stats.mhp), sw.x + sw.width - 10, sw.y + 28, ui::BONE);
    ui::bar({sw.x + 10, sw.y + 52, sw.width - 20, 4}, static_cast<float>(h.hp) / h.stats.mhp, ui::HP_A, ui::HP_B);

    if (page_ == Items) {
        Rectangle iw{156, 16, 468, 248};
        ui::window(iw, 0.95f);
        if (itemCount_ == 0) ui::text("(tidak membawa apa-apa)", iw.x + 16, iw.y + 12, ui::DIM);
        for (int i = 0; i < itemCount_; ++i) {
            const ItemDef* d = itemDef(items_[i]);
            float y = iw.y + 10 + i * 22;
            bool sel = i == itemIndex_;
            ui::text(d->name, iw.x + 26, y, d->inField ? (sel ? ui::AMBER : ui::BONE) : ui::DIM);
            char buf[16];
            std::snprintf(buf, sizeof buf, "x%d", gs.count(items_[i]));
            ui::textRight(buf, iw.x + iw.width - 14, y, ui::BONE);
            if (sel) ui::cursor(iw.x + 12, y + 8, time);
        }
        if (itemCount_ > 0) {
            Rectangle dw{156, 270, 468, 74};
            ui::window(dw, 0.95f);
            std::vector<std::string> lines = ui::wrap(itemDef(items_[itemIndex_])->desc, dw.width - 28);
            for (size_t k = 0; k < lines.size() && k < 3; ++k) ui::text(lines[k], dw.x + 14, dw.y + 10 + k * 20.0f, ui::BONE);
        }
    }
    if (page_ == Save) {
        Rectangle w{156, 16, 468, 110};
        ui::window(w, 0.95f);
        ui::text("Simpan ke slot (mati tidak pernah menyimpan otomatis):", w.x + 14, w.y + 8, ui::DIM, 10);
        for (int i = 0; i < 3; ++i) {
            float y = w.y + 26 + i * 26;
            bool sel = i == slotIndex_;
            std::string label = "Slot " + std::to_string(i + 1) + "   " + (slotInfo_[i].empty() ? "(kosong)" : slotInfo_[i]);
            ui::text(label, w.x + 30, y, sel ? ui::AMBER : ui::BONE);
            if (sel) ui::cursor(w.x + 14, y + 8, time);
        }
    }
    if (page_ == Status) {
        Rectangle stw{156, 16, 468, 248};
        ui::window(stw, 0.95f);
        ui::text(h.name, stw.x + 16, stw.y + 10, ui::AMBER, 30);
        ui::text(std::string(classDef(gs.classId()).name) + "  -  Level " + std::to_string(gs.level()), stw.x + 16, stw.y + 44, ui::BONE);
        const char* names[] = {"HP", "MP", "Serang", "Tahan", "Ilmu", "Tolak", "Gesit", "Untung"};
        int vals[] = {h.stats.mhp, h.stats.mmp, h.stats.atk, h.stats.def, h.stats.mag, h.stats.res, h.stats.agi, h.stats.luk};
        for (int i = 0; i < 8; ++i) {
            float x = stw.x + 16 + (i % 2) * 220, y = stw.y + 78 + (i / 2) * 24;
            ui::text(names[i], x, y, ui::DIM);
            ui::textRight(std::to_string(vals[i]), x + 180, y, ui::BONE);
        }
        ui::text("Mati: " + std::to_string(gs.deaths()) + " kali", stw.x + 16, stw.y + 184, ui::DIM);
        if (gs.bloodstain().active)
            ui::text("Sukma tercecer: " + std::to_string(gs.bloodstain().sukma), stw.x + 16, stw.y + 206, ui::BLOOD);
    }
}
