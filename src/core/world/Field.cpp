#include "Field.hpp"

int Actor::walkFrame() const {
    if (!moving) return 0;
    // urutan frame Aurora: 0 diam, 1 langkah kiri, 2 diam, 3 langkah kanan
    int f = static_cast<int>(animTime * speed * 2.0f) % 4;
    return f;
}

bool Field::enter(int mapId, int x, int y, int facing) {
    TileMap* m = world_.map(mapId);
    if (m == nullptr) return false;
    mapId_ = mapId;
    map_ = m;
    player_.x = x;
    player_.y = y;
    player_.dx = player_.dy = 0;
    player_.moving = false;
    player_.progress = 0.0f;
    player_.facing = facing;
    world_.markVisited(mapId);
    return true;
}

bool Field::enterAtSpawn(int mapId, const std::string& spawnName) {
    TileMap* m = world_.map(mapId);
    if (m == nullptr) return false;
    const MapObject* sp = m->findObject(spawnName);
    if (sp == nullptr) return enter(mapId, m->width() / 2, m->height() / 2, FACE_DOWN);
    return enter(mapId, sp->tileX(m->tileW()), sp->tileY(m->tileH()), sp->props.getInt("dir", FACE_DOWN));
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

static int facingFor(int sx, int sy, int current) {
    // diagonal → hadap horizontal (seperti RPG Maker 8 arah)
    if (sx < 0) return FACE_LEFT;
    if (sx > 0) return FACE_RIGHT;
    if (sy < 0) return FACE_UP;
    if (sy > 0) return FACE_DOWN;
    return current;
}

FieldEvent Field::update(float dt, int inX, int inY, bool run) {
    FieldEvent ev;
    Actor& p = player_;
    const float spd = run ? 6.5f : 4.0f;

    if (p.moving) {
        p.speed = spd;
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
        ev = arrive();
        if (ev.kind != FieldEvent::None) return ev;
        // lanjut jalan mulus bila tombol masih ditekan
        if ((inX != 0 || inY != 0)) {
            int sx = inX, sy = inY;
            if (!canStep(p.x, p.y, sx, sy)) {
                // geser sepanjang tembok: coba komponen tunggal
                if (sx != 0 && sy != 0) {
                    if (canStep(p.x, p.y, sx, 0)) sy = 0;
                    else if (canStep(p.x, p.y, 0, sy)) sx = 0;
                }
            }
            p.facing = facingFor(sx, sy, p.facing);
            if (canStep(p.x, p.y, sx, sy)) {
                p.dx = sx; p.dy = sy; p.moving = true; p.progress = leftover;
            }
        }
        if (!p.moving) p.animTime = 0.0f;
        return ev;
    }

    if (inX == 0 && inY == 0) return ev;
    int sx = inX, sy = inY;
    if (!canStep(p.x, p.y, sx, sy) && sx != 0 && sy != 0) {
        if (canStep(p.x, p.y, sx, 0)) sy = 0;
        else if (canStep(p.x, p.y, 0, sy)) sx = 0;
    }
    p.facing = facingFor(sx, sy, p.facing);
    if (canStep(p.x, p.y, sx, sy)) {
        p.dx = sx;
        p.dy = sy;
        p.moving = true;
        p.progress = 0.0f;
        p.speed = spd;
    } else {
        // menabrak: mungkin pintu di luar batas/ terhalang yang bisa dipicu
        int tx = p.x + inX, ty = p.y + inY;
        if (const MapObject* door = map_->objectAt(tx, ty, "door")) {
            FieldEvent e;
            e.travel = world_.travel(mapId_, *door);
            e.object = door;
            e.kind = e.travel.ok ? FieldEvent::Transfer : (e.travel.locked ? FieldEvent::LockedDoor : FieldEvent::None);
            return e;
        }
    }
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
