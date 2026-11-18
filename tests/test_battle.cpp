// ============================================================================
// Unit test mekanik battle: M1 Action Timeline, M3 Rewind, Break, Summon.
// Memakai unit dengan stat buatan agar hasil bisa diprediksi.
// ============================================================================

#include <string>
#include "TestFramework.hpp"
#include "../src/core/battle/Battle.hpp"

namespace {

// Hero "abadi" dengan SPD tertentu (HP besar, ATK kecil → battle tidak cepat selesai)
Unit hero(HeroKind k, float spd) {
    Unit u = makeHero(k);
    u.baseSpd = spd;
    u.maxHp = u.hp = 99999;
    u.atk = 1;
    return u;
}

// Musuh tanpa toughness (tidak bisa Break) dengan SPD tertentu
Unit dummy(float spd, int hp = 99999) {
    Unit u = makeEnemy(EnemyKind::HollowThrall);
    u.name = "Dummy";
    u.baseSpd = spd;
    u.maxHp = u.hp = hp;
    u.atk = 1;
    u.def = 0;
    u.maxToughness = u.toughness = 0;
    return u;
}

void drain(Battle& b) {
    BattleEvent e;
    while (b.pollEvent(e)) {}
}

bool hasEvent(Battle& b, EventType type, int amount = 0x7fffffff) {
    BattleEvent e;
    bool found = false;
    while (b.pollEvent(e))
        if (e.type == type && (amount == 0x7fffffff || e.amount == amount)) found = true;
    return found;
}

int firstEnemy(const Battle& b) {
    for (int i = 0; i < b.unitCount(); ++i)
        if (b.unit(i).side == Side::Enemy && b.unit(i).alive) return i;
    return -1;
}

// -------------------------------------------------------------------------
void testTurnOrderAndPrediction() {
    SECTION("M1: urutan giliran mengikuti SPD & prediksi akurat");
    Unit h[2] = {hero(HeroKind::Varga, 100), hero(HeroKind::Sable, 160)};
    Unit e[1] = {dummy(70)};
    Battle b(h, 2, e, 1, 42, 3, false);
    b.start();

    CHECK(b.state() == BattleState::WaitingForPlayer);
    CHECK(b.currentIndex() == 1);              // Sable (SPD 160) bertindak pertama
    CHECK(b.timeline().isValid());

    int predicted[6];
    CHECK(b.predictTurnOrder(6, predicted) == 6);
    CHECK(b.timeline().size() == 2);           // prediksi tidak mengubah timeline asli
    drain(b);

    // Mainkan dengan basic attack, catat urutan aktor dari event TurnStart
    int actual[6];
    int n = 0;
    while (n < 6 && b.state() == BattleState::WaitingForPlayer) {
        b.playerUseSkill(0, firstEnemy(b));
        BattleEvent ev;
        while (b.pollEvent(ev))
            if (ev.type == EventType::TurnStart && n < 6) actual[n++] = ev.source;
        CHECK(b.timeline().isValid());
    }
    CHECK(n == 6);
    bool same = true;
    for (int i = 0; i < 6; ++i) same = same && predicted[i] == actual[i];
    CHECK(same);
}

// -------------------------------------------------------------------------
void testDelayAdvance() {
    SECTION("M1: delay / advance merelink node");
    Unit h[1] = {hero(HeroKind::Sable, 200)};
    Unit e[2] = {dummy(100), dummy(80)};
    Battle b(h, 1, e, 2, 7, 3, false);
    b.start();
    drain(b);
    CHECK(b.currentIndex() == 0);

    // timeline saat ini: Dummy(100 SPD) lalu Dummy(80 SPD)
    const Unit* fast = b.timeline().front()->item;
    int fastIdx = b.indexOf(fast);
    float before = b.timeline().front()->av;

    CHECK(b.shiftActionValue(fastIdx, 0.40f));      // tunda 40% dari AV dasar 100
    Timeline<Unit>::Node* node = b.timeline().search(b.timeline().back()->item);
    CHECK(b.timeline().back()->item == fast);       // pindah ke belakang
    CHECK(node->av > before + 39.9f && node->av < before + 40.1f);
    CHECK(b.timeline().isValid());

    CHECK(b.shiftActionValue(fastIdx, -2.0f));      // majukan habis → AV 0, jadi head
    CHECK(b.timeline().front()->item == fast && b.timeline().front()->av == 0.0f);
    CHECK(b.timeline().isValid());

    CHECK(!b.shiftActionValue(0, 0.5f));            // aktor aktif tidak ada di timeline

    // Unravel (slot 1) memicu event AvShift +40
    CHECK(b.playerUseSkill(1, fastIdx));
    // (event diperiksa terpisah karena battle sudah lanjut)
}

// -------------------------------------------------------------------------
void testDeathAndVictory() {
    SECTION("M1: unit tumbang dihapus dari timeline");
    Unit h[1] = {hero(HeroKind::Varga, 150)};
    Unit e[2] = {dummy(100, 1), dummy(90)};
    Battle b(h, 1, e, 2, 3, 3, false);
    b.start();
    drain(b);

    int target = 1;
    CHECK(b.timeline().contains(&b.unit(target)));
    CHECK(b.playerUseSkill(0, target));
    CHECK(!b.unit(target).alive);
    CHECK(!b.timeline().contains(&b.unit(target)));
    CHECK(b.timeline().isValid());

    b.unit(2).hp = 1;
    while (b.state() == BattleState::WaitingForPlayer) b.playerUseSkill(0, 2);
    CHECK(b.state() == BattleState::Victory);
}

// -------------------------------------------------------------------------
void testRewind() {
    SECTION("M3: rewind memulihkan state dari Stack");
    Unit h[1] = {hero(HeroKind::Varga, 100)};
    h[0].atk = 50;
    Unit e[1] = {dummy(50)};
    Battle b(h, 1, e, 1, 99, 2, false);
    b.start();
    drain(b);

    CHECK(b.historySize() == 1);
    CHECK(!b.rewindTurn());                         // belum ada giliran sebelumnya
    int turn1  = b.turnNumber();
    int hpFull = b.unit(1).hp;

    CHECK(b.playerUseSkill(0, 1));
    drain(b);
    CHECK(b.unit(1).hp < hpFull);
    CHECK(b.historySize() == 2);

    CHECK(b.rewindTurn());
    CHECK(hasEvent(b, EventType::Rewind));
    CHECK(b.unit(1).hp == hpFull);
    CHECK(b.turnNumber() == turn1);
    CHECK(b.currentIndex() == 0);
    CHECK(b.hourglass() == 1);
    CHECK(b.historySize() == 1);
    CHECK(b.timeline().isValid() && b.timeline().contains(&b.unit(1)));
    CHECK(b.ember() == START_EMBER);

    // Hourglass habis → rewind ditolak
    b.playerUseSkill(0, 1);
    b.playerUseSkill(0, 1);
    CHECK(b.rewindTurn());
    CHECK(b.hourglass() == 0);
    b.playerUseSkill(0, 1);
    CHECK(!b.rewindTurn());
}

// -------------------------------------------------------------------------
void testInterject() {
    SECTION("M1: interject menyisipkan hero di head");
    Unit h[2] = {hero(HeroKind::Varga, 200), hero(HeroKind::Sable, 90)};
    Unit e[1] = {dummy(100)};
    Battle b(h, 2, e, 1, 5, 3, false);
    b.start();
    drain(b);
    CHECK(b.currentIndex() == 0);

    CHECK(!b.canInterject(1));                      // Resolve belum penuh
    b.unit(1).resolve = MAX_RESOLVE;
    CHECK(b.interject(1));
    CHECK(b.timeline().front()->item == &b.unit(1));
    CHECK(b.unit(1).empowered && b.unit(1).resolve == 0);
    CHECK(b.timeline().isValid());

    CHECK(b.playerUseSkill(0, 2));                  // Varga selesai → Sable langsung
    CHECK(b.currentIndex() == 1);
    CHECK(!b.canUseSkill(0) && b.canUseSkill(ULT_SLOT));
    CHECK(b.playerUseSkill(ULT_SLOT, 2));
    CHECK(!b.unit(1).empowered);
}

// -------------------------------------------------------------------------
void testBreakSummonAmbush() {
    SECTION("Break, Summon, dan Ambush");
    Unit h[1] = {hero(HeroKind::Varga, 200)};
    Unit e[1] = {dummy(100)};
    e[0].maxToughness = e[0].toughness = 10;

    Battle b(h, 1, e, 1, 11, 3, false);
    b.start();
    drain(b);
    CHECK(b.playerUseSkill(0, 1));                  // Cleave: toughness 10 → Break
    BattleEvent ev;
    bool brk = false, delayed = false;
    while (b.pollEvent(ev)) {
        if (ev.type == EventType::Break) brk = true;
        if (ev.type == EventType::AvShift && ev.target == 1 && ev.amount == 25) delayed = true;
    }
    CHECK(brk && delayed);

    int slot = b.spawnUnit(makeEnemy(EnemyKind::Wisp));
    CHECK(slot == 2 && b.unitCount() == 3);
    CHECK(b.timeline().contains(&b.unit(slot)) && b.timeline().isValid());

    Unit e2[1] = {dummy(100)};
    Battle amb(h, 1, e2, 1, 11, 3, true);
    amb.start();
    CHECK(hasEvent(amb, EventType::AvShift, -30));
}

}  // namespace

void runBattleTests() {
    testTurnOrderAndPrediction();
    testDelayAdvance();
    testDeathAndVictory();
    testRewind();
    testInterject();
    testBreakSummonAmbush();
}
