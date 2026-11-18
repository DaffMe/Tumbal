// ============================================================================
// Cinderwake — entry point.
//
// Fase 0: hanya membuka jendela raylib untuk memastikan toolchain bekerja.
//   cinderwake.exe            → jendela game
//   cinderwake.exe --smoke    → buka jendela ±1 detik lalu keluar (cek build)
//   cinderwake.exe --console  → mode console (diisi di Fase 1)
// ============================================================================

#include <cstdio>
#include <cstring>
#include <cmath>
#include "raylib.h"

static const int kScreenW = 1280;
static const int kScreenH = 720;

int main(int argc, char** argv) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--console") == 0) {
            std::printf("Mode console belum tersedia (Fase 1).\n");
            return 0;
        }
        if (std::strcmp(argv[i], "--smoke") == 0) smoke = true;
    }

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenW, kScreenH, "Cinderwake");
    SetTargetFPS(60);

    int frames = 0;
    while (!WindowShouldClose()) {
        if (smoke && ++frames > 60) break;
        float t = static_cast<float>(GetTime());

        BeginDrawing();
        ClearBackground(Color{10, 8, 12, 255});

        // Bara lentera yang berdenyut — placeholder sampai Fase 3
        float flicker = 0.85f + 0.15f * std::sin(t * 7.0f) * std::sin(t * 3.1f);
        Vector2 c = {kScreenW / 2.0f, kScreenH / 2.0f - 40.0f};
        DrawCircleGradient(c, 220.0f * flicker,
                           Color{255, 140, 50, 60}, Color{10, 8, 12, 0});
        DrawCircleV(c, 10.0f * flicker, Color{255, 190, 110, 255});

        const char* title = "CINDERWAKE";
        int tw = MeasureText(title, 64);
        DrawText(title, (kScreenW - tw) / 2, kScreenH / 2 + 60, 64, Color{214, 196, 160, 255});
        const char* sub = "Fase 0 - fondasi siap";
        int sw = MeasureText(sub, 20);
        DrawText(sub, (kScreenW - sw) / 2, kScreenH / 2 + 135, 20, Color{120, 110, 96, 255});
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
