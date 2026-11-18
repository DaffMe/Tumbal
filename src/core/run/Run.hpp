#pragma once
// ============================================================================
// Run — satu ekspedisi penuh (3 lantai). Mengikat M2 (Floor/Graph) dengan
// M1 & M3 (Battle).
//
//   Exploring ──travelTo──► isi ruangan:
//       Battle/Elite/Boss → InBattle ──finishBattle──► Exploring / ChoosingRelic
//       Shrine            → AtShrine  (rest, checkpoint)
//       Merchant          → InShop
//       Event             → InEvent
//   Party tumbang → kembali ke checkpoint, Ash tertinggal di ruang kematian.
//   Boss kalah → pilih relic → lantai berikutnya → menang setelah lantai 3.
// ============================================================================

#include <cstdint>
#include <memory>
#include "../adt/Queue.hpp"
#include "../adt/Stack.hpp"
#include "../battle/Battle.hpp"
#include "../dungeon/Floor.hpp"
#include "../util/Rng.hpp"

constexpr int MAX_FLOORS  = 3;
constexpr int PARTY_SIZE  = 3;
constexpr int MAX_LIGHT   = 100;
constexpr int RELIC_CHOICES = 3;

enum class RunState { Exploring, InBattle, AtShrine, InShop, InEvent, ChoosingRelic, RunWon };

enum class ShopItem { LampOil, Hourglass, Elixir, COUNT };

enum class Relic { CinderRing, CrackedClock, EternalWick, MartyrBlood, IronHide, COUNT };

struct EventDef {
    const char* title;
    const char* text;
    const char* options[2];
};

const char*     shopItemName(ShopItem item);
const char*     shopItemDesc(ShopItem item);
const char*     relicName(Relic r);
const char*     relicDesc(Relic r);
const EventDef& eventDef(int id);

class Run {
public:
    explicit Run(std::uint32_t seed);

    // ------------------------------------------------ eksplorasi (M2)
    bool travelTo(int roomId);          // ke ruang bertetangga
    int  autoTravel(int roomId);        // ikuti jalur Dijkstra; kembalikan langkah yang ditempuh
    bool travelToNearestShrine();

    // ------------------------------------------------ battle
    Battle* battle() { return battle_.get(); }
    void    finishBattle();             // panggil setelah battle()->isOver()

    // ------------------------------------------------ ruangan khusus
    bool rest();                        // di shrine
    void leaveRoom();                   // keluar dari shrine/toko
    int  price(ShopItem item) const;
    bool buy(ShopItem item);
    const EventDef& currentEvent() const;
    bool chooseEventOption(int option);
    int   relicOptionCount() const { return relicOptionCount_; }
    Relic relicOption(int i) const { return relicOptions_[i]; }
    bool  chooseRelic(int i);

    // ------------------------------------------------ query
    RunState    state() const { return state_; }
    Floor&      floor() { return floor_; }
    const Floor& floor() const { return floor_; }
    Unit&       hero(int i) { return party_[i]; }
    const Unit& hero(int i) const { return party_[i]; }
    int  ash() const { return ash_; }
    int  light() const { return light_; }
    LightLevel lightLevel() const;
    int  hourglass() const { return hourglass_; }
    int  deaths() const { return deaths_; }
    int  checkpoint() const { return checkpoint_; }
    int  droppedAsh() const { return droppedAsh_; }
    int  droppedAshRoom() const { return droppedAshRoom_; }
    bool hasRelic(Relic r) const { return (relics_ & (1u << static_cast<int>(r))) != 0; }
    int  lightCost(int from, int to) const;   // biaya cahaya setelah relic

    bool pollEvent(BattleEvent& out) { return log_.dequeue(out); }

private:
    std::uint32_t           seed_;
    Rng                     rng_;
    Floor                   floor_;
    Unit                    party_[PARTY_SIZE];
    std::unique_ptr<Battle> battle_;
    Queue<BattleEvent>      log_;

    RunState state_;
    int      ash_;
    int      light_;
    int      hourglass_;
    int      deaths_;
    int      checkpoint_;
    int      droppedAsh_;
    int      droppedAshRoom_;
    unsigned relics_;
    Relic    relicOptions_[RELIC_CHOICES];
    int      relicOptionCount_;

    void enterRoom(int id);             // picu isi ruangan
    void startBattle(const Room& room);
    void handlePartyWipe();
    void tryPickUpAsh();
    void descend();
    void setupFloor(int floorNo);
    void spendLight(int amount);
    void healParty(float fraction, bool revive);
    void info(const std::string& text);
};
