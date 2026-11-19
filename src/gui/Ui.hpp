#pragma once
// ============================================================================
// Ui — primitif antarmuka bersama: font piksel, teks bayangan, jendela
// bergaya JRPG (panel gelap berbingkai), bar HP/MP/AP.
// ============================================================================

#include <string>
#include "raylib.h"

namespace ui {

void init();        // muat font (panggil setelah InitWindow)
void shutdown();
Font font();

// Ukuran default 16 (font asli). Ukuran lain diskalakan.
void text(const std::string& s, float x, float y, Color c = RAYWHITE, float size = 16, bool shadow = true);
void textCentered(const std::string& s, float cx, float y, Color c = RAYWHITE, float size = 16);
void textRight(const std::string& s, float rx, float y, Color c = RAYWHITE, float size = 16);
float measure(const std::string& s, float size = 16);

void window(Rectangle r, float alpha = 0.88f);
void bar(Rectangle r, float fill, Color a, Color b, Color back = Color{20, 16, 22, 255});
void cursor(float x, float y, double time);   // panah kecil berkedip

// Warna palet TUMBAL
constexpr Color AMBER  = {232, 200, 140, 255};
constexpr Color BONE   = {226, 216, 196, 255};
constexpr Color DIM    = {150, 140, 128, 255};
constexpr Color BLOOD  = {200, 64, 56, 255};
constexpr Color HP_A   = {176, 52, 44, 255};
constexpr Color HP_B   = {232, 110, 72, 255};
constexpr Color MP_A   = {52, 92, 160, 255};
constexpr Color MP_B   = {92, 160, 232, 255};
constexpr Color AP_A   = {150, 120, 60, 255};
constexpr Color AP_B   = {240, 210, 120, 255};
constexpr Color CH_A   = {120, 60, 160, 255};
constexpr Color CH_B   = {210, 140, 255, 255};

}  // namespace ui
