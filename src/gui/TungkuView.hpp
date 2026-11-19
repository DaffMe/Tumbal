#pragma once
// ============================================================================
// TungkuView — menu Tungku (seperti menu bonfire Black Souls): naik level
// dengan sukma, warp ke Tungku lain yang terjangkau, atau pergi.
// Daftar warp diambil dari Game::warpTargets() = BFS pada Graph dunia.
// ============================================================================

#include "core/game/Game.hpp"

class TungkuView {
public:
    // Mengembalikan pesan hasil (untuk notifikasi), kosong bila tidak ada.
    std::string update(Game& game);
    void draw(Game& game, double time);

private:
    enum Page { Root, Level, Warp, Companion };
    Page page_ = Root;
    int  index_ = 0;
    int  warpIndex_ = 0;
    int  warpMap_[16] = {};
    int  warpDepth_[16] = {};
    int  warpCount_ = 0;
};
