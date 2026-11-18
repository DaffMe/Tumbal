#include "Gallery.hpp"

#include <string>
#include "Art.hpp"
#include "Gfx.hpp"

using namespace gfx;

int runGallery() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(VW, VH, "Cinderwake - gallery");
    SetTargetFPS(60);
    gfx::init();

    const int pages = 4;
    int frame = 0;
    while (!WindowShouldClose()) {
        ++frame;
        int page = frame / 40;
        if (page >= pages) break;
        float t = gfx::time();

        beginFrame();
        Post post;
        if (page < 3) {
            int floorNo = page + 1;
            art::drawBattleBackground(floorNo, t);
            EnemyKind sets[3][3] = {
                {EnemyKind::HollowThrall, EnemyKind::ChoirWarden, EnemyKind::BellRinger},
                {EnemyKind::CandleWraith, EnemyKind::SisterOfAsh, EnemyKind::Wisp},
                {EnemyKind::Gargoyle, EnemyKind::LastFlame, EnemyKind::HollowThrall},
            };
            for (int i = 0; i < 3; ++i) {
                EnemyKind k = sets[page][i];
                float x = 300.0f + i * 340.0f;
                art::drawEnemy(k, {x, 455}, 1.0f, 0.0f, t, false);
                text(body(), art::enemySlug(k), x, 480, 16, col::ink, Center);
            }
        } else {
            art::drawMapBackground(t);
            for (int i = 0; i < 3; ++i)
                art::drawPortrait(static_cast<HeroKind>(i), {80.0f + i * 220, 80, 180, 240}, 1.0f, i == 0);
            const RoomType types[] = {RoomType::Entrance, RoomType::Battle, RoomType::Elite, RoomType::Shrine,
                                      RoomType::Event, RoomType::Merchant, RoomType::Treasure, RoomType::Boss};
            for (int i = 0; i < 8; ++i) {
                Vector2 c = {760.0f + (i % 4) * 120, 160.0f + (i / 4) * 140};
                DrawCircleV(c, 42, Color{26, 20, 22, 255});
                art::drawRoomIcon(types[i], c, 30, col::gold);
            }
        }
        endFrame(post);
        if (frame % 40 == 39) TakeScreenshot(("gallery_" + std::to_string(page + 1) + ".png").c_str());
    }
    gfx::shutdown();
    CloseWindow();
    return 0;
}
