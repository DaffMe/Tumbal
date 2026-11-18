#include "Unit.hpp"
#include <initializer_list>

float Unit::spd() const { return baseSpd * (1.0f + statusValue(StatusType::SpdUp)); }

float Unit::statusValue(StatusType t) const {
    float v = 0.0f;
    for (int i = 0; i < statusCount; ++i)
        if (statuses[i].type == t) v += statuses[i].value;
    return v;
}

bool Unit::hasStatus(StatusType t) const {
    for (int i = 0; i < statusCount; ++i)
        if (statuses[i].type == t) return true;
    return false;
}

void Unit::addStatus(StatusType t, int turns, float value) {
    for (int i = 0; i < statusCount; ++i) {
        if (statuses[i].type == t) {                 // refresh, ambil yang terkuat
            if (turns > statuses[i].turns) statuses[i].turns = turns;
            if (value > statuses[i].value) statuses[i].value = value;
            return;
        }
    }
    if (statusCount < MAX_STATUS) statuses[statusCount++] = Status{t, turns, value};
}

void Unit::removeStatusAt(int i) {
    if (i < 0 || i >= statusCount) return;
    for (int j = i; j < statusCount - 1; ++j) statuses[j] = statuses[j + 1];
    --statusCount;
}

// ---------------------------------------------------------------------------
// Roster
// ---------------------------------------------------------------------------

static void setStats(Unit& u, int hp, int atk, int def, float spd, int tough) {
    u.maxHp = u.hp = hp;
    u.atk = atk;
    u.def = def;
    u.baseSpd = spd;
    u.maxToughness = u.toughness = tough;
}

static void addSkill(Unit& u, SkillId id) {
    if (u.skillCount < MAX_SKILLS) u.skills[u.skillCount++] = &getSkill(id);
}

static void setPattern(Unit& u, std::initializer_list<int> p) {
    u.patternLen = 0;
    for (int v : p)
        if (u.patternLen < MAX_PATTERN) u.pattern[u.patternLen++] = v;
}

Unit makeHero(HeroKind kind) {
    Unit u;
    u.side = Side::Hero;
    u.kind = static_cast<int>(kind);
    switch (kind) {
        case HeroKind::Varga:
            u.name = "Varga";
            setStats(u, 260, 34, 40, 95.0f, 0);
            addSkill(u, SkillId::Cleave);
            addSkill(u, SkillId::ShieldBash);
            addSkill(u, SkillId::Bulwark);
            u.ultimate = &getSkill(SkillId::Oathbreaker);
            break;
        case HeroKind::Sable:
            u.name = "Sable";
            setStats(u, 170, 46, 18, 110.0f, 0);
            addSkill(u, SkillId::HexBolt);
            addSkill(u, SkillId::Unravel);
            addSkill(u, SkillId::CinderCurse);
            u.ultimate = &getSkill(SkillId::AshenRequiem);
            break;
        case HeroKind::Ione:
        default:
            u.name = "Ione";
            setStats(u, 190, 28, 24, 120.0f, 0);
            addSkill(u, SkillId::LanternStrike);
            addSkill(u, SkillId::Kindle);
            addSkill(u, SkillId::Mend);
            u.ultimate = &getSkill(SkillId::Dawnlight);
            break;
    }
    return u;
}

Unit makeEnemy(EnemyKind kind) {
    Unit u;
    u.side = Side::Enemy;
    u.kind = static_cast<int>(kind);
    switch (kind) {
        case EnemyKind::HollowThrall:
            u.name = "Hollow Thrall";
            setStats(u, 120, 26, 15, 90.0f, 30);
            addSkill(u, SkillId::Claw);
            addSkill(u, SkillId::Lunge);
            setPattern(u, {0, 0, 1});
            break;
        case EnemyKind::BellRinger:
            u.name = "Bell-Ringer";
            setStats(u, 140, 20, 20, 100.0f, 40);
            addSkill(u, SkillId::Mallet);
            addSkill(u, SkillId::DeathToll);
            setPattern(u, {0, 1, 0});
            break;
        case EnemyKind::CandleWraith:
            u.name = "Candle Wraith";
            setStats(u, 110, 30, 10, 105.0f, 30);
            addSkill(u, SkillId::WaxBurn);
            addSkill(u, SkillId::KindleWisp);
            setPattern(u, {1, 0, 0});
            break;
        case EnemyKind::Wisp:
            u.name = "Wisp";
            setStats(u, 40, 18, 0, 130.0f, 10);
            addSkill(u, SkillId::Flicker);
            setPattern(u, {0});
            break;
        case EnemyKind::Gargoyle:
            u.name = "Gargoyle";
            setStats(u, 260, 38, 50, 75.0f, 70);
            addSkill(u, SkillId::Crush);
            addSkill(u, SkillId::StoneSkin);
            setPattern(u, {0, 0, 1});
            break;
        case EnemyKind::ChoirWarden:
        default:
            u.name = "The Choir Warden";
            u.isBoss = true;
            setStats(u, 900, 40, 35, 100.0f, 120);
            addSkill(u, SkillId::HymnLash);
            addSkill(u, SkillId::Requiem);
            addSkill(u, SkillId::SummonChoir);
            addSkill(u, SkillId::Crescendo);   // khusus: dipakai otomatis saat HP < 50%
            setPattern(u, {0, 1, 2, 1, 0});
            break;
    }
    return u;
}
