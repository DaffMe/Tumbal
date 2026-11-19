#pragma once
// ============================================================================
// ShopView — menu pedagang keliling (beli dengan sukma) dan pandai besi
// (tempa senjata +0..+3). Dibuka oleh perintah event "shop"; semua aturan
// (harga, stok, biaya tempa) ada di GameState/GameData, layar ini hanya
// membaca state dan memanggil Game::buy() / Game::forge().
// ============================================================================

#include "core/game/Game.hpp"

class ShopView {
public:
    // Mengembalikan pesan hasil (untuk notifikasi), kosong bila tidak ada.
    std::string update(Game& game);
    void draw(Game& game, double time);

private:
    int index_ = 0;
};
