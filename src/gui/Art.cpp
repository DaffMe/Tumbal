#include "Art.hpp"
#include "Gfx.hpp"

#include <cmath>
#include <string>

using namespace gfx;

namespace art {

std::string shortName(const std::string& name, size_t maxLen) {
    std::string s = name;
    const char* prefixes[] = {"Hollow ", "The ", "Candle "};
    for (const char* p : prefixes) {
        std::string ps(p);
        if (s.compare(0, ps.size(), ps) == 0) s = s.substr(ps.size());
    }
    if (s.size() > maxLen) s = s.substr(0, maxLen - 1) + ".";
    return s;
}

const char* enemySlug(EnemyKind k) {
    switch (k) {
        case EnemyKind::HollowThrall: return "hollow_thrall";
        case EnemyKind::BellRinger:   return "bell_ringer";
        case EnemyKind::CandleWraith: return "candle_wraith";
        case EnemyKind::Wisp:         return "wisp";
        case EnemyKind::Gargoyle:     return "gargoyle";
        case EnemyKind::ChoirWarden:  return "choir_warden";
        case EnemyKind::SisterOfAsh:  return "sister_of_ash";
        case EnemyKind::LastFlame:    return "last_flame";
        default:                      return "unknown";
    }
}

const char* heroSlug(HeroKind k) {
    switch (k) {
        case HeroKind::Varga: return "varga";
        case HeroKind::Sable: return "sable";
        case HeroKind::Ione:  return "ione";
        default:              return "unknown";
    }
}

Vector2 enemySize(EnemyKind k) {
    switch (k) {
        case EnemyKind::Wisp:        return {120, 150};
        case EnemyKind::Gargoyle:    return {280, 280};
        case EnemyKind::ChoirWarden: return {380, 460};
        case EnemyKind::SisterOfAsh: return {340, 450};
        case EnemyKind::LastFlame:   return {420, 470};
        case EnemyKind::BellRinger:  return {200, 300};
        default:                     return {200, 290};
    }
}

// ---------------------------------------------------------------------------
// Placeholder musuh prosedural
// ---------------------------------------------------------------------------

namespace {

// raylib hanya menggambar segitiga berurutan counter-clockwise; helper ini
// menukar urutan titik bila perlu sehingga semua segitiga pasti tampil.
void tri(Vector2 a, Vector2 b, Vector2 c, Color col) {
    float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross > 0) DrawTriangle(a, c, b, col);
    else DrawTriangle(a, b, c, col);
}

Color shadeOf(Color base, float a, float flash) {
    Color c = lerpColor(base, Color{255, 245, 230, 255}, flash);
    return fade(c, a);
}

void eyes(Vector2 a, Vector2 b, float r, Color c, float alpha) {
    glow(a, r * 5, fade(c, 0.5f * alpha));
    glow(b, r * 5, fade(c, 0.5f * alpha));
    DrawCircleV(a, r, fade(c, alpha));
    DrawCircleV(b, r, fade(c, alpha));
}

void drawPlaceholder(EnemyKind k, Vector2 f, float a, float fl, float t) {
    float breathe = std::sin(t * 2.0f) * 0.02f;
    Color body = {34, 28, 34, 255};
    Color rim  = {92, 70, 60, 255};
    // bayangan kaki
    DrawEllipse(static_cast<int>(f.x), static_cast<int>(f.y), enemySize(k).x * 0.42f, 14, fade(BLACK, 0.55f * a));

    switch (k) {
        case EnemyKind::HollowThrall: {
            float h = 250 * (1 + breathe);
            Vector2 chest = {f.x, f.y - h * 0.55f};
            DrawEllipse(static_cast<int>(chest.x), static_cast<int>(chest.y), 62, h * 0.38f, shadeOf(rim, a, fl));
            DrawEllipse(static_cast<int>(chest.x), static_cast<int>(chest.y + 4), 58, h * 0.36f, shadeOf(body, a, fl));
            DrawLineEx({f.x - 45, chest.y - 40}, {f.x - 80, f.y - 40}, 16, shadeOf(body, a, fl));
            DrawLineEx({f.x + 45, chest.y - 40}, {f.x + 84, f.y - 60}, 16, shadeOf(body, a, fl));
            DrawLineEx({f.x - 20, chest.y + 50}, {f.x - 28, f.y}, 18, shadeOf(body, a, fl));
            DrawLineEx({f.x + 20, chest.y + 50}, {f.x + 26, f.y}, 18, shadeOf(body, a, fl));
            Vector2 head = {f.x + 8, chest.y - h * 0.36f};
            DrawCircleV(head, 34, shadeOf(rim, a, fl));
            DrawCircleV({head.x, head.y + 3}, 32, shadeOf(body, a, fl));
            eyes({head.x - 11, head.y}, {head.x + 11, head.y + 2}, 4, Color{255, 120, 60, 255}, a);
            break;
        }
        case EnemyKind::BellRinger: {
            float h = 270 * (1 + breathe);
            Vector2 top = {f.x, f.y - h};
            tri(top, {f.x - 85, f.y}, {f.x + 85, f.y}, shadeOf(rim, a, fl));
            tri({top.x, top.y + 8}, {f.x - 78, f.y}, {f.x + 78, f.y}, shadeOf(body, a, fl));
            DrawCircleV({top.x, top.y + 30}, 30, shadeOf(Color{20, 16, 20, 255}, a, fl));
            eyes({top.x - 9, top.y + 30}, {top.x + 9, top.y + 30}, 3, Color{240, 210, 120, 255}, a);
            // lonceng
            float sw = std::sin(t * 3.0f) * 10.0f;
            Vector2 bell = {f.x + 70 + sw, f.y - 120};
            DrawLineEx({f.x + 40, f.y - 170}, bell, 4, shadeOf(rim, a, fl));
            tri({bell.x, bell.y - 30}, {bell.x - 30, bell.y + 20}, {bell.x + 30, bell.y + 20}, shadeOf(Color{150, 118, 60, 255}, a, fl));
            DrawCircleV({bell.x, bell.y + 22}, 7, shadeOf(Color{90, 70, 40, 255}, a, fl));
            break;
        }
        case EnemyKind::CandleWraith: {
            float bob = std::sin(t * 1.6f) * 8.0f;
            float h = 260;
            Vector2 top = {f.x, f.y - h + bob};
            tri(top, {f.x - 70, f.y - 10 + bob}, {f.x + 70, f.y - 10 + bob}, shadeOf(Color{70, 62, 80, 255}, a * 0.9f, fl));
            tri({top.x, top.y + 14}, {f.x - 60, f.y - 14 + bob}, {f.x + 60, f.y - 14 + bob}, shadeOf(Color{40, 34, 48, 255}, a, fl));
            for (int i = -1; i <= 1; ++i) {
                Vector2 c = {top.x + i * 26.0f, top.y + 30 + std::abs(i) * 14.0f};
                DrawRectangle(static_cast<int>(c.x - 5), static_cast<int>(c.y), 10, 26, shadeOf(Color{220, 210, 190, 255}, a, fl));
                float fl2 = 1.0f + 0.2f * std::sin(t * 12 + i);
                glow({c.x, c.y - 8}, 28 * fl2, fade(col::ember, 0.7f * a));
                DrawCircleV({c.x, c.y - 6}, 5 * fl2, fade(col::emberHot, a));
            }
            eyes({top.x - 12, top.y + 80}, {top.x + 12, top.y + 80}, 3.5f, Color{180, 220, 255, 255}, a);
            break;
        }
        case EnemyKind::Wisp: {
            float bob = std::sin(t * 3.0f) * 10.0f;
            Vector2 c = {f.x, f.y - 80 + bob};
            float fl2 = 1.0f + 0.15f * std::sin(t * 14);
            glow(c, 70 * fl2, fade(Color{120, 180, 255, 255}, 0.6f * a));
            DrawCircleV(c, 22 * fl2, shadeOf(Color{170, 210, 255, 255}, a, fl));
            DrawCircleV(c, 12, fade(WHITE, a));
            break;
        }
        case EnemyKind::Gargoyle: {
            float h = 240 * (1 + breathe * 0.5f);
            Vector2 c = {f.x, f.y - h * 0.45f};
            tri({c.x - 40, c.y - 50}, {c.x - 160, c.y - 110}, {c.x - 70, c.y + 40}, shadeOf(Color{58, 56, 60, 255}, a, fl));
            tri({c.x + 40, c.y - 50}, {c.x + 70, c.y + 40}, {c.x + 160, c.y - 110}, shadeOf(Color{58, 56, 60, 255}, a, fl));
            DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y), 90, h * 0.45f, shadeOf(Color{88, 84, 88, 255}, a, fl));
            DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y + 6), 84, h * 0.43f, shadeOf(Color{52, 50, 56, 255}, a, fl));
            Vector2 head = {c.x, c.y - h * 0.42f};
            DrawCircleV(head, 38, shadeOf(Color{70, 68, 72, 255}, a, fl));
            tri({head.x - 30, head.y - 20}, {head.x - 22, head.y - 60}, {head.x - 8, head.y - 30}, shadeOf(Color{70, 68, 72, 255}, a, fl));
            tri({head.x + 30, head.y - 20}, {head.x + 8, head.y - 30}, {head.x + 22, head.y - 60}, shadeOf(Color{70, 68, 72, 255}, a, fl));
            eyes({head.x - 13, head.y}, {head.x + 13, head.y}, 5, Color{120, 255, 170, 255}, a);
            break;
        }
        case EnemyKind::ChoirWarden:
        default: {
            float h = 440 * (1 + breathe * 0.6f);
            Vector2 top = {f.x, f.y - h};
            glow({top.x, top.y + 40}, 150, fade(Color{255, 210, 140, 255}, 0.25f * a));
            DrawRing({top.x, top.y + 40}, 58, 64, 0, 360, 48, fade(Color{230, 190, 110, 255}, 0.8f * a));
            tri(top, {f.x - 150, f.y}, {f.x + 150, f.y}, shadeOf(Color{110, 30, 34, 255}, a, fl));
            tri({top.x, top.y + 16}, {f.x - 138, f.y}, {f.x + 138, f.y}, shadeOf(Color{44, 16, 22, 255}, a, fl));
            DrawRectangle(static_cast<int>(f.x - 8), static_cast<int>(top.y + 110), 16, static_cast<int>(h - 120), shadeOf(Color{150, 118, 60, 255}, a * 0.8f, fl));
            DrawCircleV({top.x, top.y + 40}, 40, shadeOf(Color{20, 12, 16, 255}, a, fl));
            for (int i = 0; i < 3; ++i) {
                float ex = top.x - 16 + i * 16;
                eyes({ex, top.y + 36 + (i == 1 ? -8 : 0)}, {ex, top.y + 36 + (i == 1 ? -8 : 0)}, 3.5f, Color{255, 220, 150, 255}, a);
            }
            break;
        }
    }
}

}  // namespace

void drawEnemy(EnemyKind k, Vector2 foot, float alpha, float flash, float t, bool highlight) {
    Vector2 size = enemySize(k);
    if (highlight) glow({foot.x, foot.y - size.y * 0.45f}, size.x * 0.75f, fade(col::ember, 0.22f * alpha));

    std::string path = std::string("final/enemies/") + enemySlug(k) + ".png";
    if (Texture2D* tx = tex(path)) {
        float breathe = 1.0f + std::sin(t * 2.0f) * 0.012f;
        float h = size.y * breathe;
        float w = h * tx->width / static_cast<float>(tx->height);
        DrawEllipse(static_cast<int>(foot.x), static_cast<int>(foot.y), w * 0.38f, 14, fade(BLACK, 0.55f * alpha));
        Rectangle dst = {foot.x - w / 2, foot.y - h, w, h};
        DrawTexturePro(*tx, {0, 0, static_cast<float>(tx->width), static_cast<float>(tx->height)}, dst, {0, 0}, 0, fade(WHITE, alpha));
        if (flash > 0.01f) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawTexturePro(*tx, {0, 0, static_cast<float>(tx->width), static_cast<float>(tx->height)}, dst, {0, 0}, 0, fade(WHITE, flash * alpha));
            EndBlendMode();
        }
        return;
    }
    drawPlaceholder(k, foot, alpha, flash, t);
}

// ---------------------------------------------------------------------------

void drawPortrait(HeroKind k, Rectangle r, float alpha, bool active) {
    std::string path = std::string("final/heroes/") + heroSlug(k) + ".png";
    if (Texture2D* tx = tex(path)) {
        // crop tengah agar sesuai rasio kotak
        float srcW = static_cast<float>(tx->width), srcH = static_cast<float>(tx->height);
        float want = r.width / r.height;
        Rectangle src = {0, 0, srcW, srcH};
        if (srcW / srcH > want) { src.width = srcH * want; src.x = (srcW - src.width) / 2; }
        else { src.height = srcW / want; src.y = 0; }
        DrawTexturePro(*tx, src, r, {0, 0}, 0, fade(active ? WHITE : Color{170, 160, 150, 255}, alpha));
    } else {
        Color tone = k == HeroKind::Varga ? Color{90, 96, 110, 255} : (k == HeroKind::Sable ? Color{96, 60, 110, 255} : Color{130, 100, 50, 255});
        DrawRectangleGradientV(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width), static_cast<int>(r.height),
                               fade(tone, alpha), fade(Color{14, 12, 16, 255}, alpha));
        Vector2 c = {r.x + r.width / 2, r.y + r.height * 0.62f};
        DrawCircleV({c.x, c.y - r.height * 0.18f}, r.width * 0.2f, fade(Color{18, 14, 18, 255}, alpha));
        DrawEllipse(static_cast<int>(c.x), static_cast<int>(r.y + r.height), r.width * 0.38f, r.height * 0.42f, fade(Color{18, 14, 18, 255}, alpha));
        const char* initial = k == HeroKind::Varga ? "V" : (k == HeroKind::Sable ? "S" : "I");
        text(title(), initial, r.x + 8, r.y + 2, r.height * 0.32f, fade(col::gold, 0.7f * alpha));
    }
    DrawRectangleLinesEx(r, active ? 2.0f : 1.0f, fade(active ? col::gold : col::edge, alpha));
}

// ---------------------------------------------------------------------------

namespace {

void arch(float x, float w, float top, float bottom, Color fill, Color line) {
    float r = w / 2;
    DrawRectangle(static_cast<int>(x), static_cast<int>(top + r), static_cast<int>(w), static_cast<int>(bottom - top - r), fill);
    DrawCircleSector({x + r, top + r}, r, 180, 360, 32, fill);
    DrawLineEx({x, bottom}, {x, top + r}, 2, line);
    DrawLineEx({x + w, bottom}, {x + w, top + r}, 2, line);
    DrawRing({x + r, top + r}, r - 1, r + 1, 180, 360, 32, line);
}

bool backgroundTexture(const std::string& path) {
    if (Texture2D* tx = tex(path)) {
        DrawTexturePro(*tx, {0, 0, static_cast<float>(tx->width), static_cast<float>(tx->height)},
                       {0, 0, static_cast<float>(VW), static_cast<float>(VH)}, {0, 0}, 0, WHITE);
        return true;
    }
    return false;
}

}  // namespace

void drawBattleBackground(int floorNo, float t) {
    if (backgroundTexture("final/bg/battle_floor" + std::to_string(floorNo) + ".png")) return;

    Color top = floorNo == 1 ? Color{30, 22, 30, 255} : (floorNo == 2 ? Color{20, 24, 34, 255} : Color{36, 16, 14, 255});
    DrawRectangleGradientV(0, 0, VW, VH, top, Color{8, 6, 8, 255});
    // jendela kaca patri
    glow({VW / 2.0f, 150}, 260, fade(floorNo == 3 ? Color{255, 90, 50, 255} : Color{120, 90, 200, 255}, 0.25f));
    arch(VW / 2.0f - 70, 140, 40, 300, Color{40, 30, 50, 255}, Color{90, 70, 60, 255});
    // deretan lengkungan
    for (int i = 0; i < 6; ++i) {
        float x = 40.0f + i * 210.0f;
        if (i == 3) continue;
        arch(x, 150, 90 + (i % 2) * 20.0f, 470, Color{18, 14, 20, 255}, Color{56, 44, 40, 255});
    }
    // lantai
    DrawRectangleGradientV(0, 470, VW, 250, Color{26, 20, 22, 255}, Color{6, 5, 6, 255});
    for (int i = -8; i <= 8; ++i)
        DrawLineEx({VW / 2.0f + i * 40.0f, 470}, {VW / 2.0f + i * 190.0f, VH}, 1, Color{40, 32, 30, 255});
    // lilin
    for (int i = 0; i < 6; ++i) {
        if (i == 3) continue;
        float x = 115.0f + i * 210.0f;
        float fl = 1.0f + 0.15f * std::sin(t * 9 + i * 1.7f);
        DrawRectangle(static_cast<int>(x - 3), 440, 6, 30, Color{200, 190, 170, 255});
        glow({x, 434}, 60 * fl, fade(col::ember, 0.35f));
        DrawCircleV({x, 436}, 3.5f * fl, col::emberHot);
    }
}

void drawMapBackground(float t) {
    if (backgroundTexture("final/bg/map.png")) return;
    DrawRectangleGradientV(0, 0, VW, VH, Color{34, 28, 26, 255}, Color{14, 11, 11, 255});
    // tekstur batu sederhana
    for (int y = 0; y < VH; y += 48)
        for (int x = (y / 48) % 2 * 48; x < VW; x += 96)
            DrawRectangleLines(x, y, 96, 48, Color{44, 36, 32, 120});
    (void)t;
}

void drawTitleBackground(float t) {
    if (backgroundTexture("final/bg/title.png")) return;
    drawBattleBackground(1, t);
    DrawRectangle(0, 0, VW, VH, fade(BLACK, 0.45f));
}

void drawRoomIcon(RoomType type, Vector2 c, float r, Color tint) {
    std::string path = "final/icons/room_";
    switch (type) {
        case RoomType::Entrance: path += "entrance"; break;
        case RoomType::Battle:   path += "battle"; break;
        case RoomType::Elite:    path += "elite"; break;
        case RoomType::Shrine:   path += "shrine"; break;
        case RoomType::Event:    path += "event"; break;
        case RoomType::Merchant: path += "merchant"; break;
        case RoomType::Treasure: path += "treasure"; break;
        case RoomType::Boss:     path += "boss"; break;
    }
    path += ".png";
    if (Texture2D* tx = tex(path)) {
        DrawTexturePro(*tx, {0, 0, static_cast<float>(tx->width), static_cast<float>(tx->height)},
                       {c.x - r, c.y - r, r * 2, r * 2}, {0, 0}, 0, tint);
        return;
    }
    // placeholder: bentuk sederhana per jenis
    switch (type) {
        case RoomType::Entrance:
            DrawRectangle(static_cast<int>(c.x - r * 0.4f), static_cast<int>(c.y - r * 0.2f), static_cast<int>(r * 0.8f), static_cast<int>(r * 0.7f), tint);
            DrawCircleSector({c.x, c.y - r * 0.2f}, r * 0.4f, 180, 360, 16, tint);
            break;
        case RoomType::Battle:
        case RoomType::Elite: {
            float s = type == RoomType::Elite ? 1.15f : 0.9f;
            DrawLineEx({c.x - r * 0.5f * s, c.y - r * 0.5f * s}, {c.x + r * 0.5f * s, c.y + r * 0.5f * s}, 4, tint);
            DrawLineEx({c.x + r * 0.5f * s, c.y - r * 0.5f * s}, {c.x - r * 0.5f * s, c.y + r * 0.5f * s}, 4, tint);
            if (type == RoomType::Elite) DrawCircleLinesV(c, r * 0.75f, tint);
            break;
        }
        case RoomType::Shrine:
            tri({c.x, c.y - r * 0.7f}, {c.x - r * 0.35f, c.y + r * 0.2f}, {c.x + r * 0.35f, c.y + r * 0.2f}, tint);
            DrawRectangle(static_cast<int>(c.x - r * 0.45f), static_cast<int>(c.y + r * 0.25f), static_cast<int>(r * 0.9f), static_cast<int>(r * 0.25f), tint);
            break;
        case RoomType::Event:
            text(head(), "?", c.x, c.y - r * 0.6f, r * 1.2f, tint, Center);
            break;
        case RoomType::Merchant:
            DrawCircleLinesV(c, r * 0.5f, tint);
            text(head(), "$", c.x, c.y - r * 0.45f, r * 0.9f, tint, Center);
            break;
        case RoomType::Treasure:
            DrawRectangle(static_cast<int>(c.x - r * 0.5f), static_cast<int>(c.y - r * 0.2f), static_cast<int>(r), static_cast<int>(r * 0.6f), tint);
            DrawRectangleLines(static_cast<int>(c.x - r * 0.5f), static_cast<int>(c.y - r * 0.45f), static_cast<int>(r), static_cast<int>(r * 0.3f), tint);
            break;
        case RoomType::Boss:
            tri({c.x - r * 0.6f, c.y + r * 0.35f}, {c.x - r * 0.3f, c.y - r * 0.5f}, {c.x - r * 0.1f, c.y + r * 0.35f}, tint);
            tri({c.x - r * 0.15f, c.y + r * 0.35f}, {c.x, c.y - r * 0.65f}, {c.x + r * 0.15f, c.y + r * 0.35f}, tint);
            tri({c.x + r * 0.1f, c.y + r * 0.35f}, {c.x + r * 0.3f, c.y - r * 0.5f}, {c.x + r * 0.6f, c.y + r * 0.35f}, tint);
            break;
    }
}

}  // namespace art
