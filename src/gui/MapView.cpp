#include "MapView.hpp"

#include <cmath>
#include "Art.hpp"
#include "Sfx.hpp"

using namespace gfx;

namespace {

const char* floorName(int n) {
    switch (n) {
        case 1:  return "Nave Para Pendosa";
        case 2:  return "Krypta Berbisik";
        default: return "Jantung Nyala";
    }
}

const char* lightName(LightLevel l) {
    switch (l) {
        case LightLevel::Bright: return "Terang";
        case LightLevel::Dim:    return "Redup";
        default:                 return "Gelap";
    }
}

Color roomColor(RoomType t) {
    switch (t) {
        case RoomType::Shrine:   return col::ember;
        case RoomType::Boss:     return col::blood;
        case RoomType::Elite:    return Color{200, 110, 200, 255};
        case RoomType::Merchant: return Color{150, 200, 120, 255};
        case RoomType::Treasure: return col::gold;
        case RoomType::Event:    return Color{140, 170, 230, 255};
        default:                 return col::ink;
    }
}

}  // namespace

Vector2 MapView::roomPos(int c, int r) { return {150.0f + c * 162.0f, 140.0f + r * 116.0f}; }

void MapView::reset() {
    hovered_ = -1;
    pathFor_ = -1;
    pathLen_ = 0;
    toasts_.clear();
    partyPos_ = {0, 0};
    lightVis_ = MAX_LIGHT;
}

void MapView::computePath(Run& run, int target) {
    pathFor_ = target;
    pathLen_ = 0;
    pathCost_ = -1;
    if (target < 0 || target == run.floor().currentRoom()) return;
    Stack<int> path;
    pathCost_ = run.floor().findPath(target, path);   // Dijkstra di core
    int id;
    while (path.pop(id) && pathLen_ < FLOOR_COLS * FLOOR_ROWS) pathIds_[pathLen_++] = id;
}

// ============================================================================

void MapView::update(Run& run, float dt, DsLog& dsLog) {
    // event run → toast & log DS
    BattleEvent e;
    while (run.pollEvent(e)) {
        if (e.type == EventType::DsOp) dsLog.push(e.text);
        else {
            toasts_.push_back(e.text);
            while (toasts_.size() > 5) toasts_.pop_front();
            toastT_ = 5.0f;
        }
    }
    toastT_ = std::fmax(0.0f, toastT_ - dt);

    const Room* cur = run.floor().room(run.floor().currentRoom());
    Vector2 target = roomPos(cur->col, cur->row);
    if (partyPos_.x == 0 && partyPos_.y == 0) partyPos_ = target;
    partyPos_.x = approach(partyPos_.x, target.x, 7.0f, dt);
    partyPos_.y = approach(partyPos_.y, target.y, 7.0f, dt);
    lightVis_ = approach(lightVis_, static_cast<float>(run.light()), 4.0f, dt);

    if (run.state() != RunState::Exploring) {
        hovered_ = -1;
        return;
    }

    // hover ruangan
    int h = -1;
    Floor& f = run.floor();
    f.graph().forEachVertex([&](Floor::Vertex* v) {
        bool visible = v->data.revealed || f.isAdjacent(f.currentRoom(), v->id);
        Vector2 p = roomPos(v->data.col, v->data.row);
        if (visible && CheckCollisionPointCircle(mouse(), p, 30)) h = v->id;
    });
    hovered_ = h;
    if (hovered_ != pathFor_) {
        bool known = hovered_ >= 0 && f.room(hovered_)->revealed && !f.isAdjacent(f.currentRoom(), hovered_);
        if (known) computePath(run, hovered_);
        else { pathFor_ = hovered_; pathLen_ = 0; pathCost_ = -1; }
    }

    if (hovered_ >= 0 && clicked()) {
        sfx::play(sfx::Id::Step);
        if (f.isAdjacent(f.currentRoom(), hovered_)) run.travelTo(hovered_);
        else if (f.room(hovered_)->revealed) run.autoTravel(hovered_);
        pathFor_ = -1;
    }
    if (IsKeyPressed(KEY_S)) {
        run.travelToNearestShrine();
        pathFor_ = -1;
    }
}

gfx::Post MapView::post(const Run& run) const {
    gfx::Post p;
    p.darkness = 0.84f;
    p.lightPos = partyPos_;
    p.lightRadius = 150.0f + lightVis_ * 4.4f;
    p.tint = run.lightLevel() == LightLevel::Dark ? Color{220, 200, 230, 255} : Color{255, 230, 200, 255};
    p.hudTop = 64;
    p.hudBottom = 596;
    return p;
}

// ============================================================================

void MapView::draw(Run& run) {
    art::drawMapBackground(gfx::time());
    drawGraph(run);
    ambientEmbers(3.0f);
    drawParticles();
    drawHud(run);
    drawToasts();
    if (run.state() == RunState::Exploring) drawTooltip(run);

    switch (run.state()) {
        case RunState::AtShrine:      drawShrine(run); break;
        case RunState::InShop:        drawShop(run); break;
        case RunState::InEvent:       drawEvent(run); break;
        case RunState::ChoosingRelic: drawRelic(run); break;
        default: break;
    }
}

void MapView::drawGraph(Run& run) {
    Floor& f = run.floor();
    int cur = f.currentRoom();
    float t = gfx::time();

    auto onPath = [&](int a, int b) {
        for (int i = 0; i + 1 < pathLen_; ++i)
            if ((pathIds_[i] == a && pathIds_[i + 1] == b) || (pathIds_[i] == b && pathIds_[i + 1] == a)) return true;
        return false;
    };

    // ---- edge (koridor)
    f.graph().forEachVertex([&](Floor::Vertex* v) {
        for (auto* e = v->edges; e != nullptr; e = e->next) {
            Floor::Vertex* w = e->to;
            if (v->id > w->id) continue;   // tiap koridor sekali
            bool bothKnown = v->data.revealed && w->data.revealed;
            bool fromHere = (v->id == cur || w->id == cur);
            if (!bothKnown && !fromHere) continue;
            Vector2 a = roomPos(v->data.col, v->data.row);
            Vector2 b = roomPos(w->data.col, w->data.row);
            bool hl = onPath(v->id, w->id) || (fromHere && (v->id == hovered_ || w->id == hovered_));
            Color c = hl ? col::emberHot : (bothKnown ? Color{120, 100, 76, 255} : Color{80, 68, 56, 160});
            float thick = hl ? 4.0f : 2.5f;
            if (bothKnown) {
                DrawLineEx(a, b, thick + 3, fade(BLACK, 0.5f));
                DrawLineEx(a, b, thick, c);
            } else {   // koridor ke ruang belum terungkap: putus-putus
                for (int s = 0; s < 10; s += 2) {
                    Vector2 p0 = {a.x + (b.x - a.x) * s / 10.0f, a.y + (b.y - a.y) * s / 10.0f};
                    Vector2 p1 = {a.x + (b.x - a.x) * (s + 1) / 10.0f, a.y + (b.y - a.y) * (s + 1) / 10.0f};
                    DrawLineEx(p0, p1, 2, c);
                }
            }
            // bobot koridor = titik-titik kecil di tengah
            Vector2 m = {(a.x + b.x) / 2, (a.y + b.y) / 2};
            for (int k = 0; k < e->weight; ++k)
                diamond({m.x + (k - (e->weight - 1) / 2.0f) * 10.0f, m.y}, 3.5f, hl ? col::emberHot : col::gold, true);
        }
    });

    // ---- vertex (ruangan)
    f.graph().forEachVertex([&](Floor::Vertex* v) {
        const Room& r = v->data;
        Vector2 p = roomPos(r.col, r.row);
        bool adj = f.isAdjacent(cur, v->id);
        bool hov = (v->id == hovered_);
        if (!r.revealed) {
            if (!adj) return;
            DrawCircleV(p, 22, Color{20, 16, 18, 230});
            DrawCircleLinesV(p, 22, hov ? col::emberHot : col::inkFaint);
            text(head(), "?", p.x, p.y - 13, 26, hov ? col::emberHot : col::inkFaint, Center);
            return;
        }
        bool done = r.cleared && r.type != RoomType::Entrance;
        Color rc = roomColor(r.type);
        float rad = (r.type == RoomType::Boss) ? 34.0f : 28.0f;
        if (r.type == RoomType::Shrine && !done) glow(p, 70 + 6 * std::sin(t * 3), fade(col::ember, 0.35f));
        if (r.type == RoomType::Boss) glow(p, 80, fade(col::blood, 0.3f));
        DrawCircleV({p.x + 3, p.y + 4}, rad, fade(BLACK, 0.5f));
        DrawCircleV(p, rad, Color{26, 20, 22, 255});
        DrawRing(p, rad - 3, rad, 0, 360, 40, hov ? col::emberHot : fade(rc, done ? 0.35f : 0.9f));
        art::drawRoomIcon(r.type, p, rad * 0.7f, done ? fade(rc, 0.3f) : rc);
        if (v->id == run.checkpoint()) diamond({p.x, p.y - rad - 8}, 5, col::ember, true);
        if (v->id == run.droppedAshRoom()) {
            float pulse = 0.6f + 0.4f * std::sin(t * 5);
            glow({p.x + rad * 0.7f, p.y - rad * 0.7f}, 26, fade(col::blood, pulse));
            DrawCircleV({p.x + rad * 0.7f, p.y - rad * 0.7f}, 6, col::blood);
        }
        text(body(), std::to_string(v->id), p.x + rad - 4, p.y + rad - 10, 13, col::inkFaint);
    });

    // ---- party (lentera)
    float fl = 1.0f + 0.08f * std::sin(t * 11) * std::sin(t * 5.3f);
    glow(partyPos_, 46 * fl, fade(col::ember, 0.7f));
    DrawCircleV(partyPos_, 9, col::emberHot);
    DrawCircleV(partyPos_, 5, WHITE);
}

void MapView::drawHud(Run& run) {
    // judul lantai
    DrawRectangleGradientV(0, 0, VW, 70, fade(BLACK, 0.8f), fade(BLACK, 0));
    textShadow(head(), "LANTAI " + std::to_string(run.floor().number()) + "  ·  " + floorName(run.floor().number()),
               VW / 2.0f, 14, 26, col::gold, Center);
    text(body(), "Klik ruang untuk berjalan  ·  S: shrine terdekat  ·  Tab: Inspector struktur data",
         VW / 2.0f, 46, 15, col::inkFaint, Center);

    // panel bawah
    Rectangle p = {0, 600, static_cast<float>(VW), 120};
    DrawRectangleGradientV(0, 570, VW, 150, fade(BLACK, 0), fade(BLACK, 0.85f));
    for (int i = 0; i < PARTY_SIZE; ++i) {
        const Unit& u = run.hero(i);
        float x = 24 + i * 230.0f;
        art::drawPortrait(static_cast<HeroKind>(u.kind), {x, p.y + 14, 62, 86}, u.alive ? 1.0f : 0.4f, false);
        text(head(), u.name, x + 72, p.y + 18, 20, col::ink);
        bar({x + 72, p.y + 48, 130, 10}, static_cast<float>(u.hp) / u.maxHp, col::hp, Color{20, 26, 16, 230});
        text(body(), std::to_string(u.hp) + " / " + std::to_string(u.maxHp), x + 72, p.y + 62, 15, col::inkDim);
    }

    // lentera
    float lx = 730;
    text(head(), "LENTERA", lx, p.y + 16, 16, col::gold);
    text(body(), std::string(lightName(run.lightLevel())) + "  " + std::to_string(run.light()), lx + 230, p.y + 16, 16,
         run.lightLevel() == LightLevel::Dark ? col::blood : col::emberHot, Right);
    bar({lx, p.y + 40, 230, 12}, lightVis_ / MAX_LIGHT, col::ember, Color{30, 20, 12, 230});
    // ambang terang/redup
    DrawLine(static_cast<int>(lx + 230 * 0.6f), static_cast<int>(p.y + 36), static_cast<int>(lx + 230 * 0.6f), static_cast<int>(p.y + 56), col::inkDim);
    DrawLine(static_cast<int>(lx + 230 * 0.25f), static_cast<int>(p.y + 36), static_cast<int>(lx + 230 * 0.25f), static_cast<int>(p.y + 56), col::inkDim);
    text(body(), "Radius pandang (BFS): " + std::to_string(revealRadius(run.lightLevel())), lx, p.y + 62, 14, col::inkFaint);

    float sx = 1010;
    text(body(), "Ash", sx, p.y + 16, 16, col::inkDim);
    text(head(), std::to_string(run.ash()), sx + 240, p.y + 12, 22, col::gold, Right);
    text(body(), "Hourglass", sx, p.y + 44, 16, col::inkDim);
    text(head(), std::to_string(run.hourglass()), sx + 240, p.y + 40, 22, col::ink, Right);
    text(body(), "Kematian", sx, p.y + 72, 16, col::inkDim);
    text(head(), std::to_string(run.deaths()), sx + 240, p.y + 68, 22, col::blood, Right);
}

void MapView::drawTooltip(Run& run) {
    if (hovered_ < 0) return;
    Floor& f = run.floor();
    const Room* r = f.room(hovered_);
    bool adj = f.isAdjacent(f.currentRoom(), hovered_);
    std::string t1 = r->revealed ? roomTypeName(r->type) : "Ruang tak dikenal";
    std::string t2;
    if (hovered_ == f.currentRoom()) t2 = "Posisi kamu";
    else if (adj) t2 = "Klik: berjalan (cahaya -" + std::to_string(run.lightCost(f.currentRoom(), hovered_)) + ")";
    else if (pathCost_ >= 0) t2 = "Klik: auto-travel " + std::to_string(pathLen_ - 1) + " langkah (cahaya -" +
                                  std::to_string(pathCost_ * LIGHT_PER_WEIGHT) + ")";
    else t2 = "Tidak ada jalur yang diketahui";
    if (r->revealed && r->cleared && r->type != RoomType::Entrance) t1 += " (selesai)";

    Vector2 m = mouse();
    float w = std::fmax(textWidth(head(), t1, 18), textWidth(body(), t2, 15)) + 24;
    Rectangle box = {m.x + 18, m.y + 10, w, 54};
    if (box.x + box.width > VW) box.x = m.x - w - 18;
    panel(box);
    text(head(), t1, box.x + 12, box.y + 8, 18, r->revealed ? roomColor(r->type) : col::ink);
    text(body(), t2, box.x + 12, box.y + 31, 15, col::inkDim);
}

void MapView::drawToasts() {
    if (toastT_ <= 0) return;
    float a = std::fmin(1.0f, toastT_);
    float y = 80;
    for (size_t i = 0; i < toasts_.size(); ++i) {
        float ai = a * (0.4f + 0.6f * (i + 1) / toasts_.size());
        text(body(), toasts_[i], 24, y + i * 20.0f, 16, fade(col::ink, ai));
    }
}

// ============================================================================
// Modal ruangan
// ============================================================================

namespace {
Rectangle modal(float h) {
    DrawRectangle(0, 0, VW, VH, fade(BLACK, 0.55f));
    Rectangle r = {VW / 2.0f - 320, VH / 2.0f - h / 2 - 30, 640, h};
    panel(r);
    return r;
}
}  // namespace

void MapView::drawShrine(Run& run) {
    Rectangle r = modal(300);
    glow({r.x + r.width / 2, r.y + 70}, 120, fade(col::ember, 0.4f));
    textShadow(title(), "Ember Shrine", r.x + r.width / 2, r.y + 24, 44, col::emberHot, Center);
    textWrap(body(), "Bara suci masih menyala. Checkpoint tersimpan di sini: jika party tumbang, kalian akan bangkit di shrine ini.",
             r.x + 40, r.y + 96, r.width - 80, 18, col::inkDim);
    bool canRest = !run.floor().room(run.floor().currentRoom())->cleared;
    if (button({r.x + 60, r.y + 210, 250, 50}, canRest ? "Beristirahat" : "Bara telah padam", canRest)) run.rest();
    if (button({r.x + 330, r.y + 210, 250, 50}, "Lanjutkan")) run.leaveRoom();
    if (canRest) text(body(), "Pulih penuh, bangkitkan yang tumbang, +40 cahaya", r.x + 185, r.y + 268, 14, col::inkFaint, Center);
}

void MapView::drawShop(Run& run) {
    Rectangle r = modal(360);
    textShadow(title(), "Pedagang Bertudung", r.x + r.width / 2, r.y + 20, 40, col::gold, Center);
    text(body(), "Ash kamu: " + std::to_string(run.ash()), r.x + r.width / 2, r.y + 72, 18, col::inkDim, Center);
    for (int i = 0; i < static_cast<int>(ShopItem::COUNT); ++i) {
        ShopItem it = static_cast<ShopItem>(i);
        float y = r.y + 104 + i * 64.0f;
        bool afford = run.ash() >= run.price(it);
        if (button({r.x + 40, y, r.width - 80, 54}, "", afford)) run.buy(it);
        text(head(), shopItemName(it), r.x + 60, y + 7, 20, afford ? col::ink : col::inkFaint);
        text(body(), shopItemDesc(it), r.x + 60, y + 31, 15, col::inkDim);
        text(head(), std::to_string(run.price(it)) + " Ash", r.x + r.width - 60, y + 16, 20, afford ? col::gold : col::inkFaint, Right);
    }
    if (button({r.x + r.width / 2 - 90, r.y + r.height - 50, 180, 38}, "Pergi") || IsKeyPressed(KEY_ESCAPE)) run.leaveRoom();
}

void MapView::drawEvent(Run& run) {
    const EventDef& e = run.currentEvent();
    Rectangle r = modal(330);
    textShadow(title(), e.title, r.x + r.width / 2, r.y + 22, 40, Color{170, 190, 240, 255}, Center);
    textWrap(body(), e.text, r.x + 40, r.y + 88, r.width - 80, 19, col::ink);
    for (int i = 0; i < 2; ++i)
        if (button({r.x + 40, r.y + 180 + i * 62.0f, r.width - 80, 50}, e.options[i], true, false, 18)) run.chooseEventOption(i);
}

void MapView::drawRelic(Run& run) {
    Rectangle r = modal(380);
    textShadow(title(), "Pilih Relic", r.x + r.width / 2, r.y + 20, 44, col::gold, Center);
    text(body(), "Boss tumbang. Ambil satu peninggalannya sebelum turun lebih dalam.", r.x + r.width / 2, r.y + 76, 17,
         col::inkDim, Center);
    int n = run.relicOptionCount();
    float cw = (r.width - 80 - (n - 1) * 16.0f) / n;
    for (int i = 0; i < n; ++i) {
        Rectangle c = {r.x + 40 + i * (cw + 16), r.y + 114, cw, 220};
        bool h = hover(c);
        if (button(c, "", true, h)) { run.chooseRelic(i); return; }
        glow({c.x + c.width / 2, c.y + 60}, 50, fade(col::gold, h ? 0.5f : 0.25f));
        diamond({c.x + c.width / 2, c.y + 60}, 18, col::gold, true);
        text(head(), relicName(run.relicOption(i)), c.x + c.width / 2, c.y + 100, 20, h ? col::emberHot : col::ink, Center);
        textWrap(body(), relicDesc(run.relicOption(i)), c.x + 14, c.y + 134, c.width - 28, 15, col::inkDim);
    }
}
