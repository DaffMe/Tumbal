#include "GameState.hpp"

#include "../util/Json.hpp"

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
    if (companionJoined_) {
        companion_.hp = companion_.stats.mhp;
        companion_.mp = companion_.stats.mmp;
        for (int s = 1; s < ST_COUNT; ++s) companion_.status[s] = 0;
    }
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

// ------------------------------------------------------------------ pendamping

void GameState::joinCompanion() {
    if (companionJoined_) return;
    companionJoined_ = true;
    companionActive_ = true;
    companionLevel_ = level_ > 1 ? level_ - 1 : 1;
    companion_ = makeHeroBattler("Timun Mas", KL_TIMUN_MAS, companionLevel_);
}

int GameState::companionLevelUpCost() const {
    int l = companionLevel_ - 1;
    return 40 + 24 * l + 5 * l * l;
}

bool GameState::companionLevelUp() {
    if (!companionJoined_ || sukma_ < companionLevelUpCost()) return false;
    sukma_ -= companionLevelUpCost();
    ++companionLevel_;
    companion_.stats = statsAtLevel(KL_TIMUN_MAS, companionLevel_);
    companion_.hp = companion_.stats.mhp;
    companion_.mp = companion_.stats.mmp;
    return true;
}

int GameState::party(Battler* out) const {
    int n = 0;
    out[n++] = hero_;
    if (companionActive() && companion_.hp > 0) out[n++] = companion_;
    return n;
}

void GameState::applyParty(const Battler* in, int n) {
    auto copyBack = [](Battler& dst, const Battler& src) {
        dst.hp = src.hp;
        dst.mp = src.mp;
        for (int s = 1; s < ST_COUNT; ++s) dst.status[s] = statusDef(s).battleOnly ? 0 : src.status[s];
    };
    if (n > 0) copyBack(hero_, in[0]);
    if (n > 1 && companionActive()) copyBack(companion_, in[1]);
}

// ------------------------------------------------------------------ simpan / muat

static std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

static void writeBattler(std::string& o, const char* key, const Battler& b) {
    o += std::string("\"") + key + "\":{\"name\":\"" + esc(b.name) + "\",\"hp\":" + std::to_string(b.hp) +
         ",\"mp\":" + std::to_string(b.mp) + ",\"status\":[";
    for (int s = 1; s < ST_COUNT; ++s) o += (s > 1 ? "," : "") + std::to_string(b.status[s]);
    o += "]}";
}

std::string GameState::toJson() const {
    std::string o = "{";
    o += "\"class\":" + std::to_string(classId_) + ",\"level\":" + std::to_string(level_);
    o += ",\"sukma\":" + std::to_string(sukma_) + ",\"jamuMax\":" + std::to_string(jamuMax_);
    o += ",\"deaths\":" + std::to_string(deaths_) + ",";
    writeBattler(o, "hero", hero_);
    o += ",\"items\":[";
    for (int i = 1; i < IT_COUNT; ++i) o += (i > 1 ? "," : "") + std::to_string(items_[i]);
    o += "],\"flags\":{";
    for (int i = 0; i < flagCount_; ++i)
        o += (i ? "," : "") + std::string("\"") + esc(flagName_[i]) + "\":" + std::to_string(flagValue_[i]);
    o += "},\"tungku\":[" + std::to_string(tungkuMap_) + "," + std::to_string(tungkuX_) + "," + std::to_string(tungkuY_) + "]";
    o += ",\"blood\":[" + std::to_string(blood_.active ? 1 : 0) + "," + std::to_string(blood_.map) + "," +
         std::to_string(blood_.x) + "," + std::to_string(blood_.y) + "," + std::to_string(blood_.sukma) + "]";
    o += ",\"companion\":[" + std::to_string(companionJoined_ ? 1 : 0) + "," + std::to_string(companionActive_ ? 1 : 0) +
         "," + std::to_string(companionLevel_) + "],";
    writeBattler(o, "companionState", companion_);
    o += "}";
    return o;
}

static void readBattler(const JsonValue* v, Battler& b) {
    if (v == nullptr) return;
    b.name = v->getString("name", b.name);
    b.hp = v->getInt("hp", b.hp);
    b.mp = v->getInt("mp", b.mp);
    if (const JsonValue* st = v->get("status")) {
        int s = 1;
        for (const JsonValue* c = st->first(); c != nullptr && s < ST_COUNT; c = c->next(), ++s) b.status[s] = c->asInt();
    }
}

bool GameState::fromJson(const JsonValue* v) {
    if (v == nullptr || !v->isObject()) return false;
    *this = GameState();
    classId_ = v->getInt("class");
    level_ = v->getInt("level", 1);
    hero_ = makeHeroBattler("Tumbal", classId_, level_);
    readBattler(v->get("hero"), hero_);
    sukma_ = v->getInt("sukma");
    jamuMax_ = v->getInt("jamuMax", 3);
    deaths_ = v->getInt("deaths");
    if (const JsonValue* it = v->get("items")) {
        int i = 1;
        for (const JsonValue* c = it->first(); c != nullptr && i < IT_COUNT; c = c->next(), ++i) items_[i] = c->asInt();
    }
    if (const JsonValue* fl = v->get("flags"))
        for (const JsonValue* c = fl->first(); c != nullptr; c = c->next()) setFlag(c->key(), c->asInt());
    if (const JsonValue* t = v->get("tungku")) {
        tungkuMap_ = t->at(0) ? t->at(0)->asInt() : 0;
        tungkuX_ = t->at(1) ? t->at(1)->asInt() : 0;
        tungkuY_ = t->at(2) ? t->at(2)->asInt() : 0;
    }
    if (const JsonValue* bl = v->get("blood")) {
        blood_.active = bl->at(0) && bl->at(0)->asInt() != 0;
        blood_.map = bl->at(1) ? bl->at(1)->asInt() : 0;
        blood_.x = bl->at(2) ? bl->at(2)->asInt() : 0;
        blood_.y = bl->at(3) ? bl->at(3)->asInt() : 0;
        blood_.sukma = bl->at(4) ? bl->at(4)->asInt() : 0;
    }
    if (const JsonValue* c = v->get("companion")) {
        companionJoined_ = c->at(0) && c->at(0)->asInt() != 0;
        companionActive_ = c->at(1) && c->at(1)->asInt() != 0;
        companionLevel_ = c->at(2) ? c->at(2)->asInt() : 1;
        if (companionJoined_) {
            companion_ = makeHeroBattler("Timun Mas", KL_TIMUN_MAS, companionLevel_);
            readBattler(v->get("companionState"), companion_);
        }
    }
    return true;
}
