#pragma once
// ============================================================================
// Gfx — utilitas tampilan raylib: resolusi virtual, font, widget sederhana,
// partikel, screen shake, tekstur, dan post-process shader.
// Lapisan ini HANYA tampilan; tidak ada logika mekanik di sini.
// ============================================================================

#include <string>
#include "raylib.h"

namespace gfx {

constexpr int VW = 1280;   // resolusi virtual
constexpr int VH = 720;

namespace col {
const Color bg       {12, 10, 14, 255};
const Color ink      {222, 206, 170, 255};
const Color inkDim   {140, 126, 104, 255};
const Color inkFaint {86, 78, 66, 255};
const Color gold     {206, 166, 92, 255};
const Color ember    {255, 140, 50, 255};
const Color emberHot {255, 200, 120, 255};
const Color blood    {178, 44, 40, 255};
const Color hp       {120, 168, 92, 255};
const Color hero     {130, 178, 206, 255};
const Color enemy    {206, 96, 84, 255};
const Color arcane   {150, 120, 220, 255};
const Color panel    {20, 16, 22, 236};
const Color panelHi  {34, 27, 34, 245};
const Color edge     {104, 84, 56, 255};
}  // namespace col

enum Align { Left, Center, Right };

struct Post {
    float   darkness    = 0.0f;   // 0 = tanpa efek lentera
    Vector2 lightPos    = {VW / 2.0f, VH / 2.0f};
    float   lightRadius = 400.0f;
    Color   tint        = {255, 236, 210, 255};
    float   flash       = 0.0f;
    float   hudTop      = 0.0f;               // di atas garis ini tidak digelapkan
    float   hudBottom   = static_cast<float>(VH);  // di bawah garis ini tidak digelapkan
    float   panelX      = static_cast<float>(VW);  // di kanan garis ini tidak digelapkan
};

void init();
void shutdown();

// ---- frame
void beginFrame();                  // mulai menggambar ke render target virtual
void endFrame(const Post& post);    // terapkan shader + letterbox ke layar
float time();

// ---- input dalam koordinat virtual
Vector2 mouse();
bool    clicked();
bool    rightClicked();
bool    hover(Rectangle r);

// ---- teks
Font& body();
Font& head();
Font& title();
void  text(const Font& f, const std::string& s, float x, float y, float size, Color c, Align a = Left);
void  textShadow(const Font& f, const std::string& s, float x, float y, float size, Color c, Align a = Left);
float textWidth(const Font& f, const std::string& s, float size);
// teks dibungkus dalam lebar maksimum; kembalikan tinggi total
float textWrap(const Font& f, const std::string& s, float x, float y, float width, float size, Color c);

// ---- widget
void panel(Rectangle r, float alpha = 1.0f);
bool button(Rectangle r, const std::string& label, bool enabled = true, bool selected = false, float fontSize = 20.0f);
void bar(Rectangle r, float frac, Color fill, Color back, float ghost = -1.0f);
void glow(Vector2 p, float radius, Color c);
void diamond(Vector2 p, float r, Color c, bool filled);

// ---- efek
void  shake(float amount);
Vector2 shakeOffset();
void  burst(Vector2 p, Color c, int count, float speed = 160.0f);
void  ambientEmbers(float rate);
void  updateEffects(float dt);
void  drawParticles();

// ---- tekstur (nullptr jika file tidak ada → pakai placeholder)
Texture2D* tex(const std::string& path);

// ---- tween
float approach(float cur, float target, float speed, float dt);
float easeOut(float t);
Color fade(Color c, float a);
Color lerpColor(Color a, Color b, float t);

}  // namespace gfx
