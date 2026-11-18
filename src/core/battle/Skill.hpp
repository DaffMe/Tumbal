#pragma once
// ============================================================================
// Skill — data aksi yang bisa dipakai unit. Satu skill terdiri dari beberapa
// Effect yang dijalankan berurutan pada target-targetnya.
// ============================================================================

// Target relatif terhadap pihak pengguna skill.
enum class TargetType {
    SingleEnemy,   // satu unit pihak lawan
    AllEnemies,    // semua unit pihak lawan yang hidup
    SingleAlly,    // satu sekutu (termasuk diri sendiri)
    OtherAlly,     // satu sekutu selain diri sendiri
    Self,
    AllAllies,
    None           // tanpa target (mis. summon)
};

enum class EffectType {
    Damage,      // value = pengali ATK
    Delay,       // value = persen tunda giliran  (Timeline: node geser ke tail)
    Advance,     // value = persen majukan giliran (Timeline: node geser ke head)
    Heal,        // value = persen HP maksimum
    Burn,        // value = pengali ATK per tick, turns = durasi
    DefUp,
    SpdUp,
    Taunt,
    Vulnerable,
    GainEmber,   // sekali per cast: value = jumlah Ember
    Summon,      // sekali per cast: value = EnemyKind yang dipanggil
    ExtraTurn    // sekali per cast: pengguna bertindak lagi segera
};

struct Effect {
    EffectType type  = EffectType::Damage;
    float      value = 0.0f;
    int        turns = 0;
};

constexpr int MAX_EFFECTS = 3;

struct Skill {
    const char* name;
    const char* desc;
    int         emberCost;     // biaya Ember (sumber daya bersama party)
    int         emberGain;     // Ember yang didapat setelah dipakai
    TargetType  target;
    int         toughnessDmg;  // pengurang toughness musuh (mekanik Break)
    bool        isUltimate;
    int         effectCount;
    Effect      effects[MAX_EFFECTS];
    bool        lowHpSpecial = false;   // AI: dipakai sekali saat HP < 50%
};

enum class SkillId {
    // Varga
    Cleave, ShieldBash, Bulwark, Oathbreaker,
    // Sable
    HexBolt, Unravel, CinderCurse, AshenRequiem,
    // Ione
    LanternStrike, Kindle, Mend, Dawnlight,
    // Musuh
    Claw, Lunge, Mallet, DeathToll, WaxBurn, KindleWisp, Flicker,
    Crush, StoneSkin, HymnLash, Requiem, SummonChoir, Crescendo,
    // Boss lantai 2 & 3
    AshVeil, UrnStrike, Mourning, AshenRebirth,
    Conflagration, SearingHand, Pyre, InfernoAscendant,
    COUNT
};

const Skill& getSkill(SkillId id);

// Efek yang hanya dijalankan sekali per penggunaan skill (bukan per target).
inline bool isOncePerCast(EffectType t) {
    return t == EffectType::GainEmber || t == EffectType::Summon || t == EffectType::ExtraTurn;
}

inline bool needsTarget(TargetType t) {
    return t == TargetType::SingleEnemy || t == TargetType::SingleAlly || t == TargetType::OtherAlly;
}
