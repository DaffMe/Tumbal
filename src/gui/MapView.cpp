#include "MapView.hpp"

#include <cmath>
#include <utility>
#include "rlgl.h"
#include "Assets.hpp"

static constexpr int TS = 32;

void MapView::init(int viewW, int viewH) {
    viewW_ = viewW;
    viewH_ = viewH;
    lightmap_ = LoadRenderTexture(viewW, viewH);
}

void MapView::shutdown() { UnloadRenderTexture(lightmap_); }

Vector2 MapView::targetFor(const Field& field) const {
    const Actor& p = field.player();
    const TileMap* m = field.map();
    float tx = p.drawX() * TS + TS / 2.0f - viewW_ / 2.0f;
    float ty = p.drawY() * TS + TS / 2.0f - viewH_ / 2.0f;
    float mw = static_cast<float>(m->width() * TS), mh = static_cast<float>(m->height() * TS);
    // peta lebih kecil dari layar → di tengah
    tx = mw <= viewW_ ? (mw - viewW_) / 2.0f : std::fmax(0.0f, std::fmin(tx, mw - viewW_));
    ty = mh <= viewH_ ? (mh - viewH_) / 2.0f : std::fmax(0.0f, std::fmin(ty, mh - viewH_));
    return {tx, ty};
}

void MapView::snapCamera(const Field& field) { cam_ = targetFor(field); }

void MapView::updateCamera(const Field& field, float dt) {
    Vector2 t = targetFor(field);
    float k = 1.0f - std::exp(-dt * 12.0f);
    cam_.x += (t.x - cam_.x) * k;
    cam_.y += (t.y - cam_.y) * k;
}

void MapView::drawLayers(const TileMap& map, bool above, long long ms) {
    int x0 = static_cast<int>(cam_.x) / TS - 1, y0 = static_cast<int>(cam_.y) / TS - 1;
    int x1 = x0 + viewW_ / TS + 3, y1 = y0 + viewH_ / TS + 3;
    for (const TileLayer* l = map.layers(); l != nullptr; l = l->next) {
        if (!l->visible || l->above != above) continue;
        Color tint = Fade(WHITE, l->opacity);
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                int gid = l->at(x, y);
                if (gid == 0) continue;
                const Tileset* ts = map.tilesetFor(gid);
                if (ts == nullptr) continue;
                int local = ts->animatedTile(gid - ts->firstGid, ms);
                Texture2D tex = Assets::get().texture(ts->image);
                int cols = ts->columns > 0 ? ts->columns : 1;
                Rectangle src{static_cast<float>((local % cols) * ts->tileW), static_cast<float>((local / cols) * ts->tileH),
                              static_cast<float>(ts->tileW), static_cast<float>(ts->tileH)};
                Vector2 pos{std::floor(x * TS - cam_.x), std::floor(y * TS - cam_.y)};
                DrawTextureRec(tex, src, pos, tint);
            }
        }
    }
}

void MapView::drawActor(const Actor& a, const char* sheet, float alpha) {
    Texture2D tex = Assets::get().texture(sheet);
    if (tex.id == 0) return;
    const int FRAME_W = tex.width / 4, FRAME_H = tex.height / 4;   // 4 frame x 4 arah
    float px = std::floor(a.drawX() * TS - cam_.x);
    float py = std::floor(a.drawY() * TS - cam_.y);
    // bayangan elips di kaki
    DrawEllipse(static_cast<int>(px + TS / 2), static_cast<int>(py + TS - 5), 9, 4, Fade(BLACK, 0.35f * alpha));
    Rectangle src{static_cast<float>(a.walkFrame() * FRAME_W), static_cast<float>(a.facing * FRAME_H),
                  static_cast<float>(FRAME_W), static_cast<float>(FRAME_H)};
    Vector2 pos{px + TS / 2.0f - FRAME_W / 2.0f, py + TS - FRAME_H + 2};
    DrawTextureRec(tex, src, pos, Fade(WHITE, alpha));
}

void MapView::drawObjects(const Field& field, double time, const Bloodstain* blood) {
    const TileMap& map = *field.map();
    // ceceran sukma: nyala hijau pucat berdenyut di lokasi kematian
    if (blood != nullptr && blood->active && blood->map == field.mapId()) {
        float cx = blood->x * TS + TS / 2.0f - cam_.x, cy = blood->y * TS + TS / 2.0f - cam_.y + 6;
        float p = 0.5f + 0.5f * static_cast<float>(std::sin(time * 3.0));
        DrawEllipse(static_cast<int>(cx), static_cast<int>(cy + 4), 10, 4, Fade(Color{60, 200, 140, 255}, 0.35f));
        DrawCircleV({cx, cy - 2}, 4 + p * 2, Fade(Color{120, 255, 190, 255}, 0.35f));
        DrawCircleV({cx, cy - 2}, 2.5f, Color{220, 255, 235, 255});
        for (int k = 0; k < 3; ++k) {
            float t = static_cast<float>(std::fmod(time * 0.8 + k * 0.33, 1.0));
            DrawCircleV({cx + std::sin(k * 2.1f + t * 6) * 4, cy - 2 - t * 18}, 1.2f, Fade(Color{160, 255, 210, 255}, 1 - t));
        }
    }
    for (const MapObject* o = map.objects(); o != nullptr; o = o->next) {
        float cx = o->x + TS / 2.0f - cam_.x;
        float cy = o->y + TS / 2.0f - cam_.y;
        if (o->type == "loot" && field.world().isCollected(field.mapId(), map.typeIndex(o))) continue;
        if (o->type == "loot") {
            // kilau barang: titik cahaya berdenyut seperti BS1
            float t = static_cast<float>(time * 3.0 + o->id);
            float s = 1.5f + std::sin(t) * 0.8f;
            DrawCircleV({cx, cy + 6}, s + 2.5f, Fade(Color{150, 200, 255, 255}, 0.25f));
            DrawCircleV({cx, cy + 6}, s, Color{230, 240, 255, 230});
            DrawLineEx({cx - 5, cy + 6}, {cx + 5, cy + 6}, 1, Fade(WHITE, 0.5f + 0.4f * std::sin(t * 1.7f)));
            DrawLineEx({cx, cy + 1}, {cx, cy + 11}, 1, Fade(WHITE, 0.5f + 0.4f * std::sin(t * 1.3f)));
        } else if (o->type == "tungku") {
            // Tungku: tungku batu + bara (placeholder prosedural)
            DrawRectangle(static_cast<int>(cx - 12), static_cast<int>(cy - 2), 24, 14, Color{58, 50, 48, 255});
            DrawRectangle(static_cast<int>(cx - 14), static_cast<int>(cy - 5), 28, 5, Color{84, 74, 70, 255});
            DrawRectangle(static_cast<int>(cx - 9), static_cast<int>(cy + 1), 18, 6, Color{30, 20, 18, 255});
            for (int i = 0; i < 7; ++i) {
                float ph = static_cast<float>(time * (2.0 + i * 0.37) + i * 1.9);
                float fx = cx - 8 + i * 2.7f + std::sin(ph) * 1.5f;
                float fh = 6.0f + 5.0f * (0.5f + 0.5f * std::sin(ph * 1.3f));
                DrawTriangle({fx - 2.5f, cy - 4}, {fx + 2.5f, cy - 4}, {fx, cy - 4 - fh}, Color{255, 140, 40, 220});
                DrawTriangle({fx - 1.2f, cy - 4}, {fx + 1.2f, cy - 4}, {fx, cy - 4 - fh * 0.6f}, Color{255, 220, 120, 230});
            }
        }
    }
}

void MapView::prepareLighting(const Field& field, double time, const Bloodstain* blood) {
    const TileMap* m = field.map();
    float dark = std::stof(m->props().getString("gelap", "0.0"));
    lightReady_ = dark > 0.01f;
    if (!lightReady_) return;
    Texture2D light = Assets::get().lightTexture();

    BeginTextureMode(lightmap_);
    ClearBackground(Color{6, 4, 12, static_cast<unsigned char>(dark * 255)});
    // hapus kegelapan: dst = dst * (1 - srcAlpha)
    rlSetBlendFactors(RL_ZERO, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    auto punch = [&](float x, float y, float radius, float strength) {
        Rectangle src{0, 0, static_cast<float>(light.width), static_cast<float>(light.height)};
        Rectangle dst{x - radius, y - radius, radius * 2, radius * 2};
        DrawTexturePro(light, src, dst, {0, 0}, 0, Fade(WHITE, strength));
    };
    const Actor& p = field.player();
    float px = p.drawX() * TS + TS / 2.0f - cam_.x, py = p.drawY() * TS + TS / 2.0f - cam_.y;
    punch(px, py - 8, 120, 0.95f);
    if (blood != nullptr && blood->active && blood->map == field.mapId())
        punch(blood->x * TS + TS / 2.0f - cam_.x, blood->y * TS + TS / 2.0f - cam_.y, 40, 0.8f);
    for (const MapObject* o = m->objects(); o != nullptr; o = o->next) {
        float cx = o->x + TS / 2.0f - cam_.x, cy = o->y + TS / 2.0f - cam_.y;
        if (o->type == "tungku") {
            float fl = 1.0f + 0.06f * std::sin(static_cast<float>(time * 9.0)) + 0.04f * std::sin(static_cast<float>(time * 23.0));
            punch(cx, cy, 190 * fl, 1.0f);
        } else if (o->type == "loot" && !field.world().isCollected(field.mapId(), m->typeIndex(o))) {
            punch(cx, cy + 6, 26, 0.6f);
        }
    }
    EndBlendMode();
    EndTextureMode();
}

void MapView::drawLighting(const Field& field, double time) {
    if (!lightReady_) return;
    const TileMap* m = field.map();
    Texture2D light = Assets::get().lightTexture();
    // cahaya hangat aditif di sekitar Tungku
    BeginBlendMode(BLEND_ADDITIVE);
    for (const MapObject* o = m->objects(); o != nullptr; o = o->next) {
        if (o->type != "tungku") continue;
        float cx = o->x + TS / 2.0f - cam_.x, cy = o->y + TS / 2.0f - cam_.y;
        float r = 120 + 6 * std::sin(static_cast<float>(time * 7.0));
        DrawTexturePro(light, {0, 0, static_cast<float>(light.width), static_cast<float>(light.height)},
                       {cx - r, cy - r, r * 2, r * 2}, {0, 0}, 0, Color{255, 120, 40, 70});
    }
    EndBlendMode();

    DrawTextureRec(lightmap_.texture, {0, 0, static_cast<float>(viewW_), -static_cast<float>(viewH_)}, {0, 0}, WHITE);
}

void MapView::draw(const Field& field, double time, bool showCollision, const Bloodstain* blood) {
    const TileMap* m = field.map();
    if (m == nullptr) return;
    long long ms = static_cast<long long>(time * 1000.0);
    drawLayers(*m, false, ms);
    drawObjects(field, time, blood);
    // karakter diurut berdasarkan y agar yang di depan menutupi yang di belakang
    int order[Field::MAX_ENEMIES + 1 + 16];
    int n = 0;
    for (int i = 0; i < field.enemyCount(); ++i)
        if (field.enemy(i).alive) order[n++] = i;
    order[n++] = -1;   // pemain
    // NPC (objek "npc" dengan sprite) ikut diurut; disimpan sebagai -100 - urutan
    const MapObject* npcs[16];
    int npcCount = 0;
    for (const MapObject* o = m->objects(); o != nullptr && npcCount < 16; o = o->next)
        if (o->type == "npc" && !o->props.getString("walker").empty()) {
            npcs[npcCount] = o;
            order[n++] = -100 - npcCount;
            ++npcCount;
        }
    auto yOf = [&](int k) {
        if (k <= -100) return npcs[-100 - k]->y / TS;
        return k < 0 ? field.player().drawY() : field.enemy(k).actor.drawY();
    };
    for (int i = 1; i < n; ++i)
        for (int j = i; j > 0 && yOf(order[j - 1]) > yOf(order[j]); --j) std::swap(order[j], order[j - 1]);
    for (int i = 0; i < n; ++i) {
        if (order[i] <= -100) {
            const MapObject* o = npcs[-100 - order[i]];
            Actor a;
            a.x = static_cast<int>(o->x) / TS;
            a.y = static_cast<int>(o->y) / TS;
            a.facing = o->props.getInt("dir", 0);
            drawActor(a, o->props.getString("walker").c_str());
            continue;
        }
        if (order[i] < 0) { drawActor(field.player(), "assets/chars/tumbal.png"); continue; }
        const FieldEnemy& e = field.enemy(order[i]);
        drawActor(e.actor, e.walker.c_str(), e.cooldown > 0 ? 0.55f : 1.0f);
    }
    // tanda "!" saat musuh menyadari pemain
    for (int i = 0; i < field.enemyCount(); ++i) {
        const FieldEnemy& e = field.enemy(i);
        if (!e.alive || e.alertFlash <= 0) continue;
        float x = e.actor.drawX() * TS + TS / 2.0f - cam_.x, y = e.actor.drawY() * TS - cam_.y - 46;
        float pop = e.alertFlash > 0.6f ? (0.8f - e.alertFlash) / 0.2f : 1.0f;
        DrawRectangle(static_cast<int>(x - 6), static_cast<int>(y - 8 * pop), 12, static_cast<int>(16 * pop), Color{240, 230, 210, 255});
        DrawRectangleLines(static_cast<int>(x - 6), static_cast<int>(y - 8 * pop), 12, static_cast<int>(16 * pop), Color{60, 20, 20, 255});
        DrawRectangle(static_cast<int>(x - 1), static_cast<int>(y - 5 * pop), 3, static_cast<int>(7 * pop), Color{170, 30, 30, 255});
        DrawRectangle(static_cast<int>(x - 1), static_cast<int>(y + 4 * pop), 3, 2, Color{170, 30, 30, 255});
    }
    drawLayers(*m, true, ms);
    drawLighting(field, time);
    if (showCollision) {
        int x0 = static_cast<int>(cam_.x) / TS, y0 = static_cast<int>(cam_.y) / TS;
        for (int y = y0; y <= y0 + viewH_ / TS + 1; ++y)
            for (int x = x0; x <= x0 + viewW_ / TS + 1; ++x)
                if (m->blocked(x, y))
                    DrawRectangle(static_cast<int>(x * TS - cam_.x), static_cast<int>(y * TS - cam_.y), TS, TS, Fade(RED, 0.25f));
    }
}
