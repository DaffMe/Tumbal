#include "GameState.hpp"

void GameState::newGame(const std::string& name, int classId) {
    *this = GameState();
    classId_ = classId;
    level_ = 1;
    hero_ = makeHeroBattler(name, classId, level_);
    jamuMax_ = 3;
    items_[IT_JAMU] = jamuMax_;
    if (classId == KL_MALING) sukma_ = 100;   // Maling membawa sukma curian (seperti BS1)
}

int GameState::count(int itemId) const {
    return (itemId > 0 && itemId < IT_COUNT) ? items_[itemId] : 0;
}

void GameState::addItem(int itemId, int n) {
    if (itemId <= 0 || itemId >= IT_COUNT) return;
    items_[itemId] += n;
    if (items_[itemId] > 99) items_[itemId] = 99;
}

bool GameState::removeItem(int itemId, int n) {
    if (count(itemId) < n) return false;
    items_[itemId] -= n;
    return true;
}

std::string GameState::useItemInField(int itemId) {
    const ItemDef* d = itemDef(itemId);
    if (d == nullptr || !d->inField || count(itemId) <= 0) return "";
    Battler& h = hero_;
    switch (d->kind) {
        case IK_PULIH_PERSEN: {
            if (h.hp >= h.stats.mhp) return "";
            int amt = h.stats.mhp * d->value / 100;
            h.hp += amt;
            if (h.hp > h.stats.mhp) h.hp = h.stats.mhp;
            removeItem(itemId);
            return "HP pulih " + std::to_string(amt) + ".";
        }
        case IK_OBAT: {
            bool any = h.status[ST_RACUN] > 0 || h.status[ST_BERDARAH] > 0;
            if (!any) return "";
            h.status[ST_RACUN] = 0;
            h.status[ST_BERDARAH] = 0;
            removeItem(itemId);
            return "Racun dan luka mereda.";
        }
        case IK_SUKMA:
            removeItem(itemId);
            addSukma(d->value);
            return "Sukma +" + std::to_string(d->value) + ".";
        default:
            return "";
    }
}

int GameState::flag(const std::string& name) const {
    for (int i = 0; i < flagCount_; ++i)
        if (flagName_[i] == name) return flagValue_[i];
    return 0;
}

void GameState::setFlag(const std::string& name, int value) {
    for (int i = 0; i < flagCount_; ++i) {
        if (flagName_[i] == name) { flagValue_[i] = value; return; }
    }
    if (flagCount_ < MAX_FLAGS) {
        flagName_[flagCount_] = name;
        flagValue_[flagCount_] = value;
        ++flagCount_;
    }
}

void GameState::rest() {
    hero_.hp = hero_.stats.mhp;
    hero_.mp = hero_.stats.mmp;
    for (int s = 1; s < ST_COUNT; ++s) hero_.status[s] = 0;
    if (items_[IT_JAMU] < jamuMax_) items_[IT_JAMU] = jamuMax_;
}

int GameState::die(int map, int x, int y) {
    int lost = blood_.active ? blood_.sukma : 0;   // ceceran lama hilang selamanya
    blood_.active = sukma_ > 0;
    blood_.map = map;
    blood_.x = x;
    blood_.y = y;
    blood_.sukma = sukma_;
    sukma_ = 0;
    ++deaths_;
    rest();                                         // bangkit di Tungku dalam keadaan pulih
    return lost;
}

int GameState::retrieveBloodstain(int map, int x, int y) {
    if (!blood_.active || blood_.map != map || blood_.x != x || blood_.y != y) return 0;
    int got = blood_.sukma;
    sukma_ += got;
    blood_ = Bloodstain();
    return got;
}

int GameState::levelUpCost() const {
    // kurva menanjak: 60, 96, 144, 204, 276, ...
    int l = level_ - 1;
    return 60 + 30 * l + 6 * l * l;
}

bool GameState::levelUp() {
    int cost = levelUpCost();
    if (sukma_ < cost) return false;
    sukma_ -= cost;
    ++level_;
    Stats s = statsAtLevel(classId_, level_);
    hero_.stats = s;
    hero_.hp = s.mhp;   // naik level di Tungku = tubuh pulih
    hero_.mp = s.mmp;
    return true;
}
