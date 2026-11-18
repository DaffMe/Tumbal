#include "Run.hpp"
#include "../battle/Encounters.hpp"
#include "../util/Format.hpp"

#include <cmath>

// ============================================================================
// Data statis: toko, relic, event
// ============================================================================

const char* shopItemName(ShopItem item) {
    switch (item) {
        case ShopItem::LampOil:   return "Minyak Lentera";
        case ShopItem::Hourglass: return "Pasir Hourglass";
        case ShopItem::Elixir:    return "Elixir Bara";
        default:                  return "?";
    }
}

const char* shopItemDesc(ShopItem item) {
    switch (item) {
        case ShopItem::LampOil:   return "+35 cahaya lentera";
        case ShopItem::Hourglass: return "+1 muatan Rewind";
        case ShopItem::Elixir:    return "Pulihkan 40% HP party, bangkitkan yang tumbang";
        default:                  return "";
    }
}

const char* relicName(Relic r) {
    switch (r) {
        case Relic::CinderRing:   return "Cincin Bara";
        case Relic::CrackedClock: return "Jam Retak";
        case Relic::EternalWick:  return "Sumbu Abadi";
        case Relic::MartyrBlood:  return "Darah Martir";
        case Relic::IronHide:     return "Kulit Besi";
        default:                  return "?";
    }
}

const char* relicDesc(Relic r) {
    switch (r) {
        case Relic::CinderRing:   return "Mulai setiap pertarungan dengan +1 Ember";
        case Relic::CrackedClock: return "+2 muatan Hourglass sekarang";
        case Relic::EternalWick:  return "Biaya cahaya perjalanan -30%";
        case Relic::MartyrBlood:  return "ATK party +15%";
        case Relic::IronHide:     return "HP maksimum party +20%";
        default:                  return "";
    }
}

static const EventDef kEvents[EVENT_COUNT] = {
    {"Altar Berdarah",
     "Altar batu hitam berdenyut pelan. Ukiran di atasnya berbunyi: darah untuk abu.",
     {"Persembahkan darah (HP party -20%, +80 Ash)", "Tinggalkan altar"}},
    {"Langit-langit Berderak",
     "Debu berjatuhan. Lorong di sekitarmu tidak akan bertahan lama.",
     {"Lari menembus reruntuhan (satu koridor runtuh)", "Topang dengan minyak lentera (-15 cahaya)"}},
    {"Lentera Tua",
     "Sebuah lentera berkarat tergantung di dinding, minyaknya masih tersisa.",
     {"Tuang minyaknya ke lenteramu (+30 cahaya)", "Biarkan"}},
    {"Bisikan Katedral",
     "Suara-suara dari balik dinding membisikkan denah lantai ini.",
     {"Dengarkan (seluruh peta terungkap, HP party -10%)", "Tutup telinga"}},
};

const EventDef& eventDef(int id) { return kEvents[(id % EVENT_COUNT + EVENT_COUNT) % EVENT_COUNT]; }

// ============================================================================
// Konstruksi
// ============================================================================

Run::Run(std::uint32_t seed)
    : seed_(seed), rng_(seed), state_(RunState::Exploring), ash_(0), light_(MAX_LIGHT),
      hourglass_(3), deaths_(0), checkpoint_(-1), droppedAsh_(0), droppedAshRoom_(-1),
      relics_(0), relicOptions_{}, relicOptionCount_(0) {
    floor_.setLog(&log_);
    buildParty(party_, PARTY_SIZE);
    setupFloor(1);
}

void Run::setupFloor(int floorNo) {
    floor_.generateFloor(seed_ + static_cast<std::uint32_t>(floorNo) * 7919u, floorNo);
    checkpoint_ = floor_.entrance();
    if (droppedAsh_ > 0) info(strf("%d Ash yang tertinggal di lantai sebelumnya hilang selamanya.", droppedAsh_));
    droppedAsh_ = 0;
    droppedAshRoom_ = -1;
    light_ = MAX_LIGHT;
    info(strf("Lantai %d. Lentera menyala penuh.", floorNo));
    floor_.revealByLight(lightLevel());
    state_ = RunState::Exploring;
}

LightLevel Run::lightLevel() const {
    if (light_ >= 60) return LightLevel::Bright;
    if (light_ >= 25) return LightLevel::Dim;
    return LightLevel::Dark;
}

int Run::lightCost(int from, int to) const {
    int base = floor_.travelCost(from, to);
    if (base < 0) return -1;
    if (hasRelic(Relic::EternalWick)) base = static_cast<int>(std::lround(base * 0.7f));
    return base;
}

void Run::spendLight(int amount) {
    light_ -= amount;
    if (light_ < 0) light_ = 0;
    if (light_ > MAX_LIGHT) light_ = MAX_LIGHT;
}

void Run::info(const std::string& text) { log_.enqueue(BattleEvent{EventType::Info, -1, -1, 0, text}); }

// ============================================================================
// Eksplorasi (M2)
// ============================================================================

// M2 subprogram: berjalan ke ruang bertetangga.
// Cahaya berkurang sesuai bobot koridor → peta terungkap sesuai cahaya
// tersisa (BFS) → isi ruangan dipicu.
bool Run::travelTo(int id) {
    if (state_ != RunState::Exploring) return false;
    int from = floor_.currentRoom();
    int cost = lightCost(from, id);
    if (cost < 0) return false;

    spendLight(cost);
    floor_.moveTo(id);
    info(strf("Berjalan ke ruang %d (%s). Cahaya -%d -> %d.", id, roomTypeName(floor_.room(id)->type), cost, light_));
    floor_.revealByLight(lightLevel());
    enterRoom(id);
    return true;
}

// Auto-travel: jalur Dijkstra (lewat ruang terungkap) ditempuh langkah demi
// langkah; berhenti jika sebuah ruangan memicu sesuatu (battle, toko, ...).
int Run::autoTravel(int target) {
    if (state_ != RunState::Exploring || target == floor_.currentRoom()) return 0;
    Stack<int> path;
    if (floor_.findPath(target, path) < 0) {
        info("Tidak ada jalur yang diketahui ke ruang itu.");
        return 0;
    }
    int id = -1;
    path.pop(id);   // ruang awal (posisi sekarang)
    int steps = 0;
    while (state_ == RunState::Exploring && path.pop(id)) {
        if (!travelTo(id)) break;
        ++steps;
    }
    return steps;
}

bool Run::travelToNearestShrine() {
    Stack<int> path;
    int shrine = floor_.findNearestShrine(path);
    if (shrine < 0) {
        info("Belum ada shrine terungkap yang bisa dicapai.");
        return false;
    }
    return autoTravel(shrine) > 0;
}

void Run::enterRoom(int id) {
    Room& r = *floor_.room(id);
    switch (r.type) {
        case RoomType::Battle:
        case RoomType::Elite:
        case RoomType::Boss:
            if (!r.cleared) {
                startBattle(r);
                return;
            }
            break;
        case RoomType::Shrine:
            if (checkpoint_ != id) {
                checkpoint_ = id;
                info("Ember Shrine menyala. Checkpoint diperbarui.");
            }
            if (!r.cleared) {
                state_ = RunState::AtShrine;
                return;
            }
            break;
        case RoomType::Merchant:
            state_ = RunState::InShop;
            return;
        case RoomType::Event:
            if (!r.cleared) {
                state_ = RunState::InEvent;
                info(strf("Peristiwa: %s", currentEvent().title));
                return;
            }
            break;
        case RoomType::Treasure:
            if (!r.cleared) {
                int gain = 60 + 20 * floor_.number() + rng_.range(0, 30);
                ash_ += gain;
                r.cleared = true;
                info(strf("Peti tua berisi %d Ash.", gain));
            }
            break;
        case RoomType::Entrance:
            break;
    }
    tryPickUpAsh();
}

void Run::tryPickUpAsh() {
    if (droppedAsh_ <= 0 || floor_.currentRoom() != droppedAshRoom_) return;
    ash_ += droppedAsh_;
    info(strf("Kamu memungut kembali %d Ash yang tertinggal.", droppedAsh_));
    droppedAsh_ = 0;
    droppedAshRoom_ = -1;
}

// ============================================================================
// Battle
// ============================================================================

void Run::startBattle(const Room& room) {
    Unit heroes[PARTY_SIZE];
    for (int i = 0; i < PARTY_SIZE; ++i) {
        heroes[i] = party_[i];
        heroes[i].resolve = 0;
        heroes[i].empowered = heroes[i].interjectQueued = false;
        heroes[i].statusCount = 0;
    }

    Unit enemies[MAX_UNITS];
    int n = buildEncounter(room.encounter, enemies, MAX_UNITS);
    float scale = 1.0f + 0.3f * (floor_.number() - 1);
    for (int i = 0; i < n; ++i) {
        enemies[i].maxHp = enemies[i].hp = static_cast<int>(enemies[i].maxHp * scale);
        enemies[i].atk = static_cast<int>(enemies[i].atk * scale);
    }

    // Hubungan M2 → M1: semakin gelap, semakin besar peluang disergap
    float ambushChance = lightLevel() == LightLevel::Dark ? 0.5f : (lightLevel() == LightLevel::Dim ? 0.2f : 0.0f);
    bool ambush = rng_.chance(ambushChance);

    battle_.reset(new Battle(heroes, PARTY_SIZE, enemies, n, rng_.next(), hourglass_, ambush));
    if (hasRelic(Relic::CinderRing)) battle_->setStartEmber(START_EMBER + 1);
    state_ = RunState::InBattle;
    info(strf("%s: %s", roomTypeName(room.type), encounterName(room.encounter)));
    battle_->start();
}

void Run::finishBattle() {
    if (state_ != RunState::InBattle || !battle_ || !battle_->isOver()) return;
    hourglass_ = battle_->hourglass();

    if (battle_->state() == BattleState::Defeat) {
        battle_.reset();
        handlePartyWipe();
        return;
    }

    // Salin HP hero kembali ke party (hero selalu di indeks 0..PARTY_SIZE-1)
    for (int i = 0; i < PARTY_SIZE; ++i) {
        const Unit& b = battle_->unit(i);
        party_[i].hp = b.hp;
        party_[i].alive = b.alive;
        if (!b.alive) {   // hero yang tumbang bangkit dengan 20% HP
            party_[i].alive = true;
            party_[i].hp = party_[i].maxHp / 5;
        }
    }
    battle_.reset();

    Room& r = *floor_.room(floor_.currentRoom());
    r.cleared = true;
    int base = r.type == RoomType::Boss ? 200 : (r.type == RoomType::Elite ? 90 : 30 + rng_.range(0, 20));
    float mult = 1.0f + 0.25f * (floor_.number() - 1);
    if (lightLevel() == LightLevel::Dark) mult *= 1.5f;
    else if (lightLevel() == LightLevel::Dim) mult *= 1.2f;
    int gain = static_cast<int>(base * mult);
    ash_ += gain;
    info(strf("Menang! +%d Ash (total %d).", gain, ash_));
    tryPickUpAsh();

    if (r.type == RoomType::Boss) {
        // Siapkan pilihan relic: acak dari relic yang belum dimiliki
        Relic avail[static_cast<int>(Relic::COUNT)];
        int na = 0;
        for (int i = 0; i < static_cast<int>(Relic::COUNT); ++i)
            if (!hasRelic(static_cast<Relic>(i))) avail[na++] = static_cast<Relic>(i);
        relicOptionCount_ = 0;
        while (relicOptionCount_ < RELIC_CHOICES && na > 0) {
            int k = rng_.range(0, na - 1);
            relicOptions_[relicOptionCount_++] = avail[k];
            avail[k] = avail[--na];
        }
        if (relicOptionCount_ > 0) {
            state_ = RunState::ChoosingRelic;
        } else {
            descend();
        }
        return;
    }
    state_ = RunState::Exploring;
}

// Souls-like: party bangkit di checkpoint, Ash tertinggal di ruang kematian.
void Run::handlePartyWipe() {
    ++deaths_;
    if (droppedAsh_ > 0) info(strf("%d Ash lama yang belum dipungut hilang selamanya.", droppedAsh_));
    droppedAsh_ = ash_;
    droppedAshRoom_ = floor_.currentRoom();
    ash_ = 0;
    info(strf("Party tumbang. %d Ash tertinggal di ruang %d.", droppedAsh_, droppedAshRoom_));

    healParty(1.0f, true);
    light_ = 60;
    floor_.setCurrent(checkpoint_);
    info(strf("Kalian bangkit kembali di ruang %d (checkpoint). Kematian: %d.", checkpoint_, deaths_));
    floor_.revealByLight(lightLevel());
    state_ = RunState::Exploring;
}

void Run::healParty(float fraction, bool revive) {
    for (int i = 0; i < PARTY_SIZE; ++i) {
        Unit& u = party_[i];
        if (!u.alive && !revive) continue;
        u.alive = true;
        int add = static_cast<int>(u.maxHp * fraction);
        u.hp = u.hp + add > u.maxHp ? u.maxHp : u.hp + add;
        if (u.hp < 1) u.hp = 1;
    }
}

// ============================================================================
// Shrine, toko, event, relic
// ============================================================================

bool Run::rest() {
    if (state_ != RunState::AtShrine) return false;
    Room& r = *floor_.room(floor_.currentRoom());
    if (r.cleared) return false;
    healParty(1.0f, true);
    spendLight(-40);
    r.cleared = true;
    info(strf("Beristirahat di Ember Shrine. Party pulih penuh, cahaya -> %d.", light_));
    floor_.revealByLight(lightLevel());
    return true;
}

void Run::leaveRoom() {
    if (state_ == RunState::AtShrine || state_ == RunState::InShop) state_ = RunState::Exploring;
}

int Run::price(ShopItem item) const {
    int extra = 10 * (floor_.number() - 1);
    switch (item) {
        case ShopItem::LampOil:   return 40 + extra;
        case ShopItem::Hourglass: return 60 + extra;
        case ShopItem::Elixir:    return 50 + extra;
        default:                  return 9999;
    }
}

bool Run::buy(ShopItem item) {
    if (state_ != RunState::InShop) return false;
    int cost = price(item);
    if (ash_ < cost) return false;
    ash_ -= cost;
    switch (item) {
        case ShopItem::LampOil:   spendLight(-35); floor_.revealByLight(lightLevel()); break;
        case ShopItem::Hourglass: ++hourglass_; break;
        case ShopItem::Elixir:    healParty(0.4f, true); break;
        default: break;
    }
    info(strf("Membeli %s (-%d Ash).", shopItemName(item), cost));
    return true;
}

const EventDef& Run::currentEvent() const { return eventDef(floor_.room(floor_.currentRoom())->eventId); }

bool Run::chooseEventOption(int option) {
    if (state_ != RunState::InEvent || option < 0 || option > 1) return false;
    Room& r = *floor_.room(floor_.currentRoom());
    switch (r.eventId) {
        case 0:   // Altar Berdarah
            if (option == 0) {
                for (int i = 0; i < PARTY_SIZE; ++i) {
                    Unit& u = party_[i];
                    if (!u.alive) continue;
                    u.hp -= u.maxHp / 5;
                    if (u.hp < 1) u.hp = 1;
                }
                ash_ += 80;
                info("Darah menetes ke altar. +80 Ash.");
            }
            break;
        case 1:   // Langit-langit Berderak
            if (option == 0) {
                if (!floor_.collapseCorridor(rng_)) info("Reruntuhan tidak menutup jalan apa pun.");
            } else {
                spendLight(15);
                info(strf("Minyak lentera menahan reruntuhan. Cahaya -> %d.", light_));
            }
            break;
        case 2:   // Lentera Tua
            if (option == 0) {
                spendLight(-30);
                info(strf("Lentera menyala lebih terang. Cahaya -> %d.", light_));
                floor_.revealByLight(lightLevel());
            }
            break;
        case 3:   // Bisikan Katedral
            if (option == 0) {
                for (int i = 0; i < PARTY_SIZE; ++i) {
                    Unit& u = party_[i];
                    if (!u.alive) continue;
                    u.hp -= u.maxHp / 10;
                    if (u.hp < 1) u.hp = 1;
                }
                floor_.revealAll();
                info("Denah lantai terpatri di benakmu.");
            }
            break;
        default:
            break;
    }
    r.cleared = true;
    state_ = RunState::Exploring;
    tryPickUpAsh();
    return true;
}

bool Run::chooseRelic(int i) {
    if (state_ != RunState::ChoosingRelic || i < 0 || i >= relicOptionCount_) return false;
    Relic r = relicOptions_[i];
    relics_ |= 1u << static_cast<int>(r);
    switch (r) {
        case Relic::CrackedClock: hourglass_ += 2; break;
        case Relic::MartyrBlood:
            for (Unit& u : party_) u.atk = static_cast<int>(std::lround(u.atk * 1.15f));
            break;
        case Relic::IronHide:
            for (Unit& u : party_) {
                u.maxHp = static_cast<int>(u.maxHp * 1.2f);
                u.hp = static_cast<int>(u.hp * 1.2f);
            }
            break;
        default: break;   // CinderRing & EternalWick dibaca saat dibutuhkan
    }
    info(strf("Relic diperoleh: %s.", relicName(r)));
    descend();
    return true;
}

void Run::descend() {
    int next = floor_.number() + 1;
    if (next > MAX_FLOORS) {
        state_ = RunState::RunWon;
        info("Nyala terakhir katedral padam di tanganmu. EKSPEDISI SELESAI.");
        return;
    }
    healParty(0.5f, true);
    setupFloor(next);
}
