#include "Field.hpp"

#include <cstdlib>

int Actor::walkFrame() const {
    if (!moving) return 0;
    // urutan frame jalan 4 langkah: 0 diam, 1 kaki kiri, 2 diam, 3 kaki kanan
    return static_cast<int>(animTime * speed * 2.0f) % 4;
}

static void facingVec(int facing, int& vx, int& vy) {
    vx = (facing == FACE_LEFT) ? -1 : (facing == FACE_RIGHT ? 1 : 0);
    vy = (facing == FACE_UP) ? -1 : (facing == FACE_DOWN ? 1 : 0);
}

static int facingFor(int sx, int sy, int current) {
    // diagonal → hadap horizontal (seperti RPG Maker 8 arah)
    if (sx < 0) return FACE_LEFT;
    if (sx > 0) return FACE_RIGHT;
    if (sy < 0) return FACE_UP;
    if (sy > 0) return FACE_DOWN;
    return current;
}

// ------------------------------------------------------------------ peta

bool Field::enter(int mapId, int x, int y, int facing) {
    TileMap* m = world_.map(mapId);
    if (m == nullptr) return false;
    bool sameMap = (m == map_);
    mapId_ = mapId;
    map_ = m;
    player_ = Actor();
    player_.x = x;
    player_.y = y;
    player_.facing = facing;
    world_.markVisited(mapId);
    if (!sameMap || enemyCount_ == 0) loadEnemies();
    return true;
}

bool Field::enterAtSpawn(int mapId, const std::string& spawnName) {
    TileMap* m = world_.map(mapId);
    if (m == nullptr) return false;
    const MapObject* sp = m->findObject(spawnName);
    if (sp == nullptr) return enter(mapId, m->width() / 2, m->height() / 2, FACE_DOWN);
    return enter(mapId, sp->tileX(m->tileW()), sp->tileY(m->tileH()), sp->props.getInt("dir", FACE_DOWN));
}

void Field::loadEnemies() {
    enemyCount_ = 0;
    int idx = 0;
    for (const MapObject* o = map_->objects(); o != nullptr && enemyCount_ < MAX_ENEMIES; o = o->next) {
        if (o->type != "enemy") continue;
        int myIndex = idx++;
        if (world_.isDefeated(mapId_, myIndex)) continue;
        FieldEnemy& e = enemies_[enemyCount_++];
        e = FieldEnemy();
        e.index = myIndex;
        e.troop = o->props.getInt("troop", 1);
        e.walker = o->props.getString("walker", "assets/chars/tumbal_gagal.png");
        e.actor.x = e.homeX = o->tileX(map_->tileW());
        e.actor.y = e.homeY = o->tileY(map_->tileH());
        e.actor.facing = o->props.getInt("dir", FACE_DOWN);
        e.think = 0.3f + rng_.unit() * 1.5f;
    }
}

void Field::respawnEnemies() { loadEnemies(); }

void Field::resolveEncounter(int i, bool won) {
    if (i < 0 || i >= enemyCount_) return;
    FieldEnemy& e = enemies_[i];
    if (won) {
        e.alive = false;
        world_.markDefeated(mapId_, e.index);
    } else {
        e.cooldown = 3.0f;      // kabur: musuh tertegun sebentar
        e.alerted = false;
        e.actor.moving = false;
        e.actor.progress = 0.0f;
    }
}

int Field::enemyAt(int x, int y) const {
    for (int i = 0; i < enemyCount_; ++i) {
        const FieldEnemy& e = enemies_[i];
        if (!e.alive) continue;
        if ((e.actor.x == x && e.actor.y == y) || (e.actor.destX() == x && e.actor.destY() == y)) return i;
    }
    return -1;
}

bool Field::canStep(int x, int y, int sx, int sy) const {
    if (map_ == nullptr || (sx == 0 && sy == 0)) return false;
    if (map_->blocked(x + sx, y + sy)) return false;
    if (sx != 0 && sy != 0) {
        // diagonal: jangan memotong sudut
        if (map_->blocked(x + sx, y) || map_->blocked(x, y + sy)) return false;
    }
    return true;
}

// ------------------------------------------------------------------ pemain

// Mulai langkah pemain. Menabrak musuh = encounter (cek arah untuk inisiatif).
bool Field::tryPlayerStep(int sx, int sy, float leftover, FieldEvent& ev) {
    Actor& p = player_;
    if (!canStep(p.x, p.y, sx, sy) && sx != 0 && sy != 0) {
        // geser sepanjang tembok: coba komponen tunggal
        if (canStep(p.x, p.y, sx, 0)) sy = 0;
        else if (canStep(p.x, p.y, 0, sy)) sx = 0;
    }
    p.facing = facingFor(sx, sy, p.facing);
    int ei = enemyAt(p.x + sx, p.y + sy);
    if (ei >= 0 && enemies_[ei].cooldown <= 0.0f) {
        const FieldEnemy& e = enemies_[ei];
        int fx, fy;
        facingVec(e.actor.facing, fx, fy);
        ev.kind = FieldEvent::Encounter;
        ev.troop = e.troop;
        ev.enemyIndex = ei;
        // pemain melangkah searah hadap musuh → menyentuh punggungnya
        ev.initiative = (sx * fx + sy * fy > 0) ? 1 : 0;
        return false;
    }
    if (ei >= 0) return false;   // musuh tertegun menghalangi jalan
    if (!canStep(p.x, p.y, sx, sy)) {
        // menabrak: mungkin pintu di tile terhalang
        if (const MapObject* door = map_->objectAt(p.x + sx, p.y + sy, "door")) {
            ev.travel = world_.travel(mapId_, *door);
            ev.object = door;
            ev.kind = ev.travel.ok ? FieldEvent::Transfer : (ev.travel.locked ? FieldEvent::LockedDoor : FieldEvent::None);
        }
        return false;
    }
    p.dx = sx;
    p.dy = sy;
    p.moving = true;
    p.progress = leftover;
    return true;
}

FieldEvent Field::update(float dt, int inX, int inY, bool run) {
    FieldEvent ev;
    Actor& p = player_;
    running_ = run;
    const float spd = run ? 6.5f : 4.0f;
    p.speed = spd;

    // musuh bergerak dulu (bisa memicu sergapan)
    for (int i = 0; i < enemyCount_ && ev.kind == FieldEvent::None; ++i) updateEnemy(i, dt, ev);
    if (ev.kind != FieldEvent::None) return ev;

    if (p.moving) {
        p.animTime += dt;
        float step = spd * dt * ((p.dx != 0 && p.dy != 0) ? 0.7071f : 1.0f);
        p.progress += step;
        if (p.progress < 1.0f) return ev;
        // tiba di tile baru
        p.x += p.dx;
        p.y += p.dy;
        float leftover = p.progress - 1.0f;
        p.progress = 0.0f;
        p.moving = false;
        p.dx = p.dy = 0;
        ev = arrive();
        if (ev.kind != FieldEvent::None) return ev;
        // lanjut jalan mulus bila tombol masih ditekan
        if (inX != 0 || inY != 0) tryPlayerStep(inX, inY, leftover, ev);
        if (!p.moving) p.animTime = 0.0f;
        return ev;
    }

    if (inX == 0 && inY == 0) return ev;
    tryPlayerStep(inX, inY, 0.0f, ev);
    return ev;
}

FieldEvent Field::arrive() {
    FieldEvent ev;
    if (const MapObject* door = map_->objectAt(player_.x, player_.y, "door")) {
        ev.travel = world_.travel(mapId_, *door);
        ev.object = door;
        ev.kind = ev.travel.ok ? FieldEvent::Transfer : (ev.travel.locked ? FieldEvent::LockedDoor : FieldEvent::None);
    }
    return ev;
}

// ------------------------------------------------------------------ musuh

bool Field::enemyCanStep(int self, int sx, int sy) const {
    const Actor& a = enemies_[self].actor;
    if (!canStep(a.x, a.y, sx, sy)) return false;
    int tx = a.x + sx, ty = a.y + sy;
    if (map_->objectAt(tx, ty, "door") != nullptr) return false;   // musuh tidak lewat pintu
    for (int i = 0; i < enemyCount_; ++i) {
        if (i == self || !enemies_[i].alive) continue;
        const Actor& o = enemies_[i].actor;
        if ((o.x == tx && o.y == ty) || (o.destX() == tx && o.destY() == ty)) return false;
    }
    return true;
}

void Field::startEnemyStep(int i, int sx, int sy, FieldEvent& ev) {
    FieldEnemy& e = enemies_[i];
    Actor& a = e.actor;
    a.facing = facingFor(sx, sy, a.facing);
    int tx = a.x + sx, ty = a.y + sy;
    // menabrak pemain → battle
    bool hitsPlayer = (player_.x == tx && player_.y == ty) || (player_.destX() == tx && player_.destY() == ty);
    if (hitsPlayer && e.cooldown <= 0.0f) {
        int fx, fy;
        facingVec(player_.facing, fx, fy);
        ev.kind = FieldEvent::Encounter;
        ev.troop = e.troop;
        ev.enemyIndex = i;
        // musuh melangkah searah hadap pemain → datang dari belakang
        ev.initiative = (sx * fx + sy * fy > 0) ? 2 : 0;
        return;
    }
    if (hitsPlayer || !enemyCanStep(i, sx, sy)) return;
    a.dx = sx;
    a.dy = sy;
    a.moving = true;
    a.progress = 0.0f;
}

void Field::updateEnemy(int i, float dt, FieldEvent& ev) {
    FieldEnemy& e = enemies_[i];
    if (!e.alive) return;
    Actor& a = e.actor;
    if (e.cooldown > 0.0f) e.cooldown -= dt;
    if (e.alertFlash > 0.0f) e.alertFlash -= dt;

    int distX = player_.x - a.x, distY = player_.y - a.y;
    int dist = std::abs(distX) + std::abs(distY);
    int sense = running_ ? 5 : 3;
    if (!e.alerted && e.cooldown <= 0.0f && dist <= sense) {
        e.alerted = true;
        e.alertFlash = 0.8f;
        e.think = 0.35f;           // jeda kaget sebelum mengejar
    } else if (e.alerted && dist > 9) {
        e.alerted = false;
    }
    a.speed = e.alerted ? 4.4f : 1.8f;

    if (a.moving) {
        a.animTime += dt;
        a.progress += a.speed * dt;
        if (a.progress >= 1.0f) {
            a.x += a.dx;
            a.y += a.dy;
            a.dx = a.dy = 0;
            a.moving = false;
            a.progress = 0.0f;
        }
        return;
    }
    e.think -= dt;
    if (e.think > 0.0f) return;

    if (e.alerted && e.cooldown <= 0.0f) {
        // kejar: pilih sumbu dengan selisih terbesar, lalu sumbu lain
        int sx = (distX > 0) - (distX < 0), sy = (distY > 0) - (distY < 0);
        bool horizFirst = std::abs(distX) >= std::abs(distY);
        int ax = horizFirst ? sx : 0, ay = horizFirst ? 0 : sy;
        int bx = horizFirst ? 0 : sx, by = horizFirst ? sy : 0;
        if (ax == 0 && ay == 0) { ax = bx; ay = by; }
        startEnemyStep(i, ax, ay, ev);
        if (!a.moving && ev.kind == FieldEvent::None && (bx != 0 || by != 0)) startEnemyStep(i, bx, by, ev);
        e.think = 0.0f;
        return;
    }
    // berkeliaran di sekitar titik asal
    e.think = 0.6f + rng_.unit() * 1.6f;
    int r = rng_.range(0, 5);
    int sx = 0, sy = 0;
    if (r == 0) sx = -1; else if (r == 1) sx = 1; else if (r == 2) sy = -1; else if (r == 3) sy = 1;
    if (sx == 0 && sy == 0) return;   // diam
    if (std::abs(a.x + sx - e.homeX) > 3 || std::abs(a.y + sy - e.homeY) > 3) { sx = -sx; sy = -sy; }
    startEnemyStep(i, sx, sy, ev);
}
