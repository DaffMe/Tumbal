#include "Ui.hpp"

#include "core/util/Lang.hpp"

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

// Ambang alpha atlas font: TTF dirasterisasi dengan anti-alias (tepi abu-abu) yang terlihat buram
// setelah layar diperbesar; atlas dijadikan biner agar setiap piksel huruf tegas.
static void crispen(Font& f) {
    Image img = LoadImageFromTexture(f.texture);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = static_cast<Color*>(img.data);
    for (int i = 0; i < img.width * img.height; ++i) {
        unsigned char a = px[i].a >= 110 ? 255 : 0;
        px[i] = Color{255, 255, 255, a};
    }
    UnloadTexture(f.texture);
    f.texture = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
}

void init() {
    std::string p = Assets::get().path("assets/fonts/Jersey10-Regular.ttf");
    // ukuran 10 px: Jersey 10 terlalu kecil untuk dirasterisasi (huruf jadi gumpalan), jadi teks
    // kecil memakai font bitmap bawaan raylib (piksel 1-bit, tajam, mudah dibaca)
    g_fonts[0] = GetFontDefault();
    for (int i = 1; i < 4; ++i) {
        g_fonts[i] = LoadFontEx(p.c_str(), SIZES[i], nullptr, 0);
        if (g_fonts[i].texture.id == 0) { g_fonts[i] = GetFontDefault(); continue; }
        crispen(g_fonts[i]);
    }
    g_ready = true;
}

void shutdown() {
    if (!g_ready) return;
    for (int i = 1; i < 4; ++i)
        if (g_fonts[i].texture.id != GetFontDefault().texture.id) UnloadFont(g_fonts[i]);
    g_ready = false;
}

Font font() { return g_ready ? g_fonts[1] : GetFontDefault(); }

static const Font& fontFor(float size) {
    static Font def = GetFontDefault();
    return g_ready ? g_fonts[pick(size)] : def;
}

// Semua teks layar lewat fungsi-fungsi ini, jadi terjemahan dilakukan di sini (tr: kunci = teks
// Indonesia; teks lain, mis. nama pemain atau angka, dikembalikan apa adanya).
static float rawMeasure(const std::string& s, float size) {
    int k = pick(size);
    return MeasureTextEx(fontFor(size), s.c_str(), static_cast<float>(SIZES[k]), k == 0 ? 1.0f : 0.0f).x;
}

static void rawText(const std::string& s, float x, float y, Color c, float size, bool shadow) {
    int k = pick(size);
    float sz = static_cast<float>(SIZES[k]);
    const Font& f = fontFor(size);
    Vector2 p{std::floor(x), std::floor(y) - (k == 1 ? 3.0f : 0.0f)};
    float spacing = k == 0 ? 1.0f : 0.0f;          // font bitmap kecil butuh jarak 1 px antarhuruf
    if (shadow) DrawTextEx(f, s.c_str(), {p.x + 1, p.y + 1}, sz, spacing, Color{0, 0, 0, static_cast<unsigned char>(c.a * 0.85f)});
    DrawTextEx(f, s.c_str(), p, sz, spacing, c);
}

float measure(const std::string& s, float size) { return rawMeasure(tr(s), size); }

void text(const std::string& s, float x, float y, Color c, float size, bool shadow) { rawText(tr(s), x, y, c, size, shadow); }

void textCentered(const std::string& s, float cx, float y, Color c, float size) {
    std::string t = tr(s);
    rawText(t, cx - rawMeasure(t, size) / 2.0f, y, c, size, true);
}

void textRight(const std::string& s, float rx, float y, Color c, float size) {
    std::string t = tr(s);
    rawText(t, rx - rawMeasure(t, size), y, c, size, true);
}

std::vector<std::string> wrap(const std::string& src, float width, float size) {
    std::string s = tr(src);
    std::vector<std::string> lines;
    std::string line, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string trial = line.empty() ? word : line + " " + word;
        if (!line.empty() && rawMeasure(trial, size) > width) {
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
