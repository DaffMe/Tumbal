#pragma once
// ============================================================================
// Battler — satu peserta battle (pemain, pendamping, atau musuh).
// Menyimpan HP/MP, status (sisa giliran per StatusId), dan keadaan rapal.
// Gauge AP tidak disimpan langsung: dihitung dari node Timeline
// (sisa waktu sampai giliran), lihat BattleSystem::gauge().
// ============================================================================

#include <string>
#include "../data/GameData.hpp"

struct Battler {
    std::string name;
    bool  enemy = false;
    int   slot = 0;                 // indeks di BattleSystem (0-3 party, 4-7 musuh)
    const EnemyDef* def = nullptr;  // hanya musuh
    Stats stats;
    int   hp = 1, mp = 0;
    int   status[ST_COUNT] = {};    // sisa giliran (0 = tidak aktif)
    float elementRate[EL_COUNT] = {1, 1, 1, 1, 1, 1, 1};
    int   skills[8] = {};
    int   skillCount = 0;

    // rapal (chant): aksi tertunda sampai gauge rapal penuh
    bool  chanting = false;
    int   chantSkill = 0;
    int   chantTarget = -1;
    float chantTicks = 0.0f;        // total waktu rapal (untuk gauge)
    float turnTicks = 0.0f;         // total waktu tunggu giliran saat ini (untuk gauge)

    bool alive() const { return hp > 0; }
    bool has(int st) const { return st > 0 && st < ST_COUNT && status[st] > 0; }

    // Laju pengisian AP per tick = AGI (dengan pengali status) + 10, minimal 5.
    float rate() const {
        float mul = 1.0f;
        for (int s = 1; s < ST_COUNT; ++s)
            if (status[s] > 0) mul *= statusDef(s).agiMul;
        float r = stats.agi * mul + 10.0f;
        return r < 5.0f ? 5.0f : r;
    }
};

// Buat battler pemain dari kelas + level.
Battler makeHeroBattler(const std::string& name, int classId, int level);
// Buat battler musuh dari id.
Battler makeEnemyBattler(int enemyId);
