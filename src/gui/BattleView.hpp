#pragma once
// ============================================================================
// BattleView — tampilan pertarungan.
// Logika battle sepenuhnya di core (Battle). View ini:
//   1. mengirim aksi pemain ke Battle (playerUseSkill, interject, rewindTurn)
//   2. men-dequeue Battle Event Queue satu per satu dan menganimasikannya
// ============================================================================

#include <deque>
#include <string>
#include <vector>
#include "raylib.h"
#include "core/battle/Battle.hpp"
#include "DsLog.hpp"

class BattleView {
public:
    void begin(Battle* battle, int floorNo, const std::string& title, DsLog* dsLog);
    void update(float dt);
    void draw();

    bool    done() const { return done_; }
    Battle* battle() const { return battle_; }
    bool    busy() const { return !pending_.empty() || eventTimer_ > 0.0f; }
    void    refresh() { pullEvents(); }   // ambil event baru setelah aksi dari luar view

private:
    struct Vis {
        float hp = 0, ghost = 0;
        float flash = 0, lunge = 0;
        float alpha = 0;
        bool  present = false, dying = false;
        float x = 0;
        float tokenY = -1;
    };
    struct Floating {
        Vector2     pos;
        std::string text;
        Color       color;
        float       size;
        float       t;
    };

    Battle*                 battle_ = nullptr;
    DsLog*                  dsLog_ = nullptr;
    int                     floor_ = 1;
    std::string             title_;
    Vis                     vis_[MAX_UNITS];
    std::deque<BattleEvent> pending_;
    float                   eventTimer_ = 0;
    std::vector<Floating>   floats_;
    std::deque<std::string> log_;
    std::string             banner_;
    float                   bannerT_ = 0;
    int                     selectedSlot_ = 0;
    int                     lastTurn_ = -1;
    int                     hoverTarget_ = -1;
    float                   endT_ = 0;
    bool                    done_ = false;
    float                   flash_ = 0;

    void  pullEvents();
    float process(const BattleEvent& e);
    void  syncAll();
    void  handleInput();
    void  tryUse(int slot, int target);

    Vector2   enemyFoot(int idx) const;
    Rectangle enemyRect(int idx) const;
    Rectangle heroCard(int idx) const;
    Vector2   unitCenter(int idx) const;
    void      layoutEnemies(float dt);

    void drawTimeline();
    void drawEnemies();
    void drawHeroes();
    void drawSkillPanel();
    void drawTopBar();
    void drawFloats();
    void drawEnd();
};
