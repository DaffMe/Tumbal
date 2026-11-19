#include "BattleView.hpp"

#include <cmath>
#include <cstdio>
#include "Assets.hpp"
#include "Audio.hpp"
#include "Ui.hpp"

static constexpr float VIEW_W = 640, VIEW_H = 360;
static constexpr float BOTTOM_Y = 262;

static bool keyConfirm() { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
static bool keyCancel() { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }
static bool keyUp() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
static bool keyDown() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
static bool keyLeft() { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
static bool keyRight() { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }

static const char* COMMANDS[] = {"Serang", "Jurus", "Bertahan", "Barang", "Kabur"};
static constexpr int COMMAND_COUNT = 5;

// ------------------------------------------------------------------ setup

void BattleView::begin(BattleSystem* bs, const std::string& battleback) {
    bs_ = bs;
    back_ = battleback;
    menu_ = MenuNone;
    cmdIndex_ = skillIndex_ = targetIndex_ = 0;
    playing_ = false;
    log_.clear();
    logTime_ = 0;
    for (auto& p : popups_) p.active = false;
    for (int i = 0; i < BattleSystem::MAX_BATTLERS; ++i) {
        flash_[i] = dying_[i] = chantGlow_[i] = 0;
        gone_[i] = false;
        if (i < bs->count()) {
            hpShown_[i] = bs->battler(i).hp;
            hpBar_[i] = static_cast<float>(hpShown_[i]);
        }
    }
    activeActor_ = -1;
    shake_ = 0;
    intro_ = 0.6f;
    finished_ = false;
    resultTime_ = 0;
    layout();
}

void BattleView::layout() {
    // musuh: tersebar di tengah, kaki di y=236
    int first = bs_->partyCount();
    int n = bs_->count() - first;
    float totalW = 0;
    float widths[BattleSystem::MAX_ENEMY] = {};
    for (int i = 0; i < n; ++i) {
        const Battler& b = bs_->battler(first + i);
        Texture2D t = Assets::get().texture(b.def ? b.def->battler : "");
        widths[i] = t.id ? static_cast<float>(t.width) : 80.0f;
        totalW += widths[i];
    }
    float gap = n > 1 ? std::fmin(40.0f, (560.0f - totalW) / (n - 1)) : 0;
    float x = VIEW_W / 2 - (totalW + gap * (n - 1)) / 2;
    for (int i = 0; i < n; ++i) {
        pos_[first + i] = {x + widths[i] / 2, 238.0f + ((i % 2) ? 6.0f : 0.0f)};
        x += widths[i] + gap;
    }
    for (int i = 0; i < first; ++i) pos_[i] = {150.0f, BOTTOM_Y + 12 + i * 22.0f};
}

void BattleView::debugOpenSkills() {
    if (bs_ && bs_->phase() == BattleSystem::Input) {
        openCommand();
        cmdIndex_ = 1;
        menu_ = MenuSkill;
    }
}

// ------------------------------------------------------------------ event

void BattleView::popup(int target, const std::string& text, Color c, float size) {
    for (auto& p : popups_) {
        if (p.active) continue;
        p.active = true;
        p.target = target;
        p.text = text;
        p.color = c;
        p.size = size;
        p.t = 0;
        // tumpukan: geser bila sudah ada popup pada target yang sama
        return;
    }
}

Vector2 BattleView::popupAnchor(int i) const {
    if (i < 0) return {VIEW_W / 2, 120};
    // anggota party: di celah antara nama dan bar HP agar angka HP tidak tertimpa
    if (i < bs_->partyCount()) return {226.0f, BOTTOM_Y + 4 + i * 22.0f};
    const Battler& b = bs_->battler(i);
    Texture2D t = Assets::get().texture(b.def ? b.def->battler : "");
    float h = t.id ? static_cast<float>(t.height) : 80.0f;
    return {pos_[i].x, pos_[i].y - h * 0.55f};
}

void BattleView::startEvent(const BattleEvent& e) {
    cur_ = e;
    evTime_ = 0;
    evDur_ = 0;
    playing_ = true;
    auto setLog = [this](const std::string& s) {
        if (s.empty()) return;
        log_ = s;
        logTime_ = 2.2f;
    };
    switch (e.kind) {
        case BattleEvent::Message: setLog(e.text); evDur_ = 0.9f; break;
        case BattleEvent::TurnStart: activeActor_ = e.src; evDur_ = 0.0f; break;
        case BattleEvent::UseSkill:
            setLog(e.text);
            if (e.skill != SKL_SERANG && e.skill != SKL_BERTAHAN) audio::play(audio::RAPAL, 0.6f);
            if (e.src >= 0) flash_[e.src] = 0.3f;
            evDur_ = 0.38f;
            break;
        case BattleEvent::ChantStart:
            setLog(e.text);
            audio::play(audio::RAPAL, 0.8f);
            if (e.src >= 0) chantGlow_[e.src] = 1.0f;
            evDur_ = 0.7f;
            break;
        case BattleEvent::ChantBroken:
            setLog(e.text);
            popup(e.dst, "BUYAR!", ui::CH_B, 16);
            audio::play(audio::BUYAR);
            evDur_ = 0.5f;
            break;
        case BattleEvent::Damage: {
            if (e.dst >= 0) {
                hpShown_[e.dst] -= e.value;
                if (hpShown_[e.dst] < 0) hpShown_[e.dst] = 0;
                flash_[e.dst] = 0.25f;
                if (e.dst < bs_->partyCount()) shake_ = 0.3f;
            }
            if (!e.text.empty()) setLog(e.text);
            audio::play(e.critical ? audio::KRITIS : (e.skill == SKL_SERANG ? audio::TEBAS : audio::PUKUL));
            if (e.critical) popup(e.dst, "KRITIS!", ui::AMBER, 16);
            popup(e.dst, std::to_string(e.value), e.dst < bs_->partyCount() ? Color{255, 120, 100, 255} : RAYWHITE,
                  e.critical ? 32 : 24);
            evDur_ = 0.42f;
            break;
        }
        case BattleEvent::Heal:
            if (e.dst >= 0) hpShown_[e.dst] += e.value;
            audio::play(audio::PULIH, 0.7f);
            popup(e.dst, "+" + std::to_string(e.value), Color{140, 240, 140, 255}, 24);
            evDur_ = 0.4f;
            break;
        case BattleEvent::Miss:
            setLog(e.text);
            popup(e.dst, "Elak!", ui::BONE, 16);
            audio::play(audio::ELAK);
            evDur_ = 0.4f;
            break;
        case BattleEvent::Counter:
            setLog(e.text);
            popup(e.dst, "Tangkis!", ui::AMBER, 16);
            audio::play(audio::PUKUL);
            evDur_ = 0.35f;
            break;
        case BattleEvent::StatusAdd:
            setLog(e.text);
            popup(e.dst, statusDef(e.status).name, Color{200, 150, 255, 255}, 16);
            evDur_ = 0.35f;
            break;
        case BattleEvent::StatusRemove: evDur_ = 0.0f; break;
        case BattleEvent::Delay:
            popup(e.dst, "Terhuyung", Color{255, 210, 140, 255}, 16);
            evDur_ = 0.3f;
            break;
        case BattleEvent::Death:
            setLog(e.text);
            if (e.dst >= 0) dying_[e.dst] = 0.0001f;
            audio::play(audio::TUMBANG);
            evDur_ = 0.6f;
            break;
        case BattleEvent::Victory:
        case BattleEvent::Defeat:
            setLog(e.text);
            evDur_ = 0.6f;
            break;
        case BattleEvent::Escaped:
        case BattleEvent::EscapeFail:
            setLog(e.text);
            audio::play(e.kind == BattleEvent::Escaped ? audio::LANGKAH1 : audio::GAGAL);
            evDur_ = 0.8f;
            break;
    }
}

// ------------------------------------------------------------------ input

void BattleView::openCommand() {
    menu_ = MenuCommand;
    const Battler& me = bs_->battler(bs_->inputActor());
    skillCount_ = 0;
    for (int i = 0; i < me.skillCount; ++i)
        if (me.skills[i] != SKL_SERANG) skillList_[skillCount_++] = me.skills[i];
    if (skillIndex_ >= skillCount_) skillIndex_ = 0;
    refreshItems();   // untuk meredupkan "Barang" bila kosong
}

void BattleView::submit(int skill, int target) {
    BattleAction a{skill, target, 0};
    if (bs_->submit(a)) menu_ = MenuNone;
}

void BattleView::submitItem(int item, int target) {
    BattleAction a{SKL_SERANG, target, item};
    if (bs_->submit(a)) menu_ = MenuNone;
}

void BattleView::refreshItems() {
    itemCount_ = 0;
    const ItemBag* bag = bs_->inventory();
    if (bag == nullptr) return;
    for (int id = 1; id < IT_COUNT; ++id) {
        const ItemDef* d = itemDef(id);
        if (d && d->inBattle && bag->count(id) > 0) itemList_[itemCount_++] = id;
    }
    if (itemIndex_ >= itemCount_) itemIndex_ = 0;
}

void BattleView::handleInput() {
    int actor = bs_->inputActor();
    if (actor < 0) return;
    if (menu_ == MenuNone) openCommand();
    if (keyUp() || keyDown() || ((keyLeft() || keyRight()) && menu_ == MenuTarget)) audio::play(audio::KURSOR, 0.5f);
    if (keyConfirm()) audio::play(audio::PILIH, 0.6f);
    if (keyCancel() && menu_ != MenuCommand) audio::play(audio::BATAL, 0.6f);

    if (menu_ == MenuCommand) {
        if (keyUp()) cmdIndex_ = (cmdIndex_ + COMMAND_COUNT - 1) % COMMAND_COUNT;
        if (keyDown()) cmdIndex_ = (cmdIndex_ + 1) % COMMAND_COUNT;
        if (keyConfirm()) {
            if (cmdIndex_ == 0) {
                pendingSkill_ = SKL_SERANG;
                menu_ = MenuTarget;
                targetIndex_ = bs_->firstTarget(actor, SKL_SERANG);
            } else if (cmdIndex_ == 1) {
                if (skillCount_ > 0) menu_ = MenuSkill;
            } else if (cmdIndex_ == 2) {
                submit(SKL_BERTAHAN, -1);
            } else if (cmdIndex_ == 3) {
                refreshItems();
                if (itemCount_ > 0) menu_ = MenuItem;
            } else {
                menu_ = MenuNone;
                bs_->tryEscape();
            }
        }
        return;
    }
    if (menu_ == MenuSkill) {
        if (keyUp()) skillIndex_ = (skillIndex_ + skillCount_ - 1) % skillCount_;
        if (keyDown()) skillIndex_ = (skillIndex_ + 1) % skillCount_;
        if (keyCancel()) menu_ = MenuCommand;
        if (keyConfirm()) {
            int sk = skillList_[skillIndex_];
            if (!bs_->canUse(actor, sk)) return;
            const SkillDef* s = skillDef(sk);
            if (s->scope == SC_MUSUH_SATU || (s->scope == SC_KAWAN_SATU && bs_->partyCount() > 1)) {
                pendingSkill_ = sk;
                menu_ = MenuTarget;
                targetIndex_ = bs_->firstTarget(actor, sk);
            } else {
                submit(sk, s->scope == SC_KAWAN_SATU ? actor : -1);
            }
        }
        return;
    }
    if (menu_ == MenuItem) {
        if (keyUp()) itemIndex_ = (itemIndex_ + itemCount_ - 1) % itemCount_;
        if (keyDown()) itemIndex_ = (itemIndex_ + 1) % itemCount_;
        if (keyCancel()) menu_ = MenuCommand;
        if (keyConfirm()) {
            int it = itemList_[itemIndex_];
            if (itemDef(it)->kind == IK_SERANG) {
                pendingItem_ = it;
                pendingSkill_ = 0;
                menu_ = MenuTarget;
                targetIndex_ = bs_->firstItemTarget(actor, it);
            } else {
                submitItem(it, bs_->firstItemTarget(actor, it));
            }
        }
        return;
    }
    if (menu_ == MenuTarget) {
        int dir = (keyLeft() || keyUp()) ? -1 : ((keyRight() || keyDown()) ? 1 : 0);
        if (dir != 0) {
            for (int k = 1; k <= bs_->count(); ++k) {
                int t = (targetIndex_ + dir * k + bs_->count() * 4) % bs_->count();
                bool ok = pendingSkill_ == 0 ? bs_->validItemTarget(actor, pendingItem_, t)
                                             : bs_->validTarget(actor, pendingSkill_, t);
                if (ok) { targetIndex_ = t; break; }
            }
        }
        if (keyCancel()) menu_ = pendingSkill_ == 0 ? MenuItem : (pendingSkill_ == SKL_SERANG ? MenuCommand : MenuSkill);
        if (keyConfirm()) {
            if (pendingSkill_ == 0) {
                if (bs_->validItemTarget(actor, pendingItem_, targetIndex_)) submitItem(pendingItem_, targetIndex_);
            } else if (bs_->validTarget(actor, pendingSkill_, targetIndex_)) {
                submit(pendingSkill_, targetIndex_);
            }
        }
    }
}

// ------------------------------------------------------------------ update

bool BattleView::update(float dt) {
    if (bs_ == nullptr) return true;
    bool fast = IsKeyDown(KEY_Z) && menu_ == MenuNone;   // tahan Z: percepat animasi (seperti BS1)
    float sdt = fast ? dt * 3.0f : dt;
    if (intro_ > 0) { intro_ -= dt; return false; }
    if (bs_->phase() != BattleSystem::Input) menu_ = MenuNone;
    if (logTime_ > 0) logTime_ -= dt;
    if (shake_ > 0) shake_ -= dt;
    for (int i = 0; i < BattleSystem::MAX_BATTLERS; ++i) {
        if (flash_[i] > 0) flash_[i] -= dt;
        if (dying_[i] > 0 && dying_[i] < 1.0f) { dying_[i] += sdt / 0.6f; if (dying_[i] >= 1.0f) gone_[i] = true; }
        if (chantGlow_[i] > 0 && i < bs_->count() && !bs_->battler(i).chanting) chantGlow_[i] -= dt * 3;
        if (i < bs_->count()) {
            float target = static_cast<float>(hpShown_[i]);
            hpBar_[i] += (target - hpBar_[i]) * (1.0f - std::exp(-dt * 10.0f));
        }
    }
    for (auto& p : popups_) if (p.active && (p.t += dt) > 1.0f) p.active = false;

    // 1) putar kejadian battle yang mengantre
    if (playing_) {
        evTime_ += sdt;
        if (evTime_ < evDur_) return false;
        playing_ = false;
    }
    BattleEvent e;
    while (!playing_ && bs_->events().dequeue(e)) {
        startEvent(e);
        if (evDur_ <= 0.0f) playing_ = false;
    }
    if (playing_) return false;

    // 2) selesai?
    if (bs_->over()) {
        if (bs_->phase() == BattleSystem::Escaped) return true;
        finished_ = true;
        resultTime_ += dt;
        if (resultTime_ > 0.6f && keyConfirm()) return true;
        return false;
    }

    // 3) input pemain atau jalankan waktu ATB (mode wait)
    if (bs_->phase() == BattleSystem::Input) {
        activeActor_ = bs_->inputActor();
        handleInput();
    } else {
        menu_ = MenuNone;
        bs_->advanceTime(dt * 60.0f * (fast ? 2.0f : 1.0f));
    }
    return false;
}

// ------------------------------------------------------------------ draw

void BattleView::drawEnemies(double time) {
    for (int i = bs_->partyCount(); i < bs_->count(); ++i) {
        if (gone_[i]) continue;
        const Battler& b = bs_->battler(i);
        Texture2D t = Assets::get().texture(b.def ? b.def->battler : "");
        if (t.id == 0) continue;
        float bob = static_cast<float>(std::sin(time * 1.6 + i * 1.3) * 1.5);
        float x = std::floor(pos_[i].x - t.width / 2.0f), y = std::floor(pos_[i].y - t.height + bob);
        float alpha = 1.0f;
        if (intro_ > 0) alpha = 1.0f - intro_ / 0.6f;
        Color tint = WHITE;
        if (dying_[i] > 0) {
            // runtuh ala RPG Maker: memerah lalu memudar
            alpha = 1.0f - dying_[i];
            tint = Color{255, static_cast<unsigned char>(120 * (1 - dying_[i])), static_cast<unsigned char>(120 * (1 - dying_[i])), 255};
        }
        // bayangan
        DrawEllipse(static_cast<int>(pos_[i].x), static_cast<int>(pos_[i].y), t.width * 0.35f, 6, Fade(BLACK, 0.45f * alpha));
        if (chantGlow_[i] > 0 || b.chanting) {
            float g = 0.35f + 0.25f * static_cast<float>(std::sin(time * 10));
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleGradient({pos_[i].x, pos_[i].y - t.height / 2.0f}, t.height * 0.6f, Fade(ui::CH_A, g), Fade(BLACK, 0));
            EndBlendMode();
        }
        DrawTexture(t, static_cast<int>(x), static_cast<int>(y), Fade(tint, alpha));
        if (flash_[i] > 0) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawTexture(t, static_cast<int>(x), static_cast<int>(y), Fade(WHITE, flash_[i] * 2.5f));
            EndBlendMode();
        }
        if (dying_[i] > 0) continue;
        // bar HP (tanpa angka, seperti BS1) + gauge AP / rapal
        float bw = 70;
        float bx = pos_[i].x - bw / 2, by = pos_[i].y + 8;
        ui::bar({bx, by, bw, 4}, hpBar_[i] / b.stats.mhp, ui::HP_A, ui::HP_B);
        if (b.chanting) ui::bar({bx, by + 7, bw, 3}, bs_->gauge(i), ui::CH_A, ui::CH_B);
        else ui::bar({bx, by + 7, bw, 3}, bs_->gauge(i), Fade(ui::AP_A, 0.6f), Fade(ui::AP_B, 0.6f));
        // ikon status singkat
        float sx = bx;
        for (int s = 1; s < ST_COUNT; ++s) {
            if (b.status[s] <= 0) continue;
            ui::text(statusDef(s).shortName, sx, by + 12, Color{200, 160, 255, 255}, 10);
            sx += ui::measure(statusDef(s).shortName, 10) + 3;
        }
    }
}

void BattleView::drawQueue() {
    // strip antrean: hasil traversal salinan Timeline (predictTurns)
    int order[6];
    int n = bs_->predictTurns(order, 6);
    float x = 8, y = 32;
    ui::text("Giliran:", x, y + 1, ui::DIM, 10);
    x += 44;
    for (int k = 0; k < n; ++k) {
        const Battler& b = bs_->battler(order[k]);
        std::string nm = b.name;
        std::size_t sp = nm.find(' ');
        if (b.enemy && sp != std::string::npos && nm.size() > 10) nm = nm.substr(0, sp) + nm.substr(nm.rfind(' '));
        float w = ui::measure(nm, 10) + 10;
        Color base = b.enemy ? Color{80, 26, 26, 220} : Color{80, 62, 26, 220};
        if (k == 0) base = b.enemy ? Color{150, 40, 36, 240} : Color{160, 118, 40, 240};
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), 13, base);
        if (b.chanting) DrawRectangleLines(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), 13, ui::CH_B);
        ui::text(nm, x + 5, y + 1, k == 0 ? WHITE : ui::BONE, 10, false);
        x += w + 2;
        if (k < n - 1) ui::text(">", x, y + 1, ui::DIM, 10, false);
        x += 8;
    }
}

void BattleView::drawStatus(double time) {
    Rectangle r{132, BOTTOM_Y, 500, 92};
    ui::window(r);
    for (int i = 0; i < bs_->partyCount(); ++i) {
        const Battler& b = bs_->battler(i);
        float y = r.y + 10 + i * 22;
        float sx = (shake_ > 0) ? static_cast<float>(std::sin(time * 80) * 3 * shake_ / 0.3f) : 0;
        bool active = (i == activeActor_ && bs_->phase() == BattleSystem::Input);
        if (active) DrawRectangle(static_cast<int>(r.x + 4), static_cast<int>(y - 2), static_cast<int>(r.width - 8), 20, Fade(ui::AMBER, 0.12f));
        if (flash_[i] > 0) DrawRectangle(static_cast<int>(r.x + 4), static_cast<int>(y - 2), static_cast<int>(r.width - 8), 20, Fade(ui::BLOOD, flash_[i]));
        ui::text(b.name, r.x + 12 + sx, y, b.alive() ? ui::BONE : ui::BLOOD);
        // HP
        ui::text("HP", r.x + 120, y + 4, ui::DIM, 10);
        ui::bar({r.x + 136, y + 9, 110, 4}, hpBar_[i] / b.stats.mhp, ui::HP_A, ui::HP_B);
        char buf[32];
        std::snprintf(buf, sizeof buf, "%d/%d", hpShown_[i], b.stats.mhp);
        ui::textRight(buf, r.x + 246, y - 4, hpShown_[i] < b.stats.mhp / 4 ? ui::BLOOD : RAYWHITE);
        // MP
        ui::text("MP", r.x + 258, y + 4, ui::DIM, 10);
        ui::bar({r.x + 274, y + 9, 60, 4}, static_cast<float>(b.mp) / (b.stats.mmp > 0 ? b.stats.mmp : 1), ui::MP_A, ui::MP_B);
        ui::textRight(std::to_string(b.mp), r.x + 334, y - 4, RAYWHITE);
        // AP / rapal
        ui::text(b.chanting ? "RP" : "AP", r.x + 346, y + 4, ui::DIM, 10);
        float g = bs_->gauge(i);
        if (b.chanting) ui::bar({r.x + 362, y + 9, 80, 4}, g, ui::CH_A, ui::CH_B);
        else ui::bar({r.x + 362, y + 9, 80, 4}, g, ui::AP_A, g >= 0.999f ? WHITE : ui::AP_B);
        // status
        float stx = r.x + 450;
        for (int s = 1; s < ST_COUNT && stx < r.x + r.width - 16; ++s) {
            if (b.status[s] <= 0) continue;
            ui::text(statusDef(s).shortName, stx, y + 4, Color{200, 160, 255, 255}, 10);
            stx += 22;
        }
    }
}

void BattleView::drawMenus(double time) {
    Rectangle cw{8, BOTTOM_Y, 120, 92};
    ui::window(cw);
    bool input = bs_->phase() == BattleSystem::Input && !playing_;
    for (int i = 0; i < COMMAND_COUNT; ++i) {
        float y = cw.y + 6 + i * 17;
        bool sel = input && menu_ != MenuNone && i == cmdIndex_;
        // perintah yang tidak bisa dipakai diredupkan (tanpa jurus/barang, boss tidak bisa dikabur)
        bool usable = !(i == 1 && skillCount_ == 0) && !(i == 3 && itemCount_ == 0) && !(i == 4 && !bs_->canEscape());
        Color c = !input ? ui::DIM : (!usable ? Fade(ui::DIM, 0.6f) : (sel ? ui::AMBER : ui::BONE));
        ui::text(COMMANDS[i], cw.x + 20, y, c);
        if (sel && menu_ == MenuCommand) ui::cursor(cw.x + 8, y + 8, time);
    }
    if (!input) return;
    const Battler& me = bs_->battler(bs_->inputActor());

    if (menu_ == MenuSkill) {
        Rectangle sw{132, 100, 376, static_cast<float>(26 + skillCount_ * 20)};
        sw.y = BOTTOM_Y - sw.height - 4;
        ui::window(sw, 0.94f);
        for (int i = 0; i < skillCount_; ++i) {
            const SkillDef* s = skillDef(skillList_[i]);
            float y = sw.y + 10 + i * 20;
            bool ok = bs_->canUse(me.slot, s->id);
            Color c = !ok ? ui::DIM : (i == skillIndex_ ? ui::AMBER : ui::BONE);
            ui::text(s->name, sw.x + 22, y, c);
            if (s->chant > 0) ui::text("rapal", sw.x + 170, y + 4, ui::CH_B, 10);
            if (s->delayPct > 0) ui::text("tunda", sw.x + 208, y + 4, Color{255, 210, 140, 255}, 10);
            ui::textRight(std::to_string(s->mpCost) + " MP", sw.x + sw.width - 12, y, ok ? ui::MP_B : ui::DIM);
            if (i == skillIndex_) ui::cursor(sw.x + 10, y + 8, time);
        }
        // jendela bantuan di atas (seperti Window_Help)
        const SkillDef* cur = skillDef(skillList_[skillIndex_]);
        Rectangle hw{8, 6, 624, 22};
        ui::window(hw, 0.95f);
        ui::text(cur->desc, hw.x + 8, hw.y + 3, ui::BONE);
    }
    if (menu_ == MenuItem) {
        Rectangle iw{132, 100, 376, static_cast<float>(26 + itemCount_ * 20)};
        iw.y = BOTTOM_Y - iw.height - 4;
        ui::window(iw, 0.94f);
        for (int i = 0; i < itemCount_; ++i) {
            const ItemDef* d = itemDef(itemList_[i]);
            float y = iw.y + 10 + i * 20;
            ui::text(d->name, iw.x + 22, y, i == itemIndex_ ? ui::AMBER : ui::BONE);
            ui::textRight("x" + std::to_string(bs_->inventory()->count(itemList_[i])), iw.x + iw.width - 12, y, ui::BONE);
            if (i == itemIndex_) ui::cursor(iw.x + 10, y + 8, time);
        }
        Rectangle hw{8, 6, 624, 22};
        ui::window(hw, 0.95f);
        ui::text(itemDef(itemList_[itemIndex_])->desc, hw.x + 8, hw.y + 3, ui::BONE);
    }
    if (menu_ == MenuTarget && targetIndex_ >= 0) {
        Vector2 a = popupAnchor(targetIndex_);
        if (targetIndex_ >= bs_->partyCount()) {
            Texture2D t = Assets::get().texture(me.slot >= 0 && bs_->battler(targetIndex_).def ? bs_->battler(targetIndex_).def->battler : "");
            float top = pos_[targetIndex_].y - (t.id ? t.height : 80) - 14;
            float bob = static_cast<float>(std::sin(time * 8) * 2);
            DrawTriangle({pos_[targetIndex_].x - 7, top - 8 + bob}, {pos_[targetIndex_].x, top + bob},
                         {pos_[targetIndex_].x + 7, top - 8 + bob}, ui::AMBER);
            ui::textCentered(bs_->battler(targetIndex_).name, pos_[targetIndex_].x, top - 26, ui::AMBER);
        } else {
            ui::cursor(136, a.y + 4, time);
        }
    }
}

void BattleView::drawPopups() {
    // offset vertikal bertumpuk untuk popup pada target sama
    for (auto& p : popups_) {
        if (!p.active) continue;
        Vector2 a = popupAnchor(p.target);
        float t = p.t;
        float rise = t < 0.15f ? -t / 0.15f * 14.0f : -14.0f - (t - 0.15f) * 8.0f;
        float alpha = t > 0.75f ? (1.0f - t) / 0.25f : 1.0f;
        int stack = 0;
        for (auto& q : popups_) if (&q != &p && q.active && q.target == p.target && q.t < p.t) ++stack;
        float y = a.y + rise - stack * (p.size * 0.8f);
        float x = a.x - ui::measure(p.text, p.size) / 2;
        // garis tepi gelap agar angka terbaca di atas latar apa pun
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
                if (dx || dy) ui::text(p.text, x + dx, y + dy, Fade(BLACK, 0.8f * alpha), p.size, false);
        ui::text(p.text, x, y, Fade(p.color, alpha), p.size, false);
    }
}

void BattleView::drawResult(double time) {
    if (!finished_) return;
    float a = std::fmin(1.0f, resultTime_ / 0.5f);
    if (bs_->phase() == BattleSystem::Defeat) {
        DrawRectangle(0, 0, 640, 360, Fade(BLACK, 0.7f * a));
        ui::textCentered("KAMU MATI", 320, 140, Fade(ui::BLOOD, a), 32);
        ui::textCentered("Sukma yang kamu bawa tercecer di tempat ini.", 320, 186, Fade(ui::DIM, a));
    } else {
        Rectangle r{190, 110, 260, 70};
        ui::window(r, 0.92f * a);
        ui::textCentered("MENANG", 320, r.y + 10, Fade(ui::AMBER, a), 32);
        ui::textCentered("Sukma +" + std::to_string(bs_->sukmaReward()), 320, r.y + 46, Fade(ui::BONE, a));
    }
    if (resultTime_ > 0.6f && static_cast<int>(time * 2) % 2 == 0) ui::textCentered("[Z]", 320, 210, ui::DIM);
}

void BattleView::draw(double time) {
    float ox = shake_ > 0 ? static_cast<float>(std::sin(time * 90) * 4 * shake_ / 0.3f) : 0;
    Texture2D bg = Assets::get().texture(back_);
    if (bg.id) DrawTexturePro(bg, {0, 0, static_cast<float>(bg.width), static_cast<float>(bg.height)},
                              {ox - 4, -4, 648, 368}, {0, 0}, 0, WHITE);
    else ClearBackground(Color{12, 10, 14, 255});
    drawEnemies(time);
    // log satu baris di atas (seperti BS1)
    if (logTime_ > 0 && menu_ != MenuSkill && menu_ != MenuItem) {
        float a = std::fmin(1.0f, logTime_ / 0.3f);
        DrawRectangleGradientH(0, 6, 320, 22, Fade(BLACK, 0.0f), Fade(BLACK, 0.65f * a));
        DrawRectangleGradientH(320, 6, 320, 22, Fade(BLACK, 0.65f * a), Fade(BLACK, 0.0f));
        ui::textCentered(log_, 320, 9, Fade(ui::BONE, a));
    }
    if (menu_ != MenuSkill && menu_ != MenuItem) drawQueue();
    drawMenus(time);
    drawStatus(time);
    drawPopups();
    if (intro_ > 0) DrawRectangle(0, 0, 640, 360, Fade(WHITE, intro_ / 0.6f * 0.8f));
    drawResult(time);
}
