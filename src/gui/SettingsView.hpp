#pragma once
// ============================================================================
// SettingsView — halaman Pengaturan (dipakai layar judul dan menu X):
// bahasa, mode layar, ukuran jendela, skala gambar, VSync, batas FPS, volume,
// tampilkan FPS. Atas/bawah memilih, kiri/kanan (atau Z) mengubah, X kembali.
// Setiap perubahan langsung diterapkan dan disimpan lewat Settings.
// ============================================================================

#include "raylib.h"

class SettingsView {
public:
    void open() { index_ = 0; }
    // Mengembalikan true bila pemain menutup halaman (X atau pilih "Kembali").
    bool update();
    void draw(Rectangle area, double time);

private:
    int index_ = 0;
    void change(int dir);
};
