#pragma once
// ============================================================================
// Unit — data satu petarung (hero atau musuh).
// Disalin utuh (by value) ke snapshot saat fitur rewind menyimpan state,
// sehingga semua field berupa nilai/array tetap, bukan container dinamis.
// ============================================================================

#include <string>
#include "Kinds.hpp"
#include "Skill.hpp"
#include "Status.hpp"

constexpr int MAX_SKILLS  = 4;
constexpr int MAX_STATUS  = 5;
constexpr int MAX_PATTERN = 6;

struct Unit {
    std::string name;
    Side  side   = Side::Enemy;
    int   kind   = 0;          // HeroKind atau EnemyKind (sesuai side)
    bool  isBoss = false;

    int   maxHp = 1, hp = 1;
    int   atk = 1, def = 0;
    float baseSpd = 100.0f;

    // Mekanik Break: toughness habis → giliran ditunda + menerima damage lebih
    int   maxToughness = 0, toughness = 0;
    bool  broken = false;

    // Hero: Resolve penuh (100) → bisa Interject (ultimate di luar giliran)
    int   resolve = 0;
    bool  empowered = false;        // giliran berikutnya wajib ultimate
    bool  interjectQueued = false;  // setelah giliran ini, langsung bertindak lagi
    bool  alive = true;

    const Skill* skills[MAX_SKILLS] = {};
    int          skillCount = 0;
    const Skill* ultimate = nullptr;

    // AI musuh: urutan indeks skill yang dipakai bergiliran
    int  pattern[MAX_PATTERN] = {};
    int  patternLen = 0;
    int  aiStep = 0;
    bool usedSpecial = false;

    Status statuses[MAX_STATUS] = {};
    int    statusCount = 0;

    float spd() const;                       // SPD efektif (dengan buff)
    float statusValue(StatusType t) const;   // total besaran status jenis t
    bool  hasStatus(StatusType t) const;
    void  addStatus(StatusType t, int turns, float value);  // refresh jika sudah ada
    void  removeStatusAt(int i);
};

Unit makeHero(HeroKind kind);
Unit makeEnemy(EnemyKind kind);
