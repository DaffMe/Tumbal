#include "MapView.hpp"

#include <cmath>
#include <algorithm>
#include <utility>
#include <vector>
#include "rlgl.h"
#include "Assets.hpp"

static constexpr int TS = 32;

void MapView::init(int viewW, int viewH) {
    viewW_ = viewW;
    viewH_ = viewH;
    lightmap_ = LoadRenderTexture(viewW, viewH);
    // kabut: noise Perlin lembut dijadikan alpha putih keabuan; ukuran 256 diulang (wrap)
    Image noise = GenImagePerlinNoise(256, 256, 0, 0, 3.0f);
    Image fog = GenImageColor(256, 256, BLANK);
    Color* src = LoadImageColors(noise);
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x) {
            // bobot tepi agar sambungan ulang tidak terlihat: campur dengan versi tercermin
            Color a = src[y * 256 + x], b = src[(255 - y) * 256 + (255 - x)];
            float v = (a.r + b.r) / 510.0f;
            v = v < 0.38f ? 0.0f : (v - 0.38f) / 0.62f;
            ImageDrawPixel(&fog, x, y, Color{150, 160, 165, static_cast<unsigned char>(v * 255)});
        }
    UnloadImageColors(src);
    fog_ = LoadTextureFromImage(fog);
    SetTextureWrap(fog_, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(fog_, TEXTURE_FILTER_BILINEAR);
    UnloadImage(noise);
    UnloadImage(fog);
}

void MapView::shutdown() {
    UnloadRenderTexture(lightmap_);
    UnloadTexture(fog_);
}

void MapView::drawFog(const Field& field, double time) {
    float k = std::stof(field.map()->props().getString("kabut", "0"));
    if (k <= 0.01f || fog_.id == 0) return;
    // dua lapis kabut bergeser pelan berlawanan arah, ikut kamera (parallax ringan)
    float t = static_cast<float>(time);
    for (int layer = 0; layer < 2; ++layer) {
        float scale = layer == 0 ? 2.0f : 3.2f;
        float sx = cam_.x * (layer == 0 ? 1.0f : 1.15f) + t * (layer == 0 ? 9.0f : -6.0f);
        float sy = cam_.y * (layer == 0 ? 1.0f : 1.15f) + t * (layer == 0 ? 2.0f : 3.5f);
        Rectangle src{sx / scale, sy / scale, viewW_ / scale, viewH_ / scale};
        DrawTexturePro(fog_, src, {0, 0, static_cast<float>(viewW_), static_cast<float>(viewH_)}, {0, 0}, 0,
                       Fade(WHITE, k * (layer == 0 ? 0.42f : 0.28f)));
    }
}

Vector2 MapView::targetFor(const Field& field) const {
    const Actor& p = field.player();
    const TileMap* m = field.map();
    // posisi kamera selalu bilangan bulat dan dihitung dari floor posisi pemain, sehingga
    // pemain diam tepat di tengah layar dan tile, karakter, props bergeser bersama tanpa
    // getaran 1 piksel (penyebab gerak terasa "goyang")
    float tx = std::floor(p.drawX() * TS) + TS / 2 - viewW_ / 2;
    float ty = std::floor(p.drawY() * TS) + TS / 2 - viewH_ / 2;
    float mw = static_cast<float>(m->width() * TS), mh = static_cast<float>(m->height() * TS);
    // peta lebih kecil dari layar → di tengah
    tx = mw <= viewW_ ? std::floor((mw - viewW_) / 2.0f) : std::fmax(0.0f, std::fmin(tx, mw - viewW_));
    ty = mh <= viewH_ ? std::floor((mh - viewH_) / 2.0f) : std::fmax(0.0f, std::fmin(ty, mh - viewH_));
    return {tx, ty};
}

void MapView::snapCamera(const Field& field) { cam_ = targetFor(field); }

void MapView::updateCamera(const Field& field, float dt) {
    // kamera terkunci ke pemain seperti RPG Maker (tanpa pelambatan yang membuat pemain
    // tertinggal lalu "ditarik" di setiap langkah tile)
    (void)dt;
    cam_ = targetFor(field);
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
        } else if (o->type == "prop" && o->props.getInt("glow") > 0 && !hidden(o)) {
            // nyala samar (mis. mata Memedi) agar siluet terlihat di gelap
            float r = static_cast<float>(o->props.getInt("glow"));
            float fl = 1.0f + 0.08f * std::sin(static_cast<float>(time * 2.3));
            punch(o->x + o->w / 2.0f - cam_.x, o->y + o->h * 0.35f - cam_.y, r * fl, 0.75f);
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
    // karakter, NPC, dan properti besar (rumah, pohon) diurut menurut garis
    // kaki (y bawah) agar yang di depan menutupi yang di belakang
    struct Drawable {
        float y;                  // garis kaki dalam satuan tile
        int   kind;               // 0 pemain, 1 musuh, 2 npc, 3 prop
        int   index;
        const MapObject* obj;
    };
    std::vector<Drawable> list;
    list.push_back({field.player().drawY() + 1.0f, 0, 0, nullptr});
    for (int i = 0; i < field.enemyCount(); ++i)
        if (field.enemy(i).alive) list.push_back({field.enemy(i).actor.drawY() + 1.0f, 1, i, nullptr});
    for (const MapObject* o = m->objects(); o != nullptr; o = o->next) {
        if (hidden(o)) continue;
        if (o->type == "npc" && !o->props.getString("walker").empty())
            list.push_back({o->y / TS + 1.0f, 2, 0, o});
        else if (o->type == "prop")   // prop "flat" (dermaga, papan) selalu di bawah karakter
            list.push_back({o->props.getBool("flat") ? -1000.0f : (o->y + o->h) / TS - 0.01f, 3, 0, o});
    }
    std::stable_sort(list.begin(), list.end(), [](const Drawable& l, const Drawable& r) { return l.y < r.y; });
    for (const Drawable& d : list) {
        if (d.kind == 0) {
            drawActor(field.player(), "assets/chars/tumbal.png");
        } else if (d.kind == 1) {
            const FieldEnemy& e = field.enemy(d.index);
            drawActor(e.actor, e.walker.c_str(), e.cooldown > 0 ? 0.55f : 1.0f);
        } else if (d.kind == 2) {
            Actor a;
            a.x = static_cast<int>(d.obj->x) / TS;
            a.y = static_cast<int>(d.obj->y) / TS;
            a.facing = d.obj->props.getInt("dir", 0);
            drawActor(a, d.obj->props.getString("walker").c_str());
        } else {
            float px = d.obj->x - cam_.x, py = d.obj->y - cam_.y;
            if (px > viewW_ || py > viewH_ || px + d.obj->w < 0 || py + d.obj->h < 0) continue;
            Texture2D t = Assets::get().texture(d.obj->props.getString("image"));
            DrawTexture(t, static_cast<int>(std::floor(px)), static_cast<int>(std::floor(py)), WHITE);
        }
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
    drawFog(field, time);       // setelah kegelapan: kabut pucat tetap terlihat seperti diterangi bulan
    // kilau barang digambar SETELAH kegelapan agar tetap terlihat (seperti kilau item BS1)
    for (const MapObject* o = m->objects(); o != nullptr; o = o->next) {
        if (o->type != "loot" || field.world().isCollected(field.mapId(), m->typeIndex(o))) continue;
        float cx = o->x + TS / 2.0f - cam_.x, cy = o->y + TS / 2.0f + 6 - cam_.y;
        if (cx < -16 || cy < -16 || cx > viewW_ + 16 || cy > viewH_ + 16) continue;
        float ph = static_cast<float>(std::fmod(time * 0.9 + o->id * 0.37, 1.0));
        float tw = ph < 0.35f ? std::sin(ph / 0.35f * 3.14159f) : 0.0f;      // kedip singkat berkala
        float r = 3.0f + tw * 6.0f;
        Color c = Fade(Color{235, 245, 255, 255}, 0.55f + 0.45f * tw);
        DrawLineEx({cx - r, cy}, {cx + r, cy}, 1.0f, c);
        DrawLineEx({cx, cy - r * 1.3f}, {cx, cy + r * 1.3f}, 1.0f, c);
        DrawCircleV({cx, cy}, 1.6f + tw, Color{255, 255, 255, 240});
    }
    if (showCollision) {
        int x0 = static_cast<int>(cam_.x) / TS, y0 = static_cast<int>(cam_.y) / TS;
        for (int y = y0; y <= y0 + viewH_ / TS + 1; ++y)
            for (int x = x0; x <= x0 + viewW_ / TS + 1; ++x)
                if (m->blocked(x, y))
                    DrawRectangle(static_cast<int>(x * TS - cam_.x), static_cast<int>(y * TS - cam_.y), TS, TS, Fade(RED, 0.25f));
    }
}
