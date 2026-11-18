#pragma once
// ============================================================================
// Battle — mesin pertarungan turn-based.
//
// Struktur data yang dipakai:
//   Timeline<Unit>      (Doubly Linked List) → M1 Action Timeline
//   Queue<BattleEvent>  (Queue)              → Battle Event Queue untuk UI
//   Stack<Snapshot*>    (Stack)              → M3 Cracked Hourglass (rewind)
//
// Alur satu giliran:
//   advanceTimeline()  : waktu maju sebesar AV head, head di-pop → aktor
//   aktor musuh        : AI memilih skill → executeSkill() → endTurn()
//   aktor hero         : saveTurnSnapshot(), tunggu input pemain
//                        playerUseSkill() → executeSkill() → endTurn()
//   endTurn()          : aktor disisipkan kembali ke Timeline (insertSorted)
// ============================================================================

#include <cstdint>
#include "../adt/Timeline.hpp"
#include "../adt/Queue.hpp"
#include "../adt/Stack.hpp"
#include "../util/Rng.hpp"
#include "Unit.hpp"
#include "BattleEvent.hpp"

constexpr int   MAX_UNITS         = 10;
constexpr int   MAX_EMBER         = 5;
constexpr int   START_EMBER       = 3;
constexpr int   MAX_RESOLVE       = 100;
constexpr int   MAX_ENEMIES_ALIVE = 5;
constexpr int   ULT_SLOT          = 9;        // slot khusus untuk ultimate
constexpr float AV_BASE           = 10000.0f; // AV = AV_BASE / SPD

enum class BattleState { Idle, Running, WaitingForPlayer, Victory, Defeat };

class Battle {
public:
    Battle(const Unit* heroes, int heroCount, const Unit* enemies, int enemyCount,
           std::uint32_t seed, int hourglassCharges = 3, bool ambush = false);
    ~Battle();

    Battle(const Battle&) = delete;
    Battle& operator=(const Battle&) = delete;

    // ---------------------------------------------------------- aksi pemain
    void start();
    bool playerUseSkill(int slot, int targetIdx);   // slot 0..n-1 atau ULT_SLOT
    bool interject(int heroIdx);                    // M1: insertFront
    bool rewindTurn();                              // M3: Stack.pop

    // ------------------------------------------- subprogram mekanik (publik)
    // M1: tunda (+) atau majukan (−) giliran unit sebesar persen AV dasarnya
    bool shiftActionValue(int unitIdx, float percent);
    // M1: simulasikan n giliran ke depan pada SALINAN timeline
    int  predictTurnOrder(int n, int outIdx[]);
    // Tambah unit baru di tengah battle (summon) → Timeline.insertSorted
    int  spawnUnit(const Unit& tmpl);

    // --------------------------------------------------------------- query
    BattleState state() const { return state_; }
    bool  isOver() const { return state_ == BattleState::Victory || state_ == BattleState::Defeat; }
    int   currentIndex() const { return current_; }
    int   unitCount() const { return unitCount_; }
    Unit&       unit(int i) { return units_[i]; }
    const Unit& unit(int i) const { return units_[i]; }
    int   ember() const { return ember_; }
    int   hourglass() const { return hourglass_; }
    int   turnNumber() const { return turn_; }
    int   historySize() const { return static_cast<int>(history_.size()); }
    const Timeline<Unit>& timeline() const { return timeline_; }
    int   indexOf(const Unit* u) const { return static_cast<int>(u - units_); }

    float baseAV(const Unit& u) const { return AV_BASE / u.spd(); }
    const Skill* skillAt(int slot) const;           // skill aktor saat ini
    bool  canUseSkill(int slot) const;
    bool  isValidTarget(int casterIdx, TargetType type, int targetIdx) const;
    bool  canInterject(int heroIdx) const;

    bool  pollEvent(BattleEvent& out) { return events_.dequeue(out); }
    const Queue<BattleEvent>& events() const { return events_; }

private:
    // Snapshot satu titik keputusan pemain (untuk rewind)
    struct Snapshot {
        Unit           units[MAX_UNITS];
        int            unitCount;
        Timeline<Unit> timeline;   // deep copy (Timeline punya copy constructor)
        int            ember;
        int            current;
        int            turn;
        Rng            rng;
    };

    Unit               units_[MAX_UNITS];
    int                unitCount_;
    Timeline<Unit>     timeline_;
    Queue<BattleEvent> events_;
    Stack<Snapshot*>   history_;

    int         ember_;
    int         current_;
    int         turn_;
    int         hourglass_;
    bool        ambush_;
    BattleState state_;
    Rng         rng_;

    // ---- alur giliran
    void  advanceUntilPlayer();
    void  beginNextTurn();
    Unit* advanceTimeline();          // M1 subprogram
    void  endTurn();
    void  checkBattleEnd();

    // ---- eksekusi skill
    void executeSkill(int casterIdx, const Skill& skill, int targetIdx);
    void applyEffect(int casterIdx, int targetIdx, const Skill& skill, const Effect& e, bool toughDone[]);
    int  collectTargets(int casterIdx, TargetType type, int targetIdx, int out[]) const;
    int  computeDamage(const Unit& attacker, const Unit& defender, float power);
    void applyDamage(int sourceIdx, int targetIdx, int amount, const char* cause);
    void handleUnitDeath(int idx);    // M1 subprogram
    void changeEmber(int delta);
    void rescaleAV(int idx, float oldSpd);
    void tickStartOfTurn(int idx);
    void decrementStatuses(int idx);

    // ---- AI musuh
    void         runEnemyTurn(int idx);
    const Skill* chooseEnemySkill(Unit& e);
    int          chooseEnemyTarget(int idx, const Skill& s);
    int          aliveCount(Side side) const;

    // ---- M3 rewind
    void saveTurnSnapshot();          // M3 subprogram
    void restore(const Snapshot& s);
    void clearHistory();

    // ---- event
    void emit(EventType type, int src, int dst, int amount, const std::string& text);
    void dsOp(const std::string& text);
};
