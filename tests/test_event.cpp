// Unit test M3 (interpreter event: Stack frame + Queue notifikasi),
// GameState (inventaris, sukma tercecer), dan barang di battle.
#include <string>
#include "TestFramework.hpp"
#include "../src/core/event/Interpreter.hpp"
#include "../src/core/game/GameState.hpp"

namespace {

struct FakeHost : EventHost {
    int battles = 0, transfers = 0, unlocks = 0, heals = 0;
    void hostStartBattle(int) override { ++battles; }
    void hostTransfer(int, const std::string&) override { ++transfers; }
    void hostUnlock(int, int) override { ++unlocks; }
    void hostHeal() override { ++heals; }
};

const char* SCRIPT = R"({
  "mayat": [
    {"cmd": "text", "who": "", "text": "Mayat itu menggenggam sesuatu."},
    {"cmd": "choice", "options": ["Ambil", "Biarkan"], "branches": [
        [ {"cmd": "item", "item": "jamu", "n": 2},
          {"cmd": "if_flag", "name": "dikutuk", "value": 1,
             "then": [ {"cmd": "text", "who": "", "text": "Tanganmu gemetar."} ],
             "else": [ {"cmd": "call", "event": "bisik"} ] },
          {"cmd": "flag", "name": "mayat_diambil", "value": 1} ],
        [ {"cmd": "text", "who": "", "text": "Kamu pergi."} ]
    ]},
    {"cmd": "text", "who": "", "text": "Selesai."}
  ],
  "bisik": [
    {"cmd": "text", "who": "Suara", "text": "...pulanglah..."},
    {"cmd": "sukma", "n": 40}
  ],
  "lawan": [
    {"cmd": "battle", "troop": 1},
    {"cmd": "notice", "text": "Menang."}
  ]
})";

void testScriptParse() {
    SECTION("M3: skrip JSON -> pohon Command");
    EventLibrary lib;
    std::string err;
    CHECK(lib.loadText(SCRIPT, &err));
    CHECK(lib.count() == 3);
    const Script* s = lib.find("mayat");
    CHECK(s != nullptr);
    if (!s) return;
    CHECK(s->head->op == CMD_TEXT);
    const Command* ch = s->head->next;
    CHECK(ch->op == CMD_CHOICE && ch->childCount == 2);
    CHECK(ch->opt[0] == "Ambil");
    CHECK(ch->child[0]->op == CMD_ITEM && ch->child[0]->n1 == IT_JAMU && ch->child[0]->n2 == 2);
    CHECK(ch->child[0]->next->op == CMD_IF_FLAG);
    CHECK(ch->next->op == CMD_TEXT);
    CHECK(lib.find("tidak_ada") == nullptr);
    CHECK(!lib.loadText(R"({"x": [ {"cmd": "terbang"} ]})", &err) || lib.find("x")->head == nullptr);
}

void testInterpreterFlow() {
    SECTION("M3: interpreter — cabang, call, kembali ke pemanggil (LIFO)");
    EventLibrary lib;
    lib.loadText(SCRIPT, nullptr);
    GameState gs;
    gs.newGame("Tumbal", KL_PENDEKAR);
    FakeHost host;
    Interpreter it(gs, lib, host);

    CHECK(it.runEvent("mayat"));
    CHECK(it.state() == Interpreter::WaitMessage);
    CHECK(it.message() == "Mayat itu menggenggam sesuatu.");
    CHECK(!it.runEvent("bisik"));          // sedang sibuk
    it.resume();
    CHECK(it.state() == Interpreter::WaitChoice);
    CHECK(it.optionCount() == 2 && it.option(1) == "Biarkan");
    int jamu = gs.count(IT_JAMU);
    int depthBefore = it.depth();
    it.choose(0);                          // Ambil → push cabang, lalu call "bisik" → push lagi
    CHECK(gs.count(IT_JAMU) == jamu + 2);
    CHECK(it.state() == Interpreter::WaitMessage);
    CHECK(it.speaker() == "Suara");        // masuk skrip "bisik" lewat call
    CHECK(it.depth() > depthBefore);
    it.resume();                           // sukma +40, skrip bisik habis → pop → lanjut flag → pop → "Selesai."
    CHECK(gs.sukma() == 40);
    CHECK(gs.flag("mayat_diambil") == 1);
    CHECK(it.message() == "Selesai.");
    it.resume();
    CHECK(it.state() == Interpreter::Idle);
    CHECK(it.depth() == 0);

    // notifikasi Queue: barang lalu sukma, urutan FIFO
    std::string n1, n2;
    CHECK(it.notices().dequeue(n1) && n1 == "Didapat: Ramuan Jamu x2");
    CHECK(it.notices().dequeue(n2) && n2 == "Sukma +40");

    // jalankan lagi dengan flag "dikutuk" → cabang then
    gs.setFlag("dikutuk", 1);
    it.runEvent("mayat");
    it.resume();
    it.choose(0);
    CHECK(it.message() == "Tanganmu gemetar.");
    it.resume();
    it.resume();
    CHECK(!it.busy());

    // battle membuat interpreter menunggu sampai resume
    CHECK(it.runEvent("lawan"));
    CHECK(host.battles == 1 && it.state() == Interpreter::WaitBattle);
    it.resume();
    CHECK(!it.busy());
}

void testBloodstain() {
    SECTION("Loop Souls: sukma tercecer & diambil kembali");
    GameState gs;
    gs.newGame("Tumbal", KL_PENDEKAR);
    CHECK(gs.count(IT_JAMU) == gs.jamuMax());
    gs.addSukma(150);
    gs.hero().hp = 10;
    int lost = gs.die(1, 7, 8);
    CHECK(lost == 0);
    CHECK(gs.sukma() == 0);
    CHECK(gs.hero().hp == gs.hero().stats.mhp);      // bangkit pulih
    CHECK(gs.bloodstain().active && gs.bloodstain().sukma == 150);
    CHECK(gs.retrieveBloodstain(1, 7, 9) == 0);      // tile salah
    CHECK(gs.retrieveBloodstain(1, 7, 8) == 150);
    CHECK(gs.sukma() == 150 && !gs.bloodstain().active);

    // mati dua kali tanpa mengambil: ceceran pertama hilang
    gs.die(1, 2, 2);
    gs.addSukma(30);
    lost = gs.die(2, 3, 3);
    CHECK(lost == 150);
    CHECK(gs.bloodstain().sukma == 30 && gs.bloodstain().map == 2);
    CHECK(gs.deaths() == 3);

    // Ramuan Jamu terisi ulang saat istirahat
    gs.removeItem(IT_JAMU, 3);
    CHECK(gs.count(IT_JAMU) == 0);
    gs.rest();
    CHECK(gs.count(IT_JAMU) == gs.jamuMax());
    gs.hero().hp = 100;
    CHECK(!gs.useItemInField(IT_JAMU).empty());
    CHECK(gs.hero().hp == 100 + gs.hero().stats.mhp / 2);
    CHECK(gs.useItemInField(IT_BOM_MINYAK).empty());  // tidak bisa di peta

    // naik level: tukar sukma, stat bertambah sesuai kelas
    GameState lv;
    lv.newGame("Tumbal", KL_PENDEKAR);
    int cost = lv.levelUpCost();
    CHECK(cost == 60);
    CHECK(!lv.levelUp());
    lv.addSukma(cost + 5);
    int atk = lv.hero().stats.atk;
    CHECK(lv.levelUp());
    CHECK(lv.level() == 2 && lv.sukma() == 5);
    CHECK(lv.hero().stats.atk == atk + classDef(KL_PENDEKAR).growth.atk);
    CHECK(lv.levelUpCost() > cost);
}

void testBattleItems() {
    SECTION("Barang di battle (ItemBag)");
    GameState gs;
    gs.newGame("Tumbal", KL_PENDEKAR);
    gs.addItem(IT_BOM_MINYAK, 1);
    BattleSystem bs;
    bs.setup(&gs.hero(), 1, TR_BELATUNG_3, INIT_PREEMPTIVE, 9);
    bs.setInventory(&gs);
    for (int i = 0; i < 20 && bs.phase() != BattleSystem::Input; ++i) bs.advanceToNextActor();
    CHECK(bs.phase() == BattleSystem::Input);
    CHECK(bs.canUseItem(0, IT_BOM_MINYAK));
    CHECK(!bs.canUseItem(0, IT_SUKMA_KECIL));       // bukan barang battle
    CHECK(!bs.validItemTarget(0, IT_BOM_MINYAK, 0));
    int hp = bs.battler(1).hp;
    BattleAction a;
    a.item = IT_BOM_MINYAK;
    a.target = 1;
    CHECK(bs.submit(a));
    CHECK(gs.count(IT_BOM_MINYAK) == 0);
    CHECK(bs.battler(1).hp < hp - 90 || !bs.battler(1).alive());   // belatung lemah api x2
}

}  // namespace

void runEventTests() {
    testScriptParse();
    testInterpreterFlow();
    testBloodstain();
    testBattleItems();
}
