#include "BattleSystem.hpp"

#include <cmath>
#include <cstdio>
#include "../util/DsTrace.hpp"

// ------------------------------------------------------------- pembuat battler

Battler makeHeroBattler(const std::string& name, int classId, int level) {
    Battler b;
    b.name = name;
    b.enemy = false;
    b.stats = statsAtLevel(classId, level);
    b.hp = b.stats.mhp;
    b.mp = b.stats.mmp;
    const ClassDef& c = classDef(classId);
    for (int i = 0; i < c.skillCount && i < 8; ++i) b.skills[b.skillCount++] = c.skills[i];
    return b;
}

Battler makeEnemyBattler(int enemyId) {
    Battler b;
    const EnemyDef* d = enemyDef(enemyId);
    if (d == nullptr) return b;
    b.name = d->name;
    b.enemy = true;
    b.def = d;
    b.stats = d->stats;
    b.hp = d->stats.mhp;
    b.mp = d->stats.mmp;
    for (int e = 0; e < EL_COUNT; ++e) b.elementRate[e] = d->elementRate[e];
    for (int i = 0; i < d->actionCount; ++i) b.skills[b.skillCount++] = d->actions[i].skill;
    return b;
}

// ------------------------------------------------------------- util

static std::string fmt(const char* f, const std::string& a, int v = 0) {
    char buf[200];
    std::snprintf(buf, sizeof buf, f, a.c_str(), v);
    return buf;
}

void BattleSystem::emit(BattleEvent::Kind k, int src, int dst, int value, const std::string& text, int skill, int status) {
    BattleEvent e;
    e.kind = k;
    e.src = src;
    e.dst = dst;
    e.value = value;
    e.text = text;
    e.skill = skill;
    e.status = status;
    events_.enqueue(e);
}

void BattleSystem::ds(const std::string& op) { dsOp(op); }

// ------------------------------------------------------------- setup

void BattleSystem::setup(const Battler* party, int partyCount, int troopId, Initiative init, std::uint32_t seed) {
    timeline_.clear();
    events_.clear();
    rng_ = Rng(seed);
    phase_ = Filling;
    inputActor_ = -1;
    sukma_ = 0;
    escapeFails_ = 0;
    count_ = 0;
    partyCount_ = partyCount > MAX_PARTY ? MAX_PARTY : partyCount;
    for (int i = 0; i < partyCount_; ++i) {
        b_[count_] = party[i];
        b_[count_].slot = count_;
        b_[count_].chanting = false;
        ++count_;
    }
    const TroopDef* t = troopDef(troopId);
    if (t != nullptr) {
        for (int i = 0; i < t->count && i < MAX_ENEMY; ++i) {
            b_[count_] = makeEnemyBattler(t->members[i]);
            b_[count_].slot = count_;
            ++count_;
        }
        // nama ganda diberi akhiran A, B, C
        for (int i = partyCount_; i < count_; ++i) {
            int same = 0;
            for (int j = partyCount_; j < count_; ++j) if (b_[j].def == b_[i].def) ++same;
            if (same > 1) {
                int k = 0;
                for (int j = partyCount_; j < i; ++j) if (b_[j].def == b_[i].def) ++k;
                b_[i].name += std::string(" ") + static_cast<char>('A' + k);
            }
        }
    }
    // AP awal acak seperti BS1: normal 30-70%, menyerang duluan 40-70%,
    // disergap 0-10%.
    for (int i = 0; i < count_; ++i) {
        Battler& b = b_[i];
        if (!b.alive()) continue;
        int lo = 30, span = 40;
        bool favored = (init == INIT_PREEMPTIVE && !b.enemy) || (init == INIT_SURPRISE && b.enemy);
        bool hindered = (init == INIT_PREEMPTIVE && b.enemy) || (init == INIT_SURPRISE && !b.enemy);
        if (favored) { lo = 40; span = 30; }
        if (hindered) { lo = 0; span = 10; }
        float pct = (lo + rng_.range(0, span)) / 100.0f;
        b.turnTicks = MAX_AP / b.rate();
        timeline_.insertSorted(&b, MAX_AP * (1.0f - pct) / b.rate());
        ds("Timeline.insertSorted(" + b.name + ")");
    }
    if (init == INIT_PREEMPTIVE) emit(BattleEvent::Message, -1, -1, 0, "Kamu menyerang lebih dulu!");
    if (init == INIT_SURPRISE) emit(BattleEvent::Message, -1, -1, 0, "Kamu disergap!");
}

// ------------------------------------------------------------- waktu

int BattleSystem::advanceTime(float ticks) {
    if (phase_ == Input) return inputActor_;
    if (phase_ != Filling) return -1;
    while (!timeline_.empty()) {
        auto* head = timeline_.front();
        if (head->av <= 0.0001f) return onReady(*head->item);
        if (ticks <= 0.0f) break;
        float delta = head->av < ticks ? head->av : ticks;
        timeline_.subtractAll(delta);
        ticks -= delta;
    }
    return -1;
}

int BattleSystem::advanceToNextActor() {
    if (phase_ != Filling || timeline_.empty()) return phase_ == Input ? inputActor_ : -1;
    float delta = timeline_.front()->av;
    timeline_.subtractAll(delta);
    ds("Timeline.subtractAll(" + std::to_string(static_cast<int>(delta)) + " tick) -> giliran " + timeline_.front()->item->name);
    return onReady(*timeline_.front()->item);
}

float BattleSystem::gauge(int i) const {
    const Battler& b = b_[i];
    auto* n = timeline_.search(const_cast<Battler*>(&b));
    if (n == nullptr) return 0.0f;
    if (b.chanting) return b.chantTicks > 0 ? 1.0f - n->av / b.chantTicks : 1.0f;
    float full = MAX_AP / b.rate();
    float g = 1.0f - n->av / full;
    return g < 0 ? 0 : (g > 1 ? 1 : g);
}

void BattleSystem::scheduleFullTurn(Battler& b) {
    if (!b.alive()) return;
    b.turnTicks = MAX_AP / b.rate();
    timeline_.updateAV(&b, b.turnTicks);
    ds("Timeline.updateAV(" + b.name + ", " + std::to_string(static_cast<int>(b.turnTicks)) + ")");
}

int BattleSystem::predictTurns(int* out, int n) const {
    Timeline<Battler> sim(timeline_);   // salinan: list asli tidak berubah
    int k = 0;
    while (k < n && !sim.empty()) {
        auto* head = sim.front();
        Battler* b = head->item;
        out[k++] = b->slot;
        sim.subtractAll(head->av);
        sim.updateAV(b, MAX_AP / b->rate());
    }
    return k;
}

// ------------------------------------------------------------- giliran

int BattleSystem::onReady(Battler& b) {
    emit(BattleEvent::TurnStart, b.slot, -1);
    if (b.chanting) {
        // rapal selesai → aksi keluar
        BattleAction a{b.chantSkill, b.chantTarget};
        b.chanting = false;
        resolveAction(b, a);
        scheduleFullTurn(b);
        checkEnd();
        return b.slot;
    }
    if (!startOfTurn(b)) {
        scheduleFullTurn(b);
        checkEnd();
        return b.slot;
    }
    if (b.enemy) {
        act(b, chooseEnemyAction(b));
        checkEnd();
        return b.slot;
    }
    phase_ = Input;
    inputActor_ = b.slot;
    return b.slot;
}

bool BattleSystem::startOfTurn(Battler& b) {
    // status reaktif berakhir saat giliran pemiliknya tiba
    if (b.has(ST_BERTAHAN)) removeStatus(b, ST_BERTAHAN);
    if (b.has(ST_ELAK)) removeStatus(b, ST_ELAK);
    if (b.has(ST_TANGKIS)) removeStatus(b, ST_TANGKIS);

    bool skip = false;
    for (int s = 1; s < ST_COUNT; ++s) {
        if (b.status[s] <= 0) continue;
        const StatusDef& d = statusDef(s);
        if (d.dotPct > 0.0f) {
            int dmg = static_cast<int>(b.stats.mhp * d.dotPct);
            if (dmg < 1) dmg = 1;
            b.hp -= dmg;
            emit(BattleEvent::Damage, -1, b.slot, dmg, b.name + " menderita karena " + d.name + ".", 0, s);
            if (b.hp <= 0) { kill(b); return false; }
        }
        if (d.skipTurn) skip = true;
    }
    if (skip) emit(BattleEvent::Message, b.slot, b.slot, 0, fmt("%s tidak bisa bergerak!", b.name));
    // kurangi durasi
    for (int s = 1; s < ST_COUNT; ++s) {
        if (b.status[s] <= 0) continue;
        if (--b.status[s] == 0) removeStatus(b, s);
    }
    return !skip && b.alive();
}

bool BattleSystem::canUse(int battler, int skill) const {
    if (battler < 0 || battler >= count_) return false;
    const Battler& b = b_[battler];
    const SkillDef* s = skillDef(skill);
    if (s == nullptr || !b.alive()) return false;
    if (b.mp < s->mpCost) return false;
    if (s->type == SK_ILMU && b.has(ST_BISU)) return false;
    return true;
}

bool BattleSystem::validTarget(int user, int skill, int target) const {
    const SkillDef* s = skillDef(skill);
    if (s == nullptr) return false;
    if (s->scope == SC_DIRI || s->scope == SC_MUSUH_SEMUA || s->scope == SC_KAWAN_SEMUA) return true;
    if (target < 0 || target >= count_ || !b_[target].alive()) return false;
    bool opposing = b_[target].enemy != b_[user].enemy;
    return s->scope == SC_MUSUH_SATU ? opposing : !opposing;
}

int BattleSystem::firstTarget(int user, int skill) const {
    for (int i = 0; i < count_; ++i)
        if (validTarget(user, skill, i)) return i;
    return -1;
}

bool BattleSystem::submit(const BattleAction& action) {
    if (phase_ != Input || inputActor_ < 0) return false;
    if (!canUse(inputActor_, action.skill)) return false;
    if (!validTarget(inputActor_, action.skill, action.target)) return false;
    Battler& user = b_[inputActor_];
    phase_ = Filling;
    inputActor_ = -1;
    act(user, action);
    checkEnd();
    return true;
}

void BattleSystem::act(Battler& user, const BattleAction& a) {
    const SkillDef* s = skillDef(a.skill);
    if (s == nullptr) { scheduleFullTurn(user); return; }
    user.mp -= s->mpCost;
    if (s->chant > 0) {
        startChant(user, a);
        return;
    }
    resolveAction(user, a);
    scheduleFullTurn(user);
}

// M1 subprogram: mulai merapal. Battler tetap di Timeline, tetapi waktu
// tunggunya diganti dengan waktu rapal (node dipindah lewat updateAV).
void BattleSystem::startChant(Battler& user, const BattleAction& a) {
    const SkillDef* s = skillDef(a.skill);
    user.chanting = true;
    user.chantSkill = a.skill;
    user.chantTarget = a.target;
    user.chantTicks = s->chant / user.rate();
    timeline_.updateAV(&user, user.chantTicks);
    ds("Timeline.updateAV(" + user.name + ", rapal " + std::to_string(static_cast<int>(user.chantTicks)) + ")");
    emit(BattleEvent::ChantStart, user.slot, a.target, 0,
         user.name + (user.enemy ? " bersiap: " : " merapal ") + s->name + "...", a.skill);
}

// M1 subprogram: dorong giliran target mundur sebesar pct% dari satu giliran.
// Target yang sedang merapal kehilangan rapalnya (rapal buyar).
void BattleSystem::applyDelay(Battler& t, int pct) {
    if (!t.alive() || pct <= 0) return;
    auto* n = timeline_.search(&t);
    if (n == nullptr) return;
    float add = MAX_AP * pct / 100.0f / t.rate();
    if (t.chanting) {
        t.chanting = false;
        emit(BattleEvent::ChantBroken, -1, t.slot, 0, t.name + ": rapal buyar!");
        timeline_.updateAV(&t, MAX_AP / t.rate() * 0.5f + add);
    } else {
        timeline_.updateAV(&t, n->av + add);
    }
    ds("Timeline.updateAV(" + t.name + ", +" + std::to_string(pct) + "% giliran) -> node digeser ke belakang");
    emit(BattleEvent::Delay, -1, t.slot, pct, t.name + " terhuyung!");
}

// M1 subprogram: jalankan efek jurus ke semua target sesuai scope.
void BattleSystem::resolveAction(Battler& user, const BattleAction& a) {
    const SkillDef* s = skillDef(a.skill);
    if (s == nullptr || !user.alive()) return;
    emit(BattleEvent::UseSkill, user.slot, a.target, 0, user.name + ": " + s->name, a.skill);

    if (s->scope == SC_DIRI) {
        if (s->status != ST_NONE) addStatus(user, s->status);
        return;
    }
    // kumpulkan target
    int targets[MAX_BATTLERS];
    int nt = 0;
    bool wantEnemySide = (s->scope == SC_MUSUH_SATU || s->scope == SC_MUSUH_SEMUA) ? !user.enemy : user.enemy;
    if (s->scope == SC_MUSUH_SATU || s->scope == SC_KAWAN_SATU) {
        int t = a.target;
        if (t < 0 || t >= count_ || !b_[t].alive() || b_[t].enemy != wantEnemySide) {
            // target asli sudah tumbang → alihkan ke yang masih hidup
            t = -1;
            for (int i = 0; i < count_; ++i)
                if (b_[i].alive() && b_[i].enemy == wantEnemySide) { t = i; break; }
        }
        if (t >= 0) targets[nt++] = t;
    } else {
        for (int i = 0; i < count_; ++i)
            if (b_[i].alive() && b_[i].enemy == wantEnemySide) targets[nt++] = i;
    }
    for (int k = 0; k < nt; ++k) {
        for (int h = 0; h < s->hits; ++h) {
            if (!user.alive() || !b_[targets[k]].alive()) break;
            hit(user, b_[targets[k]], *s);
        }
    }
}

int BattleSystem::computeDamage(const Battler& u, const Battler& t, const SkillDef& s, bool& crit) {
    float raw = 0.0f;
    if (s.formula == FM_FISIK) raw = (u.stats.atk * 4.0f - t.stats.def * 2.0f) * s.mul;
    else if (s.formula == FM_ILMU) raw = (s.base + u.stats.mag * 5.0f - t.stats.res * 2.0f) * s.mul;
    if (raw < 1.0f) raw = 1.0f;
    raw *= t.elementRate[s.element >= 0 && s.element < EL_COUNT ? s.element : 0];
    // variance ±20% seperti VX Ace
    raw *= 0.8f + rng_.unit() * 0.4f;
    crit = false;
    if (s.formula == FM_FISIK) {
        float critChance = 0.05f + (u.stats.luk - t.stats.luk) * 0.002f;
        if (rng_.chance(critChance)) { crit = true; raw *= 2.0f; }
        if (t.has(ST_BERDARAH)) raw *= 1.5f;
    }
    for (int st = 1; st < ST_COUNT; ++st)
        if (t.status[st] > 0) raw *= statusDef(st).dmgTakenMul;
    int d = static_cast<int>(std::lround(raw));
    return d < 1 ? 1 : d;
}

void BattleSystem::hit(Battler& user, Battler& target, const SkillDef& s) {
    bool offensive = (user.enemy != target.enemy);
    if (s.formula == FM_PULIH) {
        int amt = static_cast<int>((s.base + user.stats.mag * 3.0f) * (0.9f + rng_.unit() * 0.2f));
        int before = target.hp;
        target.hp += amt;
        if (target.hp > target.stats.mhp) target.hp = target.stats.mhp;
        emit(BattleEvent::Heal, user.slot, target.slot, target.hp - before);
        return;
    }
    if (offensive && target.has(ST_ELAK)) {
        emit(BattleEvent::Miss, user.slot, target.slot, 0, target.name + " mengelak!");
        return;
    }
    if (offensive && s.formula == FM_FISIK && target.has(ST_TANGKIS)) {
        // tangkis: serangan dibatalkan, target membalas x1.5
        const SkillDef* atk = skillDef(SKL_SERANG);
        bool crit = false;
        int dmg = static_cast<int>(computeDamage(target, user, *atk, crit) * 1.5f);
        emit(BattleEvent::Counter, user.slot, target.slot, 0, target.name + " menangkis dan membalas!");
        user.hp -= dmg;
        emit(BattleEvent::Damage, target.slot, user.slot, dmg);
        if (user.hp <= 0) kill(user);
        return;
    }
    if (s.formula != FM_NONE) {
        bool crit = false;
        int dmg = computeDamage(user, target, s, crit);
        target.hp -= dmg;
        BattleEvent e;
        e.kind = BattleEvent::Damage;
        e.src = user.slot;
        e.dst = target.slot;
        e.value = dmg;
        e.critical = crit;
        e.skill = s.id;
        events_.enqueue(e);
        if (target.hp <= 0) { kill(target); return; }
        if (target.has(ST_TIDUR)) removeStatus(target, ST_TIDUR);
    }
    if (s.status != ST_NONE && rng_.range(1, 100) <= s.statusChance) addStatus(target, s.status);
    if (s.delayPct > 0) applyDelay(target, s.delayPct);
}

void BattleSystem::addStatus(Battler& t, int st) {
    if (!t.alive() || st <= 0 || st >= ST_COUNT) return;
    float before = t.rate();
    bool fresh = t.status[st] == 0;
    t.status[st] = statusDef(st).turns;
    if (fresh) emit(BattleEvent::StatusAdd, -1, t.slot, 0, t.name + " terkena " + statusDef(st).name + ".", 0, st);
    float after = t.rate();
    if (after != before && !t.chanting) {
        // ubah laju gauge: sisa waktu diskalakan ulang (node pindah posisi)
        if (auto* n = timeline_.search(&t)) {
            timeline_.updateAV(&t, n->av * before / after);
            ds("Timeline.updateAV(" + t.name + ", laju berubah)");
        }
    }
}

void BattleSystem::removeStatus(Battler& t, int st) {
    if (st <= 0 || st >= ST_COUNT || t.status[st] == 0) return;
    float before = t.rate();
    t.status[st] = 0;
    emit(BattleEvent::StatusRemove, -1, t.slot, 0, "", 0, st);
    float after = t.rate();
    if (after != before && !t.chanting && t.alive()) {
        if (auto* n = timeline_.search(&t)) timeline_.updateAV(&t, n->av * before / after);
    }
}

void BattleSystem::kill(Battler& t) {
    t.hp = 0;
    t.chanting = false;
    for (int s = 1; s < ST_COUNT; ++s) t.status[s] = 0;
    timeline_.remove(&t);
    ds("Timeline.remove(" + t.name + ")");
    emit(BattleEvent::Death, -1, t.slot, 0, t.name + (t.enemy ? " tumbang." : " roboh!"));
}

void BattleSystem::checkEnd() {
    if (over()) return;
    bool enemyAlive = false, partyAlive = false;
    for (int i = 0; i < count_; ++i) {
        if (!b_[i].alive()) continue;
        if (b_[i].enemy) enemyAlive = true; else partyAlive = true;
    }
    if (!partyAlive) {
        phase_ = Defeat;
        inputActor_ = -1;
        emit(BattleEvent::Defeat, -1, -1, 0, "Kamu mati.");
    } else if (!enemyAlive) {
        sukma_ = 0;
        for (int i = partyCount_; i < count_; ++i)
            if (b_[i].def != nullptr) sukma_ += b_[i].def->sukma;
        phase_ = Victory;
        inputActor_ = -1;
        emit(BattleEvent::Victory, -1, -1, sukma_, "Menang! Sukma +" + std::to_string(sukma_));
    }
}

bool BattleSystem::tryEscape() {
    if (phase_ != Input) return false;
    float pAgi = 0, eAgi = 0;
    int pn = 0, en = 0;
    for (int i = 0; i < count_; ++i) {
        if (!b_[i].alive()) continue;
        if (b_[i].enemy) { eAgi += b_[i].stats.agi; ++en; } else { pAgi += b_[i].stats.agi; ++pn; }
    }
    pAgi /= (pn ? pn : 1);
    eAgi /= (en ? en : 1);
    float chance = 0.45f + 0.15f * escapeFails_ + (pAgi / (eAgi > 1 ? eAgi : 1) - 1.0f) * 0.3f;
    if (rng_.chance(chance)) {
        phase_ = Escaped;
        inputActor_ = -1;
        emit(BattleEvent::Escaped, -1, -1, 0, "Kamu berhasil kabur.");
        return true;
    }
    ++escapeFails_;
    emit(BattleEvent::EscapeFail, -1, -1, 0, "Gagal kabur!");
    // seperti BS1: gauge party direset ke 0-10%
    for (int i = 0; i < partyCount_; ++i) {
        Battler& b = b_[i];
        if (!b.alive()) continue;
        float pct = rng_.range(0, 10) / 100.0f;
        b.chanting = false;
        timeline_.updateAV(&b, MAX_AP * (1.0f - pct) / b.rate());
    }
    ds("Timeline.updateAV(party, gauge direset)");
    phase_ = Filling;
    inputActor_ = -1;
    return false;
}

BattleAction BattleSystem::chooseEnemyAction(const Battler& e) {
    BattleAction a;
    a.skill = SKL_SERANG;
    if (e.def != nullptr) {
        // aturan VX Ace: ambil aksi valid dengan rating >= max-2, acak berbobot
        int maxR = 0;
        int hpPct = e.hp * 100 / (e.stats.mhp > 0 ? e.stats.mhp : 1);
        for (int i = 0; i < e.def->actionCount; ++i) {
            const EnemyAction& ea = e.def->actions[i];
            if (hpPct <= ea.hpBelowPct && canUse(e.slot, ea.skill) && ea.rating > maxR) maxR = ea.rating;
        }
        int total = 0;
        for (int i = 0; i < e.def->actionCount; ++i) {
            const EnemyAction& ea = e.def->actions[i];
            if (hpPct <= ea.hpBelowPct && canUse(e.slot, ea.skill) && ea.rating >= maxR - 2) total += ea.rating - (maxR - 3);
        }
        if (total > 0) {
            int roll = rng_.range(1, total);
            for (int i = 0; i < e.def->actionCount; ++i) {
                const EnemyAction& ea = e.def->actions[i];
                if (!(hpPct <= ea.hpBelowPct && canUse(e.slot, ea.skill) && ea.rating >= maxR - 2)) continue;
                roll -= ea.rating - (maxR - 3);
                if (roll <= 0) { a.skill = ea.skill; break; }
            }
        }
    }
    // target acak dari pihak lawan yang masih hidup
    int alive[MAX_BATTLERS];
    int n = 0;
    for (int i = 0; i < count_; ++i)
        if (b_[i].alive() && validTarget(e.slot, a.skill, i)) alive[n++] = i;
    a.target = n > 0 ? alive[rng_.range(0, n - 1)] : -1;
    return a;
}
