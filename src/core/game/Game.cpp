#include "Game.hpp"

#include "../util/DsTrace.hpp"

Game::Game() : field_(world_), interp_(state_, events_, *this) {}

bool Game::load(const std::string& worldJson, const std::string* eventFiles, int eventFileCount, std::string* error) {
    if (!world_.load(worldJson, error)) return false;
    for (int i = 0; i < eventFileCount; ++i)
        if (!events_.loadFile(eventFiles[i], error)) return false;
    return true;
}

void Game::newGame(const std::string& heroName, int classId) {
    state_.newGame(heroName, classId);
    field_.enterAtSpawn(world_.startMap(), world_.startSpawn());
    state_.setLastTungku(field_.mapId(), field_.player().x, field_.player().y);
    enteredMap_ = true;
    runAutoEvents();
}

void Game::notice(const std::string& s) {
    notices_.enqueue(s);
}

bool Game::popNotice(std::string& out) {
    // notifikasi dari interpreter digabung ke antrean utama (urutan tetap FIFO)
    std::string s;
    while (interp_.notices().dequeue(s)) notices_.enqueue(s);
    return notices_.dequeue(out);
}

bool Game::inDialog() const {
    Interpreter::State s = interp_.state();
    return s == Interpreter::WaitMessage || s == Interpreter::WaitChoice;
}

// ------------------------------------------------------------------ update

void Game::update(float dt, const GameInput& in) {
    enteredMap_ = false;
    if (battlePending_ || transferPending_ || tungkuMenu_) return;
    if (interp_.busy()) {
        interp_.update();
        return;             // dialog & cutscene membekukan peta
    }
    if (in.action && !field_.player().moving) {
        interact();
        if (interp_.busy() || battlePending_ || transferPending_) return;
    }
    FieldEvent ev = field_.update(dt, in.dx, in.dy, in.run);
    switch (ev.kind) {
        case FieldEvent::Transfer:
            requestTransfer(ev.travel.toMap, ev.travel.x, ev.travel.y, ev.travel.dir, false);
            break;
        case FieldEvent::LockedDoor:
            notice("Terkunci dari sisi lain.");
            break;
        case FieldEvent::Encounter:
            battlePending_ = true;
            battleTroop_ = ev.troop;
            battleInit_ = ev.initiative;
            battleEnemyIndex_ = ev.enemyIndex;
            break;
        case FieldEvent::Touch: {
            std::string name = ev.object->props.getString("event");
            std::string once = ev.object->props.getString("once");
            if (once.empty() || state_.flag(once) == 0) interp_.runEvent(name);
            break;
        }
        case FieldEvent::Arrived:
            checkBloodstain();
            break;
        default: break;
    }
}

void Game::interact() {
    const Actor& p = field_.player();
    int fx = p.x + (p.facing == FACE_LEFT ? -1 : p.facing == FACE_RIGHT ? 1 : 0);
    int fy = p.y + (p.facing == FACE_UP ? -1 : p.facing == FACE_DOWN ? 1 : 0);
    TileMap* m = field_.map();
    const int tiles[2][2] = {{fx, fy}, {p.x, p.y}};
    for (const auto& t : tiles) {
        for (const MapObject* o = m->objects(); o != nullptr; o = o->next) {
            int ox = o->tileX(m->tileW()), oy = o->tileY(m->tileH());
            if (ox != t[0] || oy != t[1]) continue;
            if (o->type == "tungku") { restAt(*o); return; }
            if (o->type == "loot" && tryLoot(o)) return;
            if (o->type == "npc" || (o->type == "event" && o->props.getString("trigger", "action") == "action")) {
                std::string once = o->props.getString("once");
                if (!once.empty() && state_.flag(once) != 0) {
                    std::string after = o->props.getString("after");
                    if (!after.empty()) interp_.runEvent(after);
                    return;
                }
                interp_.runEvent(o->props.getString("event"));
                return;
            }
        }
    }
}

bool Game::tryLoot(const MapObject* o) {
    int idx = field_.map()->typeIndex(o);
    if (world_.isCollected(field_.mapId(), idx)) return false;
    std::string ev = o->props.getString("event");
    world_.markCollected(field_.mapId(), idx);
    if (!ev.empty()) {
        interp_.runEvent(ev);
        return true;
    }
    int item = itemIdByKey(o->props.getString("item").c_str());
    int n = o->props.getInt("n", 1);
    if (const ItemDef* d = itemDef(item)) {
        state_.addItem(item, n);
        notice(std::string("Didapat: ") + d->name + (n > 1 ? " x" + std::to_string(n) : ""));
    }
    return true;
}

void Game::restAt(const MapObject& t) {
    state_.rest();
    int revived = world_.restAtTungku();
    field_.respawnEnemies();
    int tx = t.tileX(field_.map()->tileW()), ty = t.tileY(field_.map()->tileH());
    // titik bangkit = tile kosong di sebelah Tungku
    int bx = tx - 1, by = ty;
    if (field_.map()->blocked(bx, by)) { bx = tx + 1; }
    if (field_.map()->blocked(bx, by)) { bx = tx; by = ty + 1; }
    state_.setLastTungku(field_.mapId(), bx, by);
    world_.lightTungku(field_.mapId());
    tungkuMenu_ = true;
    notice("Kamu beristirahat di Tungku. HP pulih, Ramuan Jamu terisi.");
    if (revived > 0) notice("...musuh-musuh bangkit kembali.");
}

void Game::runAutoEvents() {
    for (const MapObject* o = field_.map()->objects(); o != nullptr; o = o->next) {
        if (o->type != "event" || o->props.getString("trigger") != "auto") continue;
        std::string once = o->props.getString("once");
        if (!once.empty() && state_.flag(once) != 0) continue;
        if (interp_.runEvent(o->props.getString("event"))) return;
    }
}

void Game::checkBloodstain() {
    const Actor& p = field_.player();
    int got = state_.retrieveBloodstain(field_.mapId(), p.x, p.y);
    if (got > 0) notice("Sukma yang tercecer kembali: +" + std::to_string(got));
}

// ------------------------------------------------------------------ battle

void Game::setupBattle(BattleSystem& bs, std::uint32_t seed) {
    bs.setup(&state_.hero(), 1, battleTroop_, static_cast<Initiative>(battleInit_), seed);
    bs.setInventory(&state_);
}

void Game::endBattle(const BattleSystem& bs) {
    battlePending_ = false;
    const Battler& b = bs.battler(0);
    Battler& h = state_.hero();
    h.hp = b.hp;
    h.mp = b.mp;
    for (int s = 1; s < ST_COUNT; ++s) h.status[s] = statusDef(s).battleOnly ? 0 : b.status[s];

    BattleSystem::Phase r = bs.phase();
    bool fromEvent = battleEnemyIndex_ < 0;
    if (r == BattleSystem::Victory) {
        state_.addSukma(bs.sukmaReward());
        notice("Sukma +" + std::to_string(bs.sukmaReward()) + "   (dibawa: " + std::to_string(state_.sukma()) + ")");
        if (!fromEvent) field_.resolveEncounter(battleEnemyIndex_, true);
    } else if (r == BattleSystem::Escaped) {
        if (!fromEvent) field_.resolveEncounter(battleEnemyIndex_, false);
    } else {
        // mati: sukma tercecer di sini, bangkit di Tungku, semua musuh hidup lagi
        const Actor& p = field_.player();
        int lost = state_.die(field_.mapId(), p.x, p.y);
        dsOp("GameState.die() -> sukma tercecer di peta " + std::to_string(field_.mapId()));
        if (lost > 0) notice(std::to_string(lost) + " sukma lama hilang selamanya.");
        world_.restAtTungku();
        if (interp_.busy()) interp_.abort();   // batalkan cutscene
        requestTransfer(state_.tungkuMap(), state_.tungkuX(), state_.tungkuY(), FACE_DOWN, true);
        battleEnemyIndex_ = -1;
        return;
    }
    battleEnemyIndex_ = -1;
    if (fromEvent && interp_.state() == Interpreter::WaitBattle) interp_.resume();
}

// ------------------------------------------------------------------ transfer

void Game::requestTransfer(int map, int x, int y, int dir, bool respawn) {
    transferPending_ = true;
    transferMap_ = map;
    transferX_ = x;
    transferY_ = y;
    transferDir_ = dir;
    transferSpawn_.clear();
    respawnAfterTransfer_ = respawn;
}

void Game::finishTransfer() {
    if (!transferPending_) return;
    transferPending_ = false;
    if (!transferSpawn_.empty()) field_.enterAtSpawn(transferMap_, transferSpawn_);
    else field_.enter(transferMap_, transferX_, transferY_, transferDir_);
    if (respawnAfterTransfer_) field_.respawnEnemies();
    enteredMap_ = true;
    if (interp_.state() == Interpreter::WaitTransfer) interp_.resume();
    else runAutoEvents();
}

// ------------------------------------------------------------------ menu

std::string Game::useItem(int itemId) {
    const ItemDef* d = itemDef(itemId);
    if (d == nullptr || state_.count(itemId) <= 0) return "";
    if (d->kind == IK_PULANG) {
        state_.removeItem(itemId);
        requestTransfer(state_.tungkuMap(), state_.tungkuX(), state_.tungkuY(), FACE_DOWN, false);
        return "Abu Tungku membawamu pulang.";
    }
    return state_.useItemInField(itemId);
}

// ------------------------------------------------------------------ EventHost

void Game::hostStartBattle(int troopId) {
    battlePending_ = true;
    battleTroop_ = troopId;
    battleInit_ = INIT_NORMAL;
    battleEnemyIndex_ = -1;
}

void Game::hostTransfer(int mapId, const std::string& spawn) {
    requestTransfer(mapId, 0, 0, FACE_DOWN, false);
    transferSpawn_ = spawn;
}

void Game::hostUnlock(int a, int b) {
    if (world_.unlockShortcut(a, b)) notice("Jalan pintas terbuka.");
}

void Game::hostHeal() { state_.rest(); }

// ------------------------------------------------------------------ Tungku

std::string Game::levelUp() {
    int cost = state_.levelUpCost();
    if (!state_.levelUp()) return "Sukma tidak cukup (butuh " + std::to_string(cost) + ").";
    return "Naik ke level " + std::to_string(state_.level()) + ".";
}

int Game::warpTargets(int* outMap, int* outDepth, int max) {
    return world_.warpTargets(field_.mapId(), outMap, outDepth, max);
}

bool Game::warpTo(int mapId) {
    const MapInfo* mi = world_.info(mapId);
    if (mi == nullptr || !mi->tungkuLit) return false;
    // tujuan harus terjangkau lewat jalan terbuka (diperiksa lewat BFS)
    int maps[16], depth[16];
    int n = warpTargets(maps, depth, 16);
    bool ok = false;
    for (int i = 0; i < n; ++i) ok = ok || maps[i] == mapId;
    if (!ok) return false;
    int x = mi->tungkuX - 1, y = mi->tungkuY;
    if (mi->map->blocked(x, y)) x = mi->tungkuX + 1;
    if (mi->map->blocked(x, y)) { x = mi->tungkuX; y = mi->tungkuY + 1; }
    tungkuMenu_ = false;
    state_.setLastTungku(mapId, x, y);
    requestTransfer(mapId, x, y, FACE_DOWN, false);
    return true;
}
