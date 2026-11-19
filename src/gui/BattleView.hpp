#pragma once
// ============================================================================
// BattleView — layar battle front-view (lapisan GUI untuk Mekanik M1).
//
// Tata letak meniru Black Souls 1: battleback penuh, musuh di tengah dengan
// bar HP & gauge AP tipis, log satu baris di atas, perintah di kiri bawah,
// status party di kanan bawah. Tambahan TUMBAL: strip "giliran berikutnya"
// yang diisi dari BattleSystem::predictTurns() (traversal salinan Timeline).
//
// Semua aturan ada di BattleSystem. View ini hanya:
//   1) memutar BattleEvent dari Queue satu per satu (animasi, angka damage),
//   2) menjalankan waktu ATB saat tidak ada animasi (mode wait),
//   3) mengirim pilihan pemain lewat BattleSystem::submit().
// ============================================================================

#include <string>
#include "raylib.h"
#include "core/battle/BattleSystem.hpp"

class BattleView {
public:
    void begin(BattleSystem* bs, const std::string& battleback);
    // true bila battle selesai dan hasil sudah dikonfirmasi pemain
    bool update(float dt);
    void draw(double time);
    BattleSystem::Phase result() const { return bs_ ? bs_->phase() : BattleSystem::Escaped; }

    // untuk --shots: paksa buka menu jurus
    void debugOpenSkills();

private:
    enum Menu { MenuNone, MenuCommand, MenuSkill, MenuTarget };
    struct Popup {
        int    target = -1;
        std::string text;
        Color  color = WHITE;
        float  t = 0.0f;        // waktu hidup (detik)
        float  size = 16;
        bool   active = false;
    };

    BattleSystem* bs_ = nullptr;
    std::string   back_;
    Menu  menu_ = MenuNone;
    int   cmdIndex_ = 0, skillIndex_ = 0, targetIndex_ = 0;
    int   pendingSkill_ = SKL_SERANG;
    int   skillList_[8] = {};
    int   skillCount_ = 0;

    bool        playing_ = false;
    BattleEvent cur_;
    float       evTime_ = 0.0f, evDur_ = 0.0f;
    std::string log_;
    float       logTime_ = 0.0f;
    Popup       popups_[24];
    float       flash_[BattleSystem::MAX_BATTLERS] = {};
    float       dying_[BattleSystem::MAX_BATTLERS] = {};
    bool        gone_[BattleSystem::MAX_BATTLERS] = {};
    int         hpShown_[BattleSystem::MAX_BATTLERS] = {};
    float       hpBar_[BattleSystem::MAX_BATTLERS] = {};
    float       chantGlow_[BattleSystem::MAX_BATTLERS] = {};
    int         activeActor_ = -1;
    float       shake_ = 0.0f;
    float       intro_ = 0.0f;
    bool        finished_ = false;
    float       resultTime_ = 0.0f;
    Vector2     pos_[BattleSystem::MAX_BATTLERS] = {};   // titik dasar (kaki) musuh / baris party

    void layout();
    void startEvent(const BattleEvent& e);
    void popup(int target, const std::string& text, Color c, float size = 16);
    void openCommand();
    void handleInput();
    void submit(int skill, int target);
    Vector2 popupAnchor(int i) const;

    void drawEnemies(double time);
    void drawQueue();
    void drawStatus(double time);
    void drawMenus(double time);
    void drawPopups();
    void drawResult(double time);
};
