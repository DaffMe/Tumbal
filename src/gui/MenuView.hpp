#pragma once
// ============================================================================
// MenuView — menu lapangan (tombol X): Barang, Status, Simpan, Cheat, Keluar, Tutup.
// Memakai barang lewat Game::useItem dan cheat lewat Game::cheat (aturan ada di core).
// Halaman Keluar hanya mengirim permintaan (ke judul / keluar game) yang dijalankan App.
// ============================================================================

#include <string>
#include "core/game/Game.hpp"

class MenuView {
public:
    bool open() const { return open_; }
    void show() { open_ = true; page_ = Root; index_ = 0; }
    void hide() { open_ = false; }
    void showCheats() { open_ = true; page_ = Cheat; }
    // Permintaan dari halaman Keluar: 0 tidak ada, 1 ke layar judul, 2 keluar dari game. Diambil sekali.
    int takeExitRequest() { int r = exitRequest_; exitRequest_ = 0; return r; }
    // Mengembalikan pesan hasil (untuk notifikasi), kosong bila tidak ada.
    std::string update(Game& game);
    void draw(const Game& game, double time);

private:
    enum Page { Root, Items, Status, Save, Cheat, Exit };
    bool open_ = false;
    Page page_ = Root;
    int  index_ = 0;
    int  itemIndex_ = 0;
    int  items_[IT_COUNT] = {};
    int  itemCount_ = 0;
    int  slotIndex_ = 0;
    std::string slotInfo_[3];
    int  cheatIndex_ = 0;
    int  statIndex_ = 2;
    int  exitIndex_ = 0;
    int  exitRequest_ = 0;

    void refreshItems(const Game& game);
};
