#include "Gfx.hpp"
#include "rlgl.h"

#include <cmath>
#include <unordered_map>
#include <vector>

namespace gfx {

namespace {

RenderTexture2D g_rt;
Shader          g_post;
bool            g_hasPost = false;
int             g_locTime, g_locRes, g_locLight, g_locRadius, g_locDark, g_locTint, g_locFlash, g_locHud;
Font            g_body, g_head, g_title;
float           g_scale = 1.0f;
Vector2         g_offset = {0, 0};
float           g_shake = 0.0f;
Vector2         g_shakeOff = {0, 0};
std::string     g_assetRoot;

struct Particle {
    Vector2 p, v;
    float   life, maxLife, size;
    Color   c;
};
std::vector<Particle> g_particles;
float g_emberAccum = 0.0f;

std::unordered_map<std::string, Texture2D> g_textures;
std::unordered_map<std::string, bool>      g_missing;

// Post-process: grading, vignette, grain, dan cahaya lentera.
const char* kPostFs = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform float time;
uniform vec2  resolution;
uniform vec2  lightPos;     // dalam UV (y sudah dibalik)
uniform float lightRadius;  // piksel virtual
uniform float darkness;
uniform vec3  tint;
uniform float flash;
uniform vec3  hud;          // x: batas atas HUD, y: batas bawah HUD, z: x panel kanan (piksel virtual)
out vec4 finalColor;

float rand(vec2 co) { return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453); }

void main() {
    vec2 uv = fragTexCoord;
    vec3 c = texture(texture0, uv).rgb;

    if (darkness > 0.0) {
        float flick = 1.0 + 0.035 * sin(time * 9.0) * sin(time * 4.3);
        float d = distance(uv * resolution, lightPos * resolution);
        float l = smoothstep(lightRadius * flick, lightRadius * 0.25, d);
        // area HUD (atas/bawah) dan panel kanan tidak ikut digelapkan
        float py = (1.0 - uv.y) * resolution.y;
        float px = uv.x * resolution.x;
        float world = smoothstep(hud.x - 10.0, hud.x + 30.0, py) * (1.0 - smoothstep(hud.y - 30.0, hud.y + 10.0, py));
        world *= 1.0 - step(hud.z, px);
        l = mix(1.0, l, world);
        c *= mix(1.0 - darkness, 1.0, l);
        c += vec3(1.0, 0.55, 0.22) * 0.07 * l * l;
    }

    c = mix(c, c * tint, 0.45);
    float lum = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(lum), c, 0.88);
    c = pow(c, vec3(1.06));

    vec2 q = uv - 0.5;
    float vig = 1.0 - dot(q, q) * 1.45;
    c *= clamp(vig, 0.0, 1.0);

    c += (rand(uv * resolution + fract(time) * 100.0) - 0.5) * 0.05;
    c += flash * vec3(1.0, 0.92, 0.82);
    finalColor = vec4(c, 1.0);
}
)";

Font loadFontFirst(std::initializer_list<const char*> paths, int size) {
    // ASCII + beberapa simbol yang dipakai UI
    static int cps[128];
    static int n = 0;
    if (n == 0) {
        for (int c = 32; c < 127; ++c) cps[n++] = c;
        const int extra[] = {0x2192, 0x21C4, 0x25C6, 0x25C7, 0x2726, 0x2020, 0x2014, 0x2022, 0x00B7};
        for (int e : extra) cps[n++] = e;
    }
    for (const char* p : paths) {
        if (FileExists(p)) {
            Font f = LoadFontEx(p, size, cps, n);
            if (f.texture.id != 0) {
                GenTextureMipmaps(&f.texture);
                SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
                return f;
            }
        }
    }
    return GetFontDefault();
}

void findAssetRoot() {
    const char* candidates[] = {"assets", "../assets", "../../assets"};
    for (const char* c : candidates)
        if (DirectoryExists(c)) { g_assetRoot = std::string(c) + "/"; return; }
    g_assetRoot = "assets/";
}

}  // namespace

// ============================================================================

void init() {
    g_rt = LoadRenderTexture(VW, VH);
    SetTextureFilter(g_rt.texture, TEXTURE_FILTER_BILINEAR);

    g_post = LoadShaderFromMemory(nullptr, kPostFs);
    g_hasPost = g_post.id != 0 && g_post.id != rlGetShaderIdDefault();
    g_locTime   = GetShaderLocation(g_post, "time");
    g_locRes    = GetShaderLocation(g_post, "resolution");
    g_locLight  = GetShaderLocation(g_post, "lightPos");
    g_locRadius = GetShaderLocation(g_post, "lightRadius");
    g_locDark   = GetShaderLocation(g_post, "darkness");
    g_locTint   = GetShaderLocation(g_post, "tint");
    g_locFlash  = GetShaderLocation(g_post, "flash");
    g_locHud    = GetShaderLocation(g_post, "hud");

    findAssetRoot();
    std::string f = g_assetRoot + "fonts/";
    g_body  = loadFontFirst({(f + "body.ttf").c_str(), "C:/Windows/Fonts/pala.ttf", "C:/Windows/Fonts/georgia.ttf"}, 48);
    g_head  = loadFontFirst({(f + "head.ttf").c_str(), "C:/Windows/Fonts/palab.ttf", "C:/Windows/Fonts/georgiab.ttf"}, 64);
    g_title = loadFontFirst({(f + "title.ttf").c_str(), "C:/Windows/Fonts/BOOKOSB.TTF", "C:/Windows/Fonts/palab.ttf"}, 128);
}

void shutdown() {
    for (auto& kv : g_textures) UnloadTexture(kv.second);
    g_textures.clear();
    if (g_body.texture.id != GetFontDefault().texture.id) UnloadFont(g_body);
    if (g_head.texture.id != GetFontDefault().texture.id) UnloadFont(g_head);
    if (g_title.texture.id != GetFontDefault().texture.id) UnloadFont(g_title);
    if (g_hasPost) UnloadShader(g_post);
    UnloadRenderTexture(g_rt);
}

void beginFrame() {
    float sw = static_cast<float>(GetScreenWidth());
    float sh = static_cast<float>(GetScreenHeight());
    g_scale = std::fmin(sw / VW, sh / VH);
    g_offset = {(sw - VW * g_scale) * 0.5f, (sh - VH * g_scale) * 0.5f};

    BeginTextureMode(g_rt);
    ClearBackground(col::bg);
}

void endFrame(const Post& post) {
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    if (g_hasPost) {
        float t = static_cast<float>(GetTime());
        float res[2] = {static_cast<float>(VW), static_cast<float>(VH)};
        float lp[2] = {post.lightPos.x / VW, 1.0f - post.lightPos.y / VH};
        float tint[3] = {post.tint.r / 255.0f, post.tint.g / 255.0f, post.tint.b / 255.0f};
        SetShaderValue(g_post, g_locTime, &t, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_post, g_locRes, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(g_post, g_locLight, lp, SHADER_UNIFORM_VEC2);
        SetShaderValue(g_post, g_locRadius, &post.lightRadius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_post, g_locDark, &post.darkness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_post, g_locTint, tint, SHADER_UNIFORM_VEC3);
        SetShaderValue(g_post, g_locFlash, &post.flash, SHADER_UNIFORM_FLOAT);
        float hud[3] = {post.hudTop, post.hudBottom, post.panelX};
        SetShaderValue(g_post, g_locHud, hud, SHADER_UNIFORM_VEC3);
        BeginShaderMode(g_post);
    }
    Rectangle src = {0, 0, static_cast<float>(VW), -static_cast<float>(VH)};
    Rectangle dst = {g_offset.x + g_shakeOff.x * g_scale, g_offset.y + g_shakeOff.y * g_scale, VW * g_scale, VH * g_scale};
    DrawTexturePro(g_rt.texture, src, dst, {0, 0}, 0.0f, WHITE);
    if (g_hasPost) EndShaderMode();
    EndDrawing();
}

float time() { return static_cast<float>(GetTime()); }

// ============================================================================
// Input
// ============================================================================

Vector2 mouse() {
    Vector2 m = GetMousePosition();
    return {(m.x - g_offset.x) / g_scale, (m.y - g_offset.y) / g_scale};
}
bool clicked() { return IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }
bool rightClicked() { return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT); }
bool hover(Rectangle r) { return CheckCollisionPointRec(mouse(), r); }

// ============================================================================
// Teks
// ============================================================================

Font& body() { return g_body; }
Font& head() { return g_head; }
Font& title() { return g_title; }

float textWidth(const Font& f, const std::string& s, float size) {
    return MeasureTextEx(f, s.c_str(), size, size * 0.02f).x;
}

void text(const Font& f, const std::string& s, float x, float y, float size, Color c, Align a) {
    float w = (a == Left) ? 0.0f : textWidth(f, s, size);
    if (a == Center) x -= w * 0.5f;
    if (a == Right) x -= w;
    DrawTextEx(f, s.c_str(), {std::round(x), std::round(y)}, size, size * 0.02f, c);
}

void textShadow(const Font& f, const std::string& s, float x, float y, float size, Color c, Align a) {
    text(f, s, x + 2, y + 2, size, fade(BLACK, c.a / 255.0f * 0.8f), a);
    text(f, s, x, y, size, c, a);
}

float textWrap(const Font& f, const std::string& s, float x, float y, float width, float size, Color c) {
    std::string line, word;
    float yy = y;
    auto flush = [&]() {
        if (!line.empty()) text(f, line, x, yy, size, c);
        yy += size * 1.2f;
        line.clear();
    };
    for (size_t i = 0; i <= s.size(); ++i) {
        char ch = i < s.size() ? s[i] : ' ';
        if (ch == ' ' || ch == '\n') {
            std::string cand = line.empty() ? word : line + " " + word;
            if (!word.empty() && textWidth(f, cand, size) > width) { flush(); line = word; }
            else line = cand;
            word.clear();
            if (ch == '\n') flush();
        } else {
            word += ch;
        }
    }
    if (!line.empty()) flush();
    return yy - y;
}

// ============================================================================
// Widget
// ============================================================================

Color fade(Color c, float a) {
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    c.a = static_cast<unsigned char>(c.a * a);
    return c;
}

Color lerpColor(Color a, Color b, float t) {
    auto l = [t](unsigned char x, unsigned char y) { return static_cast<unsigned char>(x + (y - x) * t); };
    return {l(a.r, b.r), l(a.g, b.g), l(a.b, b.b), l(a.a, b.a)};
}

void diamond(Vector2 p, float r, Color c, bool filled) {
    Vector2 t = {p.x, p.y - r}, rt = {p.x + r, p.y}, b = {p.x, p.y + r}, l = {p.x - r, p.y};
    if (filled) {
        DrawTriangle(t, l, b, c);
        DrawTriangle(t, b, rt, c);
    } else {
        DrawLineEx(t, rt, 1.5f, c);
        DrawLineEx(rt, b, 1.5f, c);
        DrawLineEx(b, l, 1.5f, c);
        DrawLineEx(l, t, 1.5f, c);
    }
}

void panel(Rectangle r, float alpha) {
    DrawRectangleRec(r, fade(col::panel, alpha));
    DrawRectangleGradientV(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width), 24,
                           fade(Color{60, 46, 40, 255}, 0.35f * alpha), fade(col::panel, 0.0f));
    DrawRectangleLinesEx(r, 1.5f, fade(col::edge, alpha));
    Rectangle in = {r.x + 4, r.y + 4, r.width - 8, r.height - 8};
    DrawRectangleLinesEx(in, 1.0f, fade(Color{60, 48, 34, 255}, alpha));
    float d = 5.0f;
    diamond({r.x, r.y}, d, fade(col::gold, alpha), true);
    diamond({r.x + r.width, r.y}, d, fade(col::gold, alpha), true);
    diamond({r.x, r.y + r.height}, d, fade(col::gold, alpha), true);
    diamond({r.x + r.width, r.y + r.height}, d, fade(col::gold, alpha), true);
}

bool button(Rectangle r, const std::string& label, bool enabled, bool selected, float fontSize) {
    bool h = enabled && hover(r);
    Color fill = selected ? Color{58, 40, 30, 245} : (h ? col::panelHi : col::panel);
    DrawRectangleRec(r, fill);
    if (h || selected) {
        DrawRectangleGradientV(static_cast<int>(r.x), static_cast<int>(r.y + r.height * 0.5f), static_cast<int>(r.width),
                               static_cast<int>(r.height * 0.5f), fade(col::ember, 0.0f), fade(col::ember, 0.16f));
    }
    Color edge = selected ? col::gold : (h ? col::emberHot : col::edge);
    DrawRectangleLinesEx(r, selected ? 2.0f : 1.5f, enabled ? edge : col::inkFaint);
    Color tc = enabled ? (h || selected ? col::emberHot : col::ink) : col::inkFaint;
    text(head(), label, r.x + r.width / 2, r.y + (r.height - fontSize) / 2 - 1, fontSize, tc, Center);
    return h && clicked();
}

void bar(Rectangle r, float frac, Color fill, Color back, float ghost) {
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    DrawRectangleRec(r, back);
    if (ghost > frac) DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width * std::fmin(ghost, 1.0f)),
                                     static_cast<int>(r.height), Color{230, 210, 170, 160});
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width * frac), static_cast<int>(r.height), fill);
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width * frac),
                  static_cast<int>(r.height * 0.35f), fade(WHITE, 0.12f));
    DrawRectangleLinesEx(r, 1.0f, Color{0, 0, 0, 200});
}

void glow(Vector2 p, float radius, Color c) {
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(p, radius, c, fade(c, 0.0f));
    EndBlendMode();
}

// ============================================================================
// Efek
// ============================================================================

void shake(float amount) { g_shake = std::fmax(g_shake, amount); }
Vector2 shakeOffset() { return g_shakeOff; }

void burst(Vector2 p, Color c, int count, float speed) {
    for (int i = 0; i < count; ++i) {
        float a = GetRandomValue(0, 6283) / 1000.0f;
        float s = speed * (0.3f + GetRandomValue(0, 1000) / 1000.0f);
        float life = 0.35f + GetRandomValue(0, 400) / 1000.0f;
        g_particles.push_back({p, {std::cos(a) * s, std::sin(a) * s - 40.0f}, life, life,
                               1.5f + GetRandomValue(0, 25) / 10.0f, c});
    }
}

void ambientEmbers(float rate) {
    g_emberAccum += rate * GetFrameTime();
    while (g_emberAccum >= 1.0f) {
        g_emberAccum -= 1.0f;
        float life = 3.0f + GetRandomValue(0, 3000) / 1000.0f;
        Vector2 p = {static_cast<float>(GetRandomValue(0, VW)), static_cast<float>(VH + 10)};
        Vector2 v = {static_cast<float>(GetRandomValue(-15, 15)), -static_cast<float>(GetRandomValue(30, 80))};
        Color c = GetRandomValue(0, 3) == 0 ? col::emberHot : col::ember;
        g_particles.push_back({p, v, life, life, 1.0f + GetRandomValue(0, 15) / 10.0f, c});
    }
}

void updateEffects(float dt) {
    for (auto& p : g_particles) {
        p.life -= dt;
        p.p.x += p.v.x * dt + std::sin(p.life * 3.0f + p.p.y * 0.01f) * 12.0f * dt;
        p.p.y += p.v.y * dt;
        p.v.x *= 0.99f;
    }
    for (size_t i = 0; i < g_particles.size();) {
        if (g_particles[i].life <= 0) {
            g_particles[i] = g_particles.back();
            g_particles.pop_back();
        } else {
            ++i;
        }
    }
    if (g_shake > 0.01f) {
        g_shakeOff = {static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * g_shake,
                      static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * g_shake};
        g_shake *= std::pow(0.0008f, dt);
    } else {
        g_shake = 0;
        g_shakeOff = {0, 0};
    }
}

void drawParticles() {
    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& p : g_particles) {
        float a = p.life / p.maxLife;
        DrawCircleV(p.p, p.size * 2.4f, fade(p.c, a * 0.18f));
        DrawCircleV(p.p, p.size, fade(p.c, a));
    }
    EndBlendMode();
}

// ============================================================================
// Tekstur & tween
// ============================================================================

Texture2D* tex(const std::string& path) {
    auto it = g_textures.find(path);
    if (it != g_textures.end()) return &it->second;
    if (g_missing.count(path)) return nullptr;
    std::string full = g_assetRoot + path;
    if (!FileExists(full.c_str())) {
        g_missing[path] = true;
        return nullptr;
    }
    Texture2D t = LoadTexture(full.c_str());
    if (t.id == 0) {
        g_missing[path] = true;
        return nullptr;
    }
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    g_textures[path] = t;
    return &g_textures[path];
}

float approach(float cur, float target, float speed, float dt) {
    return cur + (target - cur) * (1.0f - std::exp(-speed * dt));
}

float easeOut(float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

}  // namespace gfx
