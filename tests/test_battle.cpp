// Unit test M1: battle ATB mode wait + antrean giliran (Timeline DLL).
#include <cstdio>
#include "TestFramework.hpp"
#include "../src/core/battle/BattleSystem.hpp"

static void drain(BattleSystem& bs) {
    BattleEvent e;
    while (bs.events().dequeue(e)) {}
}

// Jalankan battle sampai giliran input pemain berikutnya (musuh bertindak otomatis).
static int untilInput(BattleSystem& bs) {
    for (int guard = 0; guard < 200 && !bs.over(); ++guard) {
        if (bs.phase() == BattleSystem::Input) return bs.inputActor();
        bs.advanceToNextActor();
    }
    return -1;
}

static void testTurnOrderBySpeed() {
    SECTION("M1: urutan giliran mengikuti AGI");
    Battler hero = makeHeroBattler("Tumbal", KL_MALING, 1);   // AGI 50
    BattleSystem bs;
    bs.setup(&hero, 1, TR_BELATUNG_3, INIT_NORMAL, 1234);       // belatung AGI 20
    CHECK(bs.count() == 4);
    CHECK(bs.timeline().size() == 4);
    CHECK(bs.timeline().isValid());

    // prediksi 10 giliran: pemain (laju 60) harus muncul lebih sering dari
    // tiap belatung (laju 30)
    int order[10];
    int n = bs.predictTurns(order, 10);
    CHECK(n == 10);
    int heroTurns = 0, firstWorm = 0;
    for (int i = 0; i < n; ++i) {
        if (order[i] == 0) ++heroTurns;
        if (order[i] == 1) ++firstWorm;
    }
    CHECK(heroTurns > firstWorm);
    // prediksi tidak mengubah Timeline asli
    CHECK(bs.timeline().size() == 4 && bs.timeline().isValid());

    // giliran nyata sesuai prediksi pertama
    int first = bs.advanceToNextActor();
    CHECK(first == order[0]);
}

static void testWaitMode() {
    SECTION("M1: mode wait — waktu berhenti saat input");
    Battler hero = makeHeroBattler("Tumbal", KL_PENDEKAR, 1);
    BattleSystem bs;
    bs.setup(&hero, 1, TR_TUMBAL_GAGAL, INIT_PREEMPTIVE, 7);
    int who = untilInput(bs);
    CHECK(who == 0);
    CHECK(bs.phase() == BattleSystem::Input);
    float g = bs.gauge(1);
    CHECK(bs.advanceTime(500.0f) == 0);        // tetap giliran pemain
    CHECK(bs.gauge(1) == g);                    // gauge musuh tidak bergerak
    // serang → musuh menerima damage
    int hpBefore = bs.battler(1).hp;
    BattleAction a{SKL_SERANG, 1};
    CHECK(bs.submit(a));
    CHECK(bs.battler(1).hp < hpBefore);
    CHECK(bs.phase() == BattleSystem::Filling || bs.over());
    drain(bs);
}

static void testChantAndBreak() {
    SECTION("M1: rapal tertunda & dibuyarkan delay");
    Battler hero = makeHeroBattler("Tumbal", KL_PENDEKAR, 1);
    BattleSystem bs;
    bs.setup(&hero, 1, TR_PENJAGA, INIT_PREEMPTIVE, 99);
    untilInput(bs);
    // Tebas Berat punya rapal → pemain merapal, belum ada damage
    int hpBefore = bs.battler(1).hp;
    CHECK(bs.submit(BattleAction{SKL_TEBAS_BERAT, 1}));
    CHECK(bs.battler(0).chanting);
    CHECK(bs.battler(1).hp == hpBefore);
    CHECK(bs.battler(0).mp == bs.battler(0).stats.mmp - skillDef(SKL_TEBAS_BERAT)->mpCost);
    // jalankan sampai rapal selesai
    for (int i = 0; i < 20 && bs.battler(0).chanting && !bs.over(); ++i) bs.advanceToNextActor();
    CHECK(!bs.battler(0).chanting);
    CHECK(bs.battler(1).hp < hpBefore || bs.battler(0).hp <= 0);

    // applyDelay ke battler yang merapal: rapal buyar dan node mundur
    BattleSystem bs2;
    Battler pawang = makeHeroBattler("Pawang", KL_PAWANG, 1);
    bs2.setup(&pawang, 1, TR_TIKUS_2, INIT_PREEMPTIVE, 5);
    untilInput(bs2);
    CHECK(bs2.submit(BattleAction{SKL_API_TUNGKU, -1}));
    CHECK(bs2.battler(0).chanting);
    CHECK(bs2.timeline().isValid());
    drain(bs2);

    // Penjaga Lumbung bersiap "Hantam Tanah" (rapal) → Hantaman Guncang
    // dari Maling membuyarkannya
    bool broke = false;
    for (unsigned seed = 1; seed < 60 && !broke; ++seed) {
        Battler maling = makeHeroBattler("Maling", KL_MALING, 5);
        BattleSystem b3;
        b3.setup(&maling, 1, TR_PENJAGA, INIT_NORMAL, seed);
        for (int turn = 0; turn < 12 && !b3.over() && !broke; ++turn) {
            if (untilInput(b3) < 0) break;
            drain(b3);
            if (b3.battler(1).chanting) {
                float before = b3.gauge(1);
                CHECK(b3.submit(BattleAction{SKL_SERANGAN_GUNCANG, 1}));
                BattleEvent e;
                while (b3.events().dequeue(e))
                    if (e.kind == BattleEvent::ChantBroken && e.dst == 1) broke = true;
                CHECK(!b3.battler(1).chanting);
                CHECK(b3.gauge(1) < before);
                CHECK(b3.timeline().isValid());
            } else {
                b3.submit(BattleAction{SKL_ELAK, -1});
            }
        }
    }
    CHECK(broke);
}

static void testEvadeParry() {
    SECTION("M1: Elak & Tangkis");
    Battler hero = makeHeroBattler("Tumbal", KL_PENDEKAR, 1);
    BattleSystem bs;
    bs.setup(&hero, 1, TR_TUMBAL_GAGAL, INIT_PREEMPTIVE, 3);
    untilInput(bs);
    CHECK(bs.submit(BattleAction{SKL_ELAK, -1}));
    CHECK(bs.battler(0).has(ST_ELAK));
    int hp = bs.battler(0).hp;
    // musuh bertindak sebelum giliran pemain berikutnya: tidak boleh kena damage
    untilInput(bs);
    CHECK(bs.battler(0).hp == hp);
    CHECK(!bs.battler(0).has(ST_ELAK));        // habis saat giliran pemain tiba

    // ulangi Tangkis sampai musuh sempat menyerang (AGI pemain lebih tinggi,
    // jadi kadang pemain dapat dua giliran beruntun)
    bool countered = false;
    for (int round = 0; round < 6 && !countered && !bs.over(); ++round) {
        drain(bs);
        CHECK(bs.submit(BattleAction{SKL_TANGKIS, -1}));
        int heroHp = bs.battler(0).hp;
        for (int g = 0; g < 50 && bs.phase() != BattleSystem::Input && !bs.over(); ++g) bs.advanceToNextActor();
        BattleEvent e;
        bool enemyAttacked = false;
        while (bs.events().dequeue(e)) {
            if (e.kind == BattleEvent::UseSkill && e.src == 1) enemyAttacked = true;
            if (e.kind == BattleEvent::Counter) countered = true;
        }
        // Tumbal Gagal hanya punya serangan fisik → semuanya ditangkis
        if (enemyAttacked) CHECK(countered && bs.battler(0).hp == heroHp);
    }
    CHECK(countered);
    drain(bs);
}

static void testVictory() {
    SECTION("M1: battle sampai menang + hadiah sukma");
    Battler hero = makeHeroBattler("Tumbal", KL_PENDEKAR, 1);
    BattleSystem bs;
    bs.setup(&hero, 1, TR_TIKUS_2, INIT_NORMAL, 42);
    for (int guard = 0; guard < 100 && !bs.over(); ++guard) {
        int who = untilInput(bs);
        if (who < 0) break;
        BattleAction a{SKL_SERANG, bs.firstTarget(who, SKL_SERANG)};
        bs.submit(a);
        drain(bs);
    }
    CHECK(bs.over());
    CHECK(bs.phase() == BattleSystem::Victory);
    CHECK(bs.sukmaReward() == 2 * enemyDef(EN_TIKUS_LUMBUNG)->sukma);
    // musuh yang mati sudah dihapus dari Timeline
    CHECK(bs.timeline().size() == 1);
}

static void testDamageFormula() {
    SECTION("M1: rumus damage BS1 (atk*4 - def*2) dalam rentang variance");
    Battler hero = makeHeroBattler("Tumbal", KL_PENDEKAR, 1);  // ATK 16
    int expected = 16 * 4 - enemyDef(EN_TUMBAL_GAGAL)->stats.def * 2;   // 52
    int minSeen = 1 << 30, maxSeen = 0;
    for (unsigned seed = 1; seed < 40; ++seed) {
        BattleSystem bs;
        bs.setup(&hero, 1, TR_TUMBAL_GAGAL, INIT_PREEMPTIVE, seed);
        untilInput(bs);
        int before = bs.battler(1).hp;
        bs.submit(BattleAction{SKL_SERANG, 1});
        BattleEvent e;
        while (bs.events().dequeue(e)) {
            if (e.kind == BattleEvent::Damage && e.dst == 1 && !e.critical) {
                if (e.value < minSeen) minSeen = e.value;
                if (e.value > maxSeen) maxSeen = e.value;
            }
        }
        (void)before;
    }
    // mayat lemah gaib/api, bukan fisik → rate 1.0; variance ±20%
    CHECK(minSeen >= static_cast<int>(expected * 0.8f) - 1);
    CHECK(maxSeen <= static_cast<int>(expected * 1.2f) + 1);
    CHECK(maxSeen > minSeen);
}

static void testNoEscape() {
    SECTION("M1: boss dari event tidak bisa dikabur");
    Battler hero = makeHeroBattler("Tumbal", KL_MALING, 1);
    BattleSystem bs;
    bs.setup(&hero, 1, TR_MEMEDI, INIT_PREEMPTIVE, 11);
    bs.setCanEscape(false);
    untilInput(bs);
    CHECK(bs.phase() == BattleSystem::Input);
    CHECK(!bs.tryEscape());
    CHECK(bs.phase() == BattleSystem::Input);     // tetap giliran pemain
    CHECK(!bs.over());
    // Memedi lemah api: Bom Minyak masuk x2
    CHECK(enemyDef(EN_MEMEDI_SAWAH)->elementRate[EL_API] == 2.0f);
}

void runBattleTests() {
    testNoEscape();
    testTurnOrderBySpeed();
    testWaitMode();
    testChantAndBreak();
    testEvadeParry();
    testVictory();
    testDamageFormula();
}
