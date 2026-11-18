#include "Skill.hpp"
#include "Kinds.hpp"

using TT = TargetType;
using ET = EffectType;

// Tabel skill. Urutan WAJIB sama dengan enum SkillId.
static const Skill kSkills[] = {
    // ------------------------------------------------------------- Varga
    {"Cleave", "Tebasan dasar. +1 Ember.",
     0, 1, TT::SingleEnemy, 10, false, 1, {{ET::Damage, 1.0f, 0}}},
    {"Shield Bash", "Hantam perisai dan tunda giliran target 25%.",
     1, 0, TT::SingleEnemy, 20, false, 2, {{ET::Damage, 0.8f, 0}, {ET::Delay, 0.25f, 0}}},
    {"Bulwark", "Taunt dan DEF +40% selama 2 giliran.",
     1, 0, TT::Self, 0, false, 2, {{ET::Taunt, 0.0f, 2}, {ET::DefUp, 0.4f, 2}}},
    {"Oathbreaker", "ULTIMATE: hantam semua musuh dan tunda giliran mereka 15%.",
     0, 0, TT::AllEnemies, 20, true, 2, {{ET::Damage, 1.4f, 0}, {ET::Delay, 0.15f, 0}}},

    // ------------------------------------------------------------- Sable
    {"Hex Bolt", "Sihir dasar. +1 Ember.",
     0, 1, TT::SingleEnemy, 10, false, 1, {{ET::Damage, 1.0f, 0}}},
    {"Unravel", "Urai benang waktu target: tunda giliran 40%.",
     2, 0, TT::SingleEnemy, 15, false, 2, {{ET::Damage, 0.6f, 0}, {ET::Delay, 0.40f, 0}}},
    {"Cinder Curse", "Bakar semua musuh (Burn 3 giliran).",
     1, 0, TT::AllEnemies, 5, false, 2, {{ET::Damage, 0.4f, 0}, {ET::Burn, 0.45f, 3}}},
    {"Ashen Requiem", "ULTIMATE: ledakan abu, target Vulnerable +25% selama 2 giliran.",
     0, 0, TT::SingleEnemy, 40, true, 2, {{ET::Damage, 2.4f, 0}, {ET::Vulnerable, 0.25f, 2}}},

    // -------------------------------------------------------------- Ione
    {"Lantern Strike", "Pukulan lentera. +1 Ember.",
     0, 1, TT::SingleEnemy, 10, false, 1, {{ET::Damage, 1.0f, 0}}},
    {"Kindle", "Majukan giliran sekutu 50% dan SPD +15% selama 2 giliran.",
     1, 0, TT::OtherAlly, 0, false, 2, {{ET::Advance, 0.5f, 0}, {ET::SpdUp, 0.15f, 2}}},
    {"Mend", "Pulihkan 30% HP maksimum sekutu.",
     1, 0, TT::SingleAlly, 0, false, 1, {{ET::Heal, 0.30f, 0}}},
    {"Dawnlight", "ULTIMATE: pulihkan 25% HP party, majukan giliran mereka 30%, +2 Ember.",
     0, 0, TT::AllAllies, 0, true, 3, {{ET::Heal, 0.25f, 0}, {ET::Advance, 0.30f, 0}, {ET::GainEmber, 2.0f, 0}}},

    // ------------------------------------------------------------- Musuh
    {"Claw", "Cakaran.",
     0, 0, TT::SingleEnemy, 0, false, 1, {{ET::Damage, 1.0f, 0}}},
    {"Lunge", "Terjangan liar.",
     0, 0, TT::SingleEnemy, 0, false, 1, {{ET::Damage, 1.3f, 0}}},
    {"Mallet", "Pukulan palu lonceng.",
     0, 0, TT::SingleEnemy, 0, false, 1, {{ET::Damage, 1.0f, 0}}},
    {"Death Toll", "Dentang lonceng: majukan giliran semua sekutunya 20%.",
     0, 0, TT::AllAllies, 0, false, 1, {{ET::Advance, 0.20f, 0}}},
    {"Wax Burn", "Lilin cair membakar target.",
     0, 0, TT::SingleEnemy, 0, false, 2, {{ET::Damage, 0.8f, 0}, {ET::Burn, 0.35f, 2}}},
    {"Kindle Wisp", "Memanggil Wisp ke medan pertempuran.",
     0, 0, TT::None, 0, false, 1, {{ET::Summon, static_cast<float>(EnemyKind::Wisp), 0}}},
    {"Flicker", "Sambaran api kecil.",
     0, 0, TT::SingleEnemy, 0, false, 1, {{ET::Damage, 0.9f, 0}}},
    {"Crush", "Hantaman batu.",
     0, 0, TT::SingleEnemy, 0, false, 1, {{ET::Damage, 1.4f, 0}}},
    {"Stone Skin", "Mengeraskan kulit: DEF +50% selama 2 giliran.",
     0, 0, TT::Self, 0, false, 1, {{ET::DefUp, 0.5f, 2}}},
    {"Hymn Lash", "Kidung yang mencambuk seluruh party.",
     0, 0, TT::AllEnemies, 0, false, 1, {{ET::Damage, 0.7f, 0}}},
    {"Requiem", "Nada kematian pada satu target.",
     0, 0, TT::SingleEnemy, 0, false, 1, {{ET::Damage, 1.6f, 0}}},
    {"Summon Choir", "Memanggil Hollow Thrall dari bangku gereja.",
     0, 0, TT::None, 0, false, 1, {{ET::Summon, static_cast<float>(EnemyKind::HollowThrall), 0}}},
    {"Crescendo", "Saat HP < 50%: bertindak lagi segera, SPD +20%.",
     0, 0, TT::Self, 0, false, 2, {{ET::ExtraTurn, 0.0f, 0}, {ET::SpdUp, 0.20f, 3}}},
};

static_assert(sizeof(kSkills) / sizeof(kSkills[0]) == static_cast<int>(SkillId::COUNT),
              "kSkills harus berisi tepat satu entri per SkillId");

const Skill& getSkill(SkillId id) { return kSkills[static_cast<int>(id)]; }
