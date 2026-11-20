#pragma once
// ============================================================================
// BattleSystem — Mekanik M1: battle ATB mode "wait" + antrean giliran.
//
// Replikasi skrip ATB Black Souls 1:
//   - Setiap battler punya gauge AP 0..4000 yang terisi `AGI + 10` per tick
//     (1 tick = 1 frame pada 60 fps). Yang penuh lebih dulu bertindak.
//   - Mode "wait": waktu BERHENTI selama pemain memilih perintah dan selama
//     aksi berjalan, jadi tidak ada tekanan waktu (terasa seperti CTB).
//   - Jurus berat punya waktu RAPAL: setelah dipilih, battler menunggu gauge
//     rapal penuh baru aksinya keluar. Rapal bisa DIBUYARKAN serangan penunda.
//   - Serangan penunda (delay) mendorong gauge target mundur.
//
// Struktur data:
//   - Timeline<Battler> (Doubly Linked List terurut) menyimpan SISA WAKTU
//     (tick) tiap battler sampai gilirannya. Head = bertindak berikutnya.
//     Waktu berjalan = subtractAll(); delay/haste/rapal = updateAV() yang
//     memindah node (unlink + relink); battler mati = remove().
//     Salinan Timeline dipakai untuk memprediksi urutan giliran (panel antrean).
//   - Queue<BattleEvent> menampung kejadian untuk dianimasikan GUI / console.
// ============================================================================

#include <cstdint>
#include "../adt/Queue.hpp"
#include "../adt/Timeline.hpp"
#include "../util/Rng.hpp"
#include "BattleEvent.hpp"
#include "Battler.hpp"

enum Initiative { INIT_NORMAL = 0, INIT_PREEMPTIVE, INIT_SURPRISE };

struct BattleAction {
    int skill = SKL_SERANG;
    int target = -1;     // indeks battler, -1 = semua (sesuai scope)
    int item = 0;        // > 0: memakai barang ini (skill diabaikan)
};

// Tas barang pemain yang bisa dipakai di battle (diisi GameState).
class ItemBag {
public:
    virtual ~ItemBag() = default;
    virtual int  count(int itemId) const = 0;
    virtual bool removeItem(int itemId, int n = 1) = 0;
};

class BattleSystem {
public:
    static constexpr float MAX_AP = 4000.0f;
    static constexpr int   MAX_PARTY = 4;
    static constexpr int   MAX_ENEMY = 4;
    static constexpr int   MAX_BATTLERS = MAX_PARTY + MAX_ENEMY;

    enum Phase { Filling, Input, Victory, Defeat, Escaped };

    BattleSystem() = default;
    BattleSystem(const BattleSystem&) = delete;
    BattleSystem& operator=(const BattleSystem&) = delete;

    // Mulai battle: party (salinan), troop musuh, inisiatif dari kontak di peta.
    void setup(const Battler* party, int partyCount, int troopId, Initiative init, std::uint32_t seed);

    // ---- waktu
    // M1 subprogram: jalankan waktu sebanyak `ticks` (GUI: dt*60). Berhenti
    // lebih awal bila ada battler yang siap. Mengembalikan indeks battler yang
    // memegang giliran (jika aktor pemain → fase Input), atau -1.
    int advanceTime(float ticks);
    // M1 subprogram: lompat langsung ke battler berikutnya (console & test).
    int advanceToNextActor();

    // ---- perintah pemain (fase Input)
    int  inputActor() const { return inputActor_; }
    bool canUse(int battler, int skill) const;
    // Jalankan aksi untuk aktor yang sedang input. false bila tidak valid.
    bool submit(const BattleAction& action);
    // Barang: tas pemain (boleh nullptr = tanpa barang).
    void setInventory(ItemBag* bag) { bag_ = bag; }
    void setCanEscape(bool can) { canEscape_ = can; }
    // Mode curang (menu cheat): party kebal damage/status, dan/atau serangan party langsung menumbangkan.
    void setCheats(bool god, bool oneHit) { cheatGod_ = god; cheatOneHit_ = oneHit; }
    bool canEscape() const { return canEscape_; }
    const ItemBag* inventory() const { return bag_; }
    bool canUseItem(int battler, int itemId) const;
    bool validItemTarget(int user, int itemId, int target) const;
    int  firstItemTarget(int user, int itemId) const;
    // Coba kabur (dipakai saat fase Input).
    bool tryEscape();

    // ---- telegraf: serangan besar yang sedang dirapal musuh
    // M1 subprogram: jurus rapalan musuh yang mengincar anggota party `partyIndex`
    // (sasaran tunggal atau semua), 0 bila tidak ada. Dipakai GUI untuk peringatan.
    int threatOn(int partyIndex) const;

    // ---- prediksi antrean (panel "giliran berikutnya")
    // M1 subprogram: isi `out` dengan indeks battler untuk n giliran ke depan.
    int predictTurns(int* out, int n) const;

    // ---- akses state
    Phase phase() const { return phase_; }
    bool  over() const { return phase_ == Victory || phase_ == Defeat || phase_ == Escaped; }
    int   count() const { return count_; }
    int   partyCount() const { return partyCount_; }
    Battler&       battler(int i) { return b_[i]; }
    const Battler& battler(int i) const { return b_[i]; }
    // 0..1 isi gauge AP (atau gauge rapal bila merapal).
    float gauge(int i) const;
    int   sukmaReward() const { return sukma_; }
    Queue<BattleEvent>& events() { return events_; }
    const Timeline<Battler>& timeline() const { return timeline_; }

    // Target valid pertama / acak untuk scope tertentu (bantuan GUI & AI).
    bool validTarget(int user, int skill, int target) const;
    int  firstTarget(int user, int skill) const;

private:
    Battler b_[MAX_BATTLERS];
    int     count_ = 0;
    int     partyCount_ = 0;
    Timeline<Battler> timeline_;
    Queue<BattleEvent> events_;
    Rng     rng_;
    Phase   phase_ = Filling;
    int     inputActor_ = -1;
    int     sukma_ = 0;
    int     escapeFails_ = 0;
    ItemBag* bag_ = nullptr;
    bool    canEscape_ = true;
    bool    cheatGod_ = false;
    bool    cheatOneHit_ = false;
    int     cheatDamage(const Battler& user, const Battler& target, int dmg) const;

    void useItem(Battler& user, const BattleAction& a);

    void emit(BattleEvent::Kind k, int src, int dst, int value = 0, const std::string& text = "", int skill = 0, int status = 0);
    void ds(const std::string& op);

    void scheduleFullTurn(Battler& b);        // tunggu satu giliran penuh
    int  onReady(Battler& b);                 // giliran dimulai
    bool startOfTurn(Battler& b);             // status per giliran; false = giliran hilang
    void act(Battler& user, const BattleAction& a);
    void startChant(Battler& user, const BattleAction& a);       // M1 subprogram
    void resolveAction(Battler& user, const BattleAction& a);    // M1 subprogram
    void applyDelay(Battler& target, int pct);                   // M1 subprogram
    void hit(Battler& user, Battler& target, const SkillDef& s);
    int  computeDamage(const Battler& user, const Battler& target, const SkillDef& s, bool& crit);
    void addStatus(Battler& t, int st);
    void removeStatus(Battler& t, int st);
    void kill(Battler& t);
    BattleAction chooseEnemyAction(const Battler& e);
    void checkEnd();
};
