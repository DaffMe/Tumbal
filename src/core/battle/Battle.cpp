#include "Battle.hpp"

#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace {

std::string fmt(const char* f, ...) __attribute__((format(printf, 1, 2)));
std::string fmt(const char* f, ...) {
    char buf[256];
    va_list args;
    va_start(args, f);
    std::vsnprintf(buf, sizeof buf, f, args);
    va_end(args);
    return buf;
}

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

}  // namespace

// ============================================================================
// Konstruksi
// ============================================================================

Battle::Battle(const Unit* heroes, int heroCount, const Unit* enemies, int enemyCount,
               std::uint32_t seed, int hourglassCharges, bool ambush)
    : unitCount_(0), ember_(START_EMBER), current_(-1), turn_(0),
      hourglass_(hourglassCharges), ambush_(ambush), state_(BattleState::Idle), rng_(seed) {
    for (int i = 0; i < heroCount && unitCount_ < MAX_UNITS; ++i) units_[unitCount_++] = heroes[i];
    for (int i = 0; i < enemyCount && unitCount_ < MAX_UNITS; ++i) units_[unitCount_++] = enemies[i];
}

Battle::~Battle() { clearHistory(); }

void Battle::start() {
    if (state_ != BattleState::Idle) return;
    emit(EventType::BattleStart, -1, -1, 0, "Pertarungan dimulai!");

    // Bangun timeline awal: setiap unit masuk sesuai AV dasarnya
    for (int i = 0; i < unitCount_; ++i) {
        Unit& u = units_[i];
        if (!u.alive) continue;
        float av = baseAV(u);
        timeline_.insertSorted(&u, av);
        dsOp(fmt("Timeline.insertSorted(%s, av=%.1f)", u.name.c_str(), av));
    }

    // Hubungan M2 → M1: lentera gelap → musuh menyergap (giliran dimajukan 30%)
    if (ambush_) {
        emit(EventType::Info, -1, -1, 0, "Kegelapan pekat... musuh menyergap!");
        for (int i = 0; i < unitCount_; ++i)
            if (units_[i].side == Side::Enemy) shiftActionValue(i, -0.30f);
    }

    state_ = BattleState::Running;
    advanceUntilPlayer();
}

// ============================================================================
// Alur giliran
// ============================================================================

void Battle::advanceUntilPlayer() {
    while (state_ == BattleState::Running) beginNextTurn();
}

// M1 subprogram: majukan waktu sampai unit terdepan siap bertindak.
// Semua AV dikurangi sebesar AV head (traversal), lalu head di-pop.
Unit* Battle::advanceTimeline() {
    Timeline<Unit>::Node* head = timeline_.front();
    if (head == nullptr) return nullptr;

    float elapsed = head->av;
    if (elapsed > 0.0f) {
        timeline_.subtractAll(elapsed);
        dsOp(fmt("Timeline.subtractAll(%.1f) - waktu berjalan", elapsed));
    }
    Unit* actor = nullptr;
    float av = 0.0f;
    timeline_.popFront(actor, av);
    dsOp(fmt("Timeline.popFront() -> %s", actor->name.c_str()));
    return actor;
}

void Battle::beginNextTurn() {
    Unit* actor = advanceTimeline();
    if (actor == nullptr) {
        checkBattleEnd();
        if (state_ == BattleState::Running) state_ = BattleState::Defeat;
        return;
    }

    current_ = indexOf(actor);
    ++turn_;
    emit(EventType::TurnStart, current_, -1, turn_, fmt("Giliran %d: %s", turn_, actor->name.c_str()));

    // Musuh yang Break pulih di awal gilirannya
    if (actor->broken) {
        actor->broken = false;
        actor->toughness = actor->maxToughness;
        emit(EventType::Recover, current_, -1, 0, fmt("%s pulih dari Break.", actor->name.c_str()));
    }

    tickStartOfTurn(current_);
    if (!actor->alive) {            // tumbang karena Burn
        current_ = -1;
        checkBattleEnd();
        return;
    }

    if (actor->side == Side::Enemy) {
        runEnemyTurn(current_);
        endTurn();
    } else {
        state_ = BattleState::WaitingForPlayer;
        saveTurnSnapshot();
    }
}

void Battle::endTurn() {
    if (current_ >= 0 && !isOver()) {
        Unit& u = units_[current_];
        if (u.alive) {
            decrementStatuses(current_);
            if (u.interjectQueued) {
                // Extra turn / Interject milik aktor sendiri: langsung ke depan
                u.interjectQueued = false;
                if (u.side == Side::Hero) u.empowered = true;
                timeline_.insertFront(&u, 0.0f);
                dsOp(fmt("Timeline.insertFront(%s) - extra turn", u.name.c_str()));
                emit(EventType::ExtraTurn, current_, -1, 0, fmt("%s akan bertindak lagi!", u.name.c_str()));
            } else {
                float av = baseAV(u);
                timeline_.insertSorted(&u, av);
                dsOp(fmt("Timeline.insertSorted(%s, av=%.1f)", u.name.c_str(), av));
            }
        }
    }
    current_ = -1;
    checkBattleEnd();
}

int Battle::aliveCount(Side side) const {
    int n = 0;
    for (int i = 0; i < unitCount_; ++i)
        if (units_[i].alive && units_[i].side == side) ++n;
    return n;
}

void Battle::checkBattleEnd() {
    if (isOver()) return;
    if (aliveCount(Side::Enemy) == 0) {
        state_ = BattleState::Victory;
        emit(EventType::Victory, -1, -1, 0, "Semua musuh tumbang. KEMENANGAN!");
    } else if (aliveCount(Side::Hero) == 0) {
        state_ = BattleState::Defeat;
        emit(EventType::Defeat, -1, -1, 0, "Party tumbang... nyala lentera padam.");
    }
}

// ============================================================================
// Aksi pemain
// ============================================================================

const Skill* Battle::skillAt(int slot) const {
    if (current_ < 0) return nullptr;
    const Unit& c = units_[current_];
    if (slot == ULT_SLOT) return c.ultimate;
    if (slot < 0 || slot >= c.skillCount) return nullptr;
    return c.skills[slot];
}

bool Battle::canUseSkill(int slot) const {
    if (state_ != BattleState::WaitingForPlayer) return false;
    const Skill* s = skillAt(slot);
    if (s == nullptr) return false;
    if (units_[current_].empowered != s->isUltimate) return false;   // empowered → wajib ultimate
    return ember_ >= s->emberCost;
}

bool Battle::isValidTarget(int casterIdx, TargetType type, int targetIdx) const {
    if (!needsTarget(type)) return true;
    if (targetIdx < 0 || targetIdx >= unitCount_) return false;
    const Unit& c = units_[casterIdx];
    const Unit& t = units_[targetIdx];
    if (!t.alive) return false;
    switch (type) {
        case TargetType::SingleEnemy: return t.side != c.side;
        case TargetType::SingleAlly:  return t.side == c.side;
        case TargetType::OtherAlly:   return t.side == c.side && targetIdx != casterIdx;
        default:                      return true;
    }
}

bool Battle::playerUseSkill(int slot, int targetIdx) {
    if (!canUseSkill(slot)) return false;
    const Skill* s = skillAt(slot);
    if (!isValidTarget(current_, s->target, targetIdx)) return false;

    state_ = BattleState::Running;
    executeSkill(current_, *s, targetIdx);
    endTurn();
    advanceUntilPlayer();
    return true;
}

bool Battle::canInterject(int heroIdx) const {
    if (state_ != BattleState::WaitingForPlayer) return false;
    if (heroIdx < 0 || heroIdx >= unitCount_) return false;
    const Unit& h = units_[heroIdx];
    return h.side == Side::Hero && h.alive && h.ultimate != nullptr && h.resolve >= MAX_RESOLVE &&
           !h.empowered && !h.interjectQueued;
}

// Interject: hero dengan Resolve penuh menyela urutan giliran.
// Node hero dihapus dari posisinya lalu disisipkan di HEAD timeline.
bool Battle::interject(int heroIdx) {
    if (!canInterject(heroIdx)) return false;
    Unit& h = units_[heroIdx];
    h.resolve = 0;

    if (heroIdx == current_) {
        // Hero yang sedang bertindak: selesai giliran ini, langsung ultimate
        h.interjectQueued = true;
        emit(EventType::Interject, heroIdx, -1, 0,
             fmt("%s mengumpulkan tekad: ultimate setelah giliran ini!", h.name.c_str()));
    } else {
        timeline_.remove(&h);
        dsOp(fmt("Timeline.remove(%s)", h.name.c_str()));
        timeline_.insertFront(&h, 0.0f);
        dsOp(fmt("Timeline.insertFront(%s) - interject", h.name.c_str()));
        h.empowered = true;
        emit(EventType::Interject, heroIdx, -1, 0,
             fmt("%s menyela urutan giliran! (INTERJECT)", h.name.c_str()));
    }
    return true;
}

// ============================================================================
// Eksekusi skill
// ============================================================================

int Battle::collectTargets(int casterIdx, TargetType type, int targetIdx, int out[]) const {
    const Unit& c = units_[casterIdx];
    int n = 0;
    switch (type) {
        case TargetType::SingleEnemy:
        case TargetType::SingleAlly:
        case TargetType::OtherAlly:
            out[n++] = targetIdx;
            break;
        case TargetType::Self:
            out[n++] = casterIdx;
            break;
        case TargetType::AllEnemies:
        case TargetType::AllAllies: {
            bool wantEnemy = (type == TargetType::AllEnemies);
            for (int i = 0; i < unitCount_; ++i) {
                if (!units_[i].alive) continue;
                bool isEnemyOfCaster = units_[i].side != c.side;
                if (isEnemyOfCaster == wantEnemy) out[n++] = i;
            }
            break;
        }
        case TargetType::None:
            break;
    }
    return n;
}

void Battle::executeSkill(int casterIdx, const Skill& skill, int targetIdx) {
    Unit& c = units_[casterIdx];
    std::string text = fmt("%s menggunakan %s", c.name.c_str(), skill.name);
    if (needsTarget(skill.target) && targetIdx >= 0 && targetIdx != casterIdx)
        text += fmt(" pada %s", units_[targetIdx].name.c_str());
    emit(EventType::SkillUsed, casterIdx, targetIdx, 0, text + ".");

    if (c.side == Side::Hero) {
        if (skill.emberCost > 0) changeEmber(-skill.emberCost);
        if (skill.emberGain > 0) changeEmber(skill.emberGain);
        if (skill.isUltimate) {
            c.resolve = 0;
            c.empowered = false;
        } else {
            c.resolve = clampInt(c.resolve + 20, 0, MAX_RESOLVE);
        }
    }

    int targets[MAX_UNITS];
    int n = collectTargets(casterIdx, skill.target, targetIdx, targets);
    bool toughDone[MAX_UNITS] = {};

    for (int k = 0; k < skill.effectCount; ++k) {
        const Effect& e = skill.effects[k];
        if (isOncePerCast(e.type)) {
            applyEffect(casterIdx, casterIdx, skill, e, toughDone);
        } else {
            for (int i = 0; i < n; ++i)
                if (units_[targets[i]].alive) applyEffect(casterIdx, targets[i], skill, e, toughDone);
        }
        if (isOver()) break;
    }
}

void Battle::applyEffect(int casterIdx, int targetIdx, const Skill& skill, const Effect& e,
                         bool toughDone[]) {
    Unit& c = units_[casterIdx];
    Unit& t = units_[targetIdx];

    switch (e.type) {
        case EffectType::Damage: {
            applyDamage(casterIdx, targetIdx, computeDamage(c, t, e.value), nullptr);
            // Mekanik Break: kurangi toughness sekali per target per skill
            if (t.alive && t.maxToughness > 0 && !t.broken && skill.toughnessDmg > 0 && !toughDone[targetIdx]) {
                toughDone[targetIdx] = true;
                t.toughness -= skill.toughnessDmg;
                if (t.toughness <= 0) {
                    t.toughness = 0;
                    t.broken = true;
                    emit(EventType::Break, casterIdx, targetIdx, 0, fmt("BREAK! Pertahanan %s hancur.", t.name.c_str()));
                    int breakDmg = static_cast<int>(std::lround(c.atk * 0.5f * (0.9f + 0.2f * rng_.unit())));
                    applyDamage(casterIdx, targetIdx, breakDmg < 1 ? 1 : breakDmg, "break");
                    if (t.alive) shiftActionValue(targetIdx, 0.25f);
                }
            }
            break;
        }
        case EffectType::Delay:
            shiftActionValue(targetIdx, e.value);
            break;
        case EffectType::Advance:
            shiftActionValue(targetIdx, -e.value);
            break;
        case EffectType::Heal: {
            int amount = static_cast<int>(std::lround(t.maxHp * e.value));
            int before = t.hp;
            t.hp = clampInt(t.hp + amount, 0, t.maxHp);
            emit(EventType::Heal, casterIdx, targetIdx, t.hp - before,
                 fmt("%s pulih %d HP (HP %d/%d).", t.name.c_str(), t.hp - before, t.hp, t.maxHp));
            break;
        }
        case EffectType::Burn: {
            float perTick = c.atk * e.value;
            t.addStatus(StatusType::Burn, e.turns, perTick);
            emit(EventType::StatusApplied, casterIdx, targetIdx, e.turns,
                 fmt("%s terbakar (Burn %d giliran).", t.name.c_str(), e.turns));
            break;
        }
        case EffectType::DefUp:
            t.addStatus(StatusType::DefUp, e.turns, e.value);
            emit(EventType::StatusApplied, casterIdx, targetIdx, e.turns,
                 fmt("DEF %s naik %d%%.", t.name.c_str(), static_cast<int>(e.value * 100)));
            break;
        case EffectType::SpdUp: {
            float oldSpd = t.spd();
            t.addStatus(StatusType::SpdUp, e.turns, e.value);
            rescaleAV(targetIdx, oldSpd);
            emit(EventType::StatusApplied, casterIdx, targetIdx, e.turns,
                 fmt("SPD %s naik %d%%.", t.name.c_str(), static_cast<int>(e.value * 100)));
            break;
        }
        case EffectType::Taunt:
            t.addStatus(StatusType::Taunt, e.turns, 0.0f);
            emit(EventType::StatusApplied, casterIdx, targetIdx, e.turns,
                 fmt("%s menantang musuh (Taunt).", t.name.c_str()));
            break;
        case EffectType::Vulnerable:
            t.addStatus(StatusType::Vulnerable, e.turns, e.value);
            emit(EventType::StatusApplied, casterIdx, targetIdx, e.turns,
                 fmt("%s rentan: damage diterima +%d%%.", t.name.c_str(), static_cast<int>(e.value * 100)));
            break;
        case EffectType::GainEmber:
            changeEmber(static_cast<int>(e.value));
            break;
        case EffectType::Summon: {
            EnemyKind kind = static_cast<EnemyKind>(static_cast<int>(e.value));
            if (spawnUnit(makeEnemy(kind)) < 0)
                emit(EventType::Info, casterIdx, -1, 0, "Panggilan gagal: medan sudah penuh.");
            break;
        }
        case EffectType::ExtraTurn:
            c.interjectQueued = true;
            break;
    }
}

int Battle::computeDamage(const Unit& attacker, const Unit& defender, float power) {
    float raw  = attacker.atk * power;
    float defv = defender.def * (1.0f + defender.statusValue(StatusType::DefUp));
    float dmg  = raw * 100.0f / (100.0f + defv);
    if (defender.broken) dmg *= 1.2f;
    dmg *= 1.0f + defender.statusValue(StatusType::Vulnerable);
    dmg *= 0.9f + 0.2f * rng_.unit();    // variasi ±10%
    int d = static_cast<int>(std::lround(dmg));
    return d < 1 ? 1 : d;
}

void Battle::applyDamage(int sourceIdx, int targetIdx, int amount, const char* cause) {
    Unit& t = units_[targetIdx];
    if (!t.alive) return;
    t.hp = clampInt(t.hp - amount, 0, t.maxHp);
    if (t.side == Side::Hero) t.resolve = clampInt(t.resolve + 8, 0, MAX_RESOLVE);

    std::string text = fmt("%s menerima %d damage", t.name.c_str(), amount);
    if (cause != nullptr) text += fmt(" (%s)", cause);
    text += fmt(" - HP %d/%d.", t.hp, t.maxHp);
    emit(EventType::Damage, sourceIdx, targetIdx, amount, text);

    if (t.hp == 0) handleUnitDeath(targetIdx);
}

// M1 subprogram: unit tumbang → semua node miliknya dihapus dari Timeline.
void Battle::handleUnitDeath(int idx) {
    Unit& u = units_[idx];
    if (!u.alive) return;
    u.alive = false;
    u.hp = 0;
    u.statusCount = 0;
    u.empowered = u.interjectQueued = false;
    emit(EventType::Death, -1, idx, 0, fmt("%s tumbang!", u.name.c_str()));
    if (timeline_.remove(&u)) dsOp(fmt("Timeline.remove(%s) - unit tumbang", u.name.c_str()));
    checkBattleEnd();
}

void Battle::changeEmber(int delta) {
    int before = ember_;
    ember_ = clampInt(ember_ + delta, 0, MAX_EMBER);
    if (ember_ != before)
        emit(EventType::EmberChange, -1, -1, ember_ - before, fmt("Ember: %d -> %d", before, ember_));
}

// ============================================================================
// M1: manipulasi Action Value
// ============================================================================

// Delay (percent > 0) atau advance (percent < 0). Besaran dihitung dari AV
// dasar unit, lalu node unit dipindah lewat Timeline::updateAV (relink).
bool Battle::shiftActionValue(int unitIdx, float percent) {
    Unit& u = units_[unitIdx];
    Timeline<Unit>::Node* node = timeline_.search(&u);
    if (node == nullptr) return false;   // aktor yang sedang bertindak tidak ada di timeline

    float before = node->av;
    float after  = before + baseAV(u) * percent;
    if (after < 0.0f) after = 0.0f;
    timeline_.updateAV(&u, after);

    int pct = static_cast<int>(std::lround(std::fabs(percent) * 100.0f));
    emit(EventType::AvShift, -1, unitIdx, percent > 0 ? pct : -pct,
         fmt("Giliran %s %s %d%%.", u.name.c_str(), percent > 0 ? "ditunda" : "dimajukan", pct));
    dsOp(fmt("Timeline.updateAV(%s, %.1f -> %.1f) - node direlink", u.name.c_str(), before, after));
    return true;
}

// SPD berubah → sisa AV diskalakan agar progres giliran tetap proporsional.
void Battle::rescaleAV(int idx, float oldSpd) {
    Unit& u = units_[idx];
    Timeline<Unit>::Node* node = timeline_.search(&u);
    if (node == nullptr) return;
    float before = node->av;
    float after  = before * oldSpd / u.spd();
    timeline_.updateAV(&u, after);
    dsOp(fmt("Timeline.updateAV(%s, %.1f -> %.1f) - SPD berubah", u.name.c_str(), before, after));
}

// M1 subprogram: prediksi urutan n giliran ke depan.
// Bekerja pada SALINAN timeline (deep copy) sehingga timeline asli tidak berubah.
int Battle::predictTurnOrder(int n, int outIdx[]) {
    Timeline<Unit> sim(timeline_);
    // aktor yang sedang memilih aksi belum ada di timeline → simulasikan kembalinya
    if (current_ >= 0 && units_[current_].alive) {
        Unit* cur = &units_[current_];
        if (cur->interjectQueued) sim.insertFront(cur, 0.0f);
        else sim.insertSorted(cur, baseAV(*cur));
    }

    int count = 0;
    while (count < n && !sim.empty()) {
        float elapsed = sim.front()->av;
        sim.subtractAll(elapsed);
        Unit* u = nullptr;
        float av = 0.0f;
        sim.popFront(u, av);
        outIdx[count++] = indexOf(u);
        sim.insertSorted(u, baseAV(*u));
    }
    return count;
}

int Battle::spawnUnit(const Unit& tmpl) {
    if (aliveCount(Side::Enemy) >= MAX_ENEMIES_ALIVE) return -1;

    // Pakai ulang slot musuh yang sudah tumbang, atau tambah slot baru
    int slot = -1;
    for (int i = 0; i < unitCount_; ++i)
        if (!units_[i].alive && units_[i].side == Side::Enemy) { slot = i; break; }
    if (slot < 0) {
        if (unitCount_ >= MAX_UNITS) return -1;
        slot = unitCount_++;
    }

    units_[slot] = tmpl;
    float av = baseAV(units_[slot]);
    timeline_.insertSorted(&units_[slot], av);
    emit(EventType::Summon, -1, slot, 0, fmt("%s muncul!", units_[slot].name.c_str()));
    dsOp(fmt("Timeline.insertSorted(%s, av=%.1f) - summon", units_[slot].name.c_str(), av));
    return slot;
}

// ============================================================================
// Status effect
// ============================================================================

void Battle::tickStartOfTurn(int idx) {
    Unit& u = units_[idx];
    for (int i = 0; i < u.statusCount && u.alive; ++i) {
        if (u.statuses[i].type == StatusType::Burn) {
            int dmg = static_cast<int>(std::lround(u.statuses[i].value));
            applyDamage(-1, idx, dmg < 1 ? 1 : dmg, "Burn");
        }
    }
}

void Battle::decrementStatuses(int idx) {
    Unit& u = units_[idx];
    for (int i = u.statusCount - 1; i >= 0; --i) {
        if (--u.statuses[i].turns <= 0) {
            StatusType type = u.statuses[i].type;
            u.removeStatusAt(i);
            emit(EventType::StatusExpired, -1, idx, 0,
                 fmt("Efek %s pada %s berakhir.", statusName(type), u.name.c_str()));
        }
    }
}

// ============================================================================
// AI musuh (sederhana, berbasis pola)
// ============================================================================

void Battle::runEnemyTurn(int idx) {
    Unit& e = units_[idx];
    const Skill* s = chooseEnemySkill(e);
    if (s == nullptr) return;
    executeSkill(idx, *s, chooseEnemyTarget(idx, *s));
}

const Skill* Battle::chooseEnemySkill(Unit& e) {
    // Skill khusus (ExtraTurn) dipakai sekali saat HP di bawah 50%
    for (int i = 0; i < e.skillCount; ++i) {
        const Skill* s = e.skills[i];
        if (s->effects[0].type == EffectType::ExtraTurn && !e.usedSpecial && e.hp * 2 < e.maxHp) {
            e.usedSpecial = true;
            return s;
        }
    }
    // Ikuti pola; lewati summon bila medan penuh
    for (int tries = 0; tries < e.patternLen; ++tries) {
        const Skill* s = e.skills[e.pattern[e.aiStep % e.patternLen]];
        ++e.aiStep;
        if (s->effects[0].type == EffectType::ExtraTurn) continue;
        if (s->effects[0].type == EffectType::Summon && aliveCount(Side::Enemy) >= MAX_ENEMIES_ALIVE - 1) continue;
        return s;
    }
    return e.skillCount > 0 ? e.skills[0] : nullptr;
}

int Battle::chooseEnemyTarget(int idx, const Skill& s) {
    if (!needsTarget(s.target)) return -1;
    if (s.target != TargetType::SingleEnemy) return idx;   // skill sekutu → diri sendiri

    // Hero dengan Taunt wajib diserang
    for (int i = 0; i < unitCount_; ++i)
        if (units_[i].alive && units_[i].side == Side::Hero && units_[i].hasStatus(StatusType::Taunt)) return i;

    int heroes[MAX_UNITS];
    int n = 0;
    for (int i = 0; i < unitCount_; ++i)
        if (units_[i].alive && units_[i].side == Side::Hero) heroes[n++] = i;
    if (n == 0) return -1;

    if (rng_.chance(0.35f)) {   // kadang memburu hero dengan HP terendah
        int best = heroes[0];
        for (int i = 1; i < n; ++i)
            if (units_[heroes[i]].hp < units_[best].hp) best = heroes[i];
        return best;
    }
    return heroes[rng_.range(0, n - 1)];
}

// ============================================================================
// M3: Cracked Hourglass (Stack snapshot)
// ============================================================================

// M3 subprogram: simpan state di awal setiap giliran hero.
void Battle::saveTurnSnapshot() {
    Snapshot* s = new Snapshot();
    for (int i = 0; i < unitCount_; ++i) s->units[i] = units_[i];
    s->unitCount = unitCount_;
    s->timeline  = timeline_;           // deep copy node DLL
    s->ember     = ember_;
    s->current   = current_;
    s->turn      = turn_;
    s->rng       = rng_;
    history_.push(s);
    dsOp(fmt("Stack.push(snapshot giliran %d) - isi stack: %d", turn_, historySize()));
}

void Battle::restore(const Snapshot& s) {
    for (int i = 0; i < s.unitCount; ++i) units_[i] = s.units[i];
    unitCount_ = s.unitCount;
    timeline_  = s.timeline;            // pointer di snapshot menunjuk ke units_ yang sama
    ember_     = s.ember;
    current_   = s.current;
    turn_      = s.turn;
    rng_       = s.rng;
}

// M3 subprogram: kembali ke awal giliran hero SEBELUMNYA.
// Puncak stack = giliran yang sedang berjalan → dibuang; snapshot di bawahnya
// dipulihkan (dan tetap di stack karena kini menjadi giliran yang berjalan).
bool Battle::rewindTurn() {
    if (state_ != BattleState::WaitingForPlayer || hourglass_ <= 0 || history_.size() < 2) return false;

    Snapshot* currentSnap = nullptr;
    history_.pop(currentSnap);
    delete currentSnap;
    dsOp(fmt("Stack.pop() - buang snapshot giliran %d", turn_));

    Snapshot* prev = nullptr;
    history_.peek(prev);
    restore(*prev);
    --hourglass_;
    state_ = BattleState::WaitingForPlayer;

    emit(EventType::Rewind, current_, -1, turn_,
         fmt("Pasir jam berbalik... kembali ke giliran %d (%s). Hourglass tersisa %d.",
             turn_, units_[current_].name.c_str(), hourglass_));
    dsOp(fmt("Stack.peek() -> pulihkan snapshot giliran %d - isi stack: %d", turn_, historySize()));
    return true;
}

void Battle::clearHistory() {
    Snapshot* s = nullptr;
    while (history_.pop(s)) delete s;
}

// ============================================================================
// Event
// ============================================================================

void Battle::emit(EventType type, int src, int dst, int amount, const std::string& text) {
    events_.enqueue(BattleEvent{type, src, dst, amount, text});
}

void Battle::dsOp(const std::string& text) { emit(EventType::DsOp, -1, -1, 0, text); }
