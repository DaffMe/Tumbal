#include "Ui.hpp"

#include <cmath>
#include "Assets.hpp"

namespace ui {

// Jersey 10 digambar pada kelipatan 10 px agar piksel tetap tajam.
static Font g_fonts[4]{};
static const int SIZES[4] = {10, 20, 30, 40};
static bool g_ready = false;

static int pick(float size) {
    if (size <= 12) return 0;
    if (size <= 22) return 1;
    if (size <= 34) return 2;
    return 3;
}

void init() {
    std::string p = Assets::get().path("assets/fonts/Jersey10-Regular.ttf");
    for (int i = 0; i < 4; ++i) {
        g_fonts[i] = LoadFontEx(p.c_str(), SIZES[i], nullptr, 0);
        if (g_fonts[i].texture.id == 0) g_fonts[i] = GetFontDefault();
        SetTextureFilter(g_fonts[i].texture, TEXTURE_FILTER_POINT);
    }
    g_ready = true;
}

void shutdown() {
    if (!g_ready) return;
    for (auto& f : g_fonts)
        if (f.texture.id != GetFontDefault().texture.id) UnloadFont(f);
    g_ready = false;
}

Font font() { return g_ready ? g_fonts[1] : GetFontDefault(); }

static const Font& fontFor(float size) {
    static Font def = GetFontDefault();
    return g_ready ? g_fonts[pick(size)] : def;
}

float measure(const std::string& s, float size) {
    int k = pick(size);
    return MeasureTextEx(fontFor(size), s.c_str(), static_cast<float>(SIZES[k]), 0).x;
}

void text(const std::string& s, float x, float y, Color c, float size, bool shadow) {
    int k = pick(size);
    float sz = static_cast<float>(SIZES[k]);
    const Font& f = fontFor(size);
    Vector2 p{std::floor(x), std::floor(y) - (k == 1 ? 3.0f : 0.0f)};
    if (shadow) DrawTextEx(f, s.c_str(), {p.x + 1, p.y + 1}, sz, 0, Color{0, 0, 0, static_cast<unsigned char>(c.a * 0.85f)});
    DrawTextEx(f, s.c_str(), p, sz, 0, c);
}

void textCentered(const std::string& s, float cx, float y, Color c, float size) {
    text(s, cx - measure(s, size) / 2.0f, y, c, size);
}

void textRight(const std::string& s, float rx, float y, Color c, float size) {
    text(s, rx - measure(s, size), y, c, size);
}

std::vector<std::string> wrap(const std::string& s, float width, float size) {
    std::vector<std::string> lines;
    std::string line, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string trial = line.empty() ? word : line + " " + word;
        if (!line.empty() && measure(trial, size) > width) {
            lines.push_back(line);
            line = word;
        } else {
            line = trial;
        }
        word.clear();
    };
    for (char c : s) {
        if (c == ' ') flushWord();
        else if (c == '\n') { flushWord(); lines.push_back(line); line.clear(); }
        else word += c;
    }
    flushWord();
    if (!line.empty()) lines.push_back(line);
    return lines;
}

void window(Rectangle r, float alpha) {
    DrawRectangleRec(r, Fade(Color{14, 10, 16, 255}, alpha));
    DrawRectangleGradientV(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width), 10,
                           Fade(Color{60, 46, 40, 255}, 0.35f * alpha), Fade(BLACK, 0.0f));
    DrawRectangleLinesEx(r, 1, Fade(Color{120, 98, 76, 255}, alpha));
    DrawRectangleLinesEx({r.x + 2, r.y + 2, r.width - 4, r.height - 4}, 1, Fade(Color{50, 40, 34, 255}, alpha));
}

void bar(Rectangle r, float fill, Color a, Color b, Color back) {
    if (fill < 0) fill = 0;
    if (fill > 1) fill = 1;
    DrawRectangleRec({r.x - 1, r.y - 1, r.width + 2, r.height + 2}, Color{0, 0, 0, 200});
    DrawRectangleRec(r, back);
    if (fill > 0) DrawRectangleGradientH(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width * fill),
                                         static_cast<int>(r.height), a, b);
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width * fill), 1, Fade(WHITE, 0.25f));
}

void cursor(float x, float y, double time) {
    float off = static_cast<float>(std::sin(time * 8.0) * 1.5);
    x += off;
    DrawTriangle({x, y - 5}, {x, y + 5}, {x + 6, y}, AMBER);
}

}  // namespace ui
