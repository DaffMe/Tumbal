#pragma once
// ============================================================================
// MenuView — menu lapangan (tombol X): Barang, Status, Tutup.
// Memakai barang lewat Game::useItem (aturan ada di core).
// ============================================================================

#include <string>
#include "core/game/Game.hpp"

class MenuView {
public:
    bool open() const { return open_; }
    void show() { open_ = true; page_ = Root; index_ = 0; }
    // Mengembalikan pesan hasil (untuk notifikasi), kosong bila tidak ada.
    std::string update(Game& game);
    void draw(const Game& game, double time);

private:
    enum Page { Root, Items, Status };
    bool open_ = false;
    Page page_ = Root;
    int  index_ = 0;
    int  itemIndex_ = 0;
    int  items_[IT_COUNT] = {};
    int  itemCount_ = 0;

    void refreshItems(const Game& game);
};
