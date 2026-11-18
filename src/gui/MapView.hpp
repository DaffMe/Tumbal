#pragma once
// ============================================================================
// MapView — tampilan peta dungeon (Graph) dan modal ruangan
// (shrine, toko, peristiwa, relic). Semua aksi diteruskan ke core::Run.
// ============================================================================

#include <deque>
#include <string>
#include "raylib.h"
#include "core/run/Run.hpp"
#include "DsLog.hpp"
#include "Gfx.hpp"

class MapView {
public:
    void reset();
    void update(Run& run, float dt, DsLog& dsLog);
    void draw(Run& run);
    gfx::Post post(const Run& run) const;   // parameter cahaya lentera untuk shader

    static Vector2 roomPos(int col, int row);

private:
    int                     hovered_ = -1;
    int                     pathFor_ = -1;      // target path yang sedang di-cache
    int                     pathIds_[FLOOR_COLS * FLOOR_ROWS];
    int                     pathLen_ = 0;
    int                     pathCost_ = -1;
    Vector2                 partyPos_ = {0, 0};
    float                   lightVis_ = 100;
    std::deque<std::string> toasts_;
    float                   toastT_ = 0;

    void computePath(Run& run, int target);
    void drawGraph(Run& run);
    void drawHud(Run& run);
    void drawTooltip(Run& run);
    void drawToasts();
    void drawShrine(Run& run);
    void drawShop(Run& run);
    void drawEvent(Run& run);
    void drawRelic(Run& run);
};
