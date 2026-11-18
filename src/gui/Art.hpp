#pragma once
// ============================================================================
// Art — menggambar karakter, latar, dan ikon.
// Jika file aset final ada (assets/final/...), tekstur itu yang dipakai;
// jika belum, digambar placeholder prosedural agar game tetap bisa dimainkan.
// ============================================================================

#include <string>
#include "raylib.h"
#include "core/battle/Kinds.hpp"
#include "core/dungeon/Floor.hpp"

namespace art {

// Ukuran tampil (lebar, tinggi) battler musuh dalam piksel virtual
Vector2 enemySize(EnemyKind k);

// Gambar musuh dengan titik tengah-bawah di `foot`. flash 0..1 = kilat putih saat terkena.
void drawEnemy(EnemyKind k, Vector2 foot, float alpha, float flash, float t, bool highlight);

// Potret hero di dalam kotak r
void drawPortrait(HeroKind k, Rectangle r, float alpha, bool active);

// Latar battle per lantai
void drawBattleBackground(int floorNo, float t);

// Latar peta
void drawMapBackground(float t);

// Ikon jenis ruangan di peta
void drawRoomIcon(RoomType type, Vector2 c, float r, Color tint);

// Latar layar judul
void drawTitleBackground(float t);

// Nama ringkas untuk token kecil ("Hollow Thrall A" -> "Thrall A")
std::string shortName(const std::string& name, size_t maxLen = 10);

const char* enemySlug(EnemyKind k);
const char* heroSlug(HeroKind k);

}  // namespace art
