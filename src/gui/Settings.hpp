#pragma once
// ============================================================================
// Settings — pengaturan pemain yang bertahan di antara sesi (saves/settings.json):
// bahasa, mode layar (jendela / borderless / fullscreen eksklusif), ukuran jendela,
// skala gambar, VSync, batas FPS, volume musik & efek, tampilkan FPS.
// Kamus terjemahan dimuat dari assets/lang/en.json lewat core/util/Lang.
// Pengaturan hanya urusan tampilan/suara, jadi tinggal di GUI (bukan aturan game).
// ============================================================================

#include "core/util/Lang.hpp"

namespace settings {

enum Display { WINDOWED = 0, BORDERLESS, FULLSCREEN, DISPLAY_COUNT };
enum Scaling { PIXEL_PERFECT = 0, FILL, SCALING_COUNT };

struct Values {
    Language lang = LANG_EN;
    int  display = WINDOWED;
    int  windowScale = 2;       // ukuran jendela = 640x360 x skala (2..maks layar)
    int  scaling = PIXEL_PERFECT;
    bool vsync = true;
    int  fpsCap = 0;            // indeks FPS_OPTIONS: ikut monitor, 30, 60, 120, 144, tanpa batas
    int  musicVolume = 7;       // 0..10
    int  sfxVolume = 8;         // 0..10
    bool showFps = false;
};

constexpr int FPS_OPTION_COUNT = 6;
int fpsOption(int index);       // nilai FPS untuk indeks (0 = ikut monitor, -1 = tanpa batas)

void load();                    // muat kamus + pengaturan lalu terapkan; bawaan bila belum ada berkas
void save();
void apply();                   // terapkan semua nilai ke jendela, FPS, dan audio
Values& get();

void setLanguage(Language l);
void toggleLanguage();
void setDisplay(int mode);      // ganti mode layar & simpan
void toggleFullscreen();        // Alt+Enter / F11: jendela <-> mode layar penuh terakhir
int  maxWindowScale();          // skala jendela terbesar yang muat di monitor

}  // namespace settings
