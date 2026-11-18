#pragma once
// ============================================================================
// BattleEvent — catatan satu kejadian di battle.
// Logika C++ meng-enqueue event ke Battle Event Queue; antarmuka (console
// atau GUI) men-dequeue berurutan untuk ditampilkan/dianimasikan.
// Event DsOp mencatat operasi struktur data yang baru terjadi, untuk trace
// di console dan Inspector di GUI.
// ============================================================================

#include <string>

enum class EventType {
    BattleStart,
    TurnStart,
    SkillUsed,
    Damage,
    Heal,
    Break,
    Recover,
    AvShift,
    StatusApplied,
    StatusExpired,
    Burn,
    Death,
    Summon,
    Interject,
    ExtraTurn,
    Rewind,
    EmberChange,
    Info,
    Victory,
    Defeat,
    DsOp
};

struct BattleEvent {
    EventType   type   = EventType::Info;
    int         source = -1;   // indeks unit pelaku (-1 jika tidak ada)
    int         target = -1;   // indeks unit sasaran (-1 jika tidak ada)
    int         amount = 0;    // angka terkait (damage, heal, persen, ...)
    std::string text;          // deskripsi siap tampil (Bahasa Indonesia)
};
