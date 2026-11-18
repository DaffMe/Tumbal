#include "BattleView.hpp"

#include <cmath>
#include "Art.hpp"
#include "Gfx.hpp"
#include "Sfx.hpp"

using namespace gfx;

namespace {
constexpr float kTimelineX = 14;
constexpr float kTokenW    = 100;
constexpr float kTokenH    = 46;
constexpr float kSkillX    = 922;
}  // namespace

// ============================================================================
// Siklus hidup
// ============================================================================

void BattleView::begin(Battle* battle, int floorNo, const std::string& title, DsLog* dsLog) {
    battle_ = battle;
    floor_ = floorNo;
    title_ = title;
    dsLog_ = dsLog;
    pending_.clear();
    floats_.clear();
    log_.clear();
    eventTimer_ = 0;
    bannerT_ = 0;
    endT_ = 0;
    done_ = false;
    lastTurn_ = -1;
    flash_ = 0;
    for (int i = 0; i < MAX_UNITS; ++i) vis_[i] = Vis{};
    syncAll();
    for (int i = 0; i < battle_->unitCount(); ++i) vis_[i].x = enemyFoot(i).x;
    pullEvents();
}

void BattleView::syncAll() {
    for (int i = 0; i < battle_->unitCount(); ++i) {
        const Unit& u = battle_->unit(i);
        vis_[i].hp = vis_[i].ghost = static_cast<float>(u.hp);
        vis_[i].present = u.alive;
        vis_[i].dying = false;
        vis_[i].alpha = u.alive ? 1.0f : 0.0f;
    }
}

void BattleView::pullEvents() {
    BattleEvent e;
    while (battle_->pollEvent(e)) pending_.push_back(e);
}

// Satu event → animasi. Kembalikan durasi (detik) sebelum event berikutnya.
float BattleView::process(const BattleEvent& e) {
    if (e.type != EventType::DsOp && e.type != EventType::EmberChange && !e.text.empty()) {
        log_.push_back(e.text);
        while (log_.size() > 4) log_.pop_front();
    }
    switch (e.type) {
        case EventType::DsOp:
            if (dsLog_) dsLog_->push(e.text);
            return 0.0f;
        case EventType::TurnStart:
            if (e.source >= 0 && battle_->unit(e.source).side == Side::Enemy) return 0.25f;
            return 0.08f;
        case EventType::SkillUsed:
            banner_ = e.text;
            bannerT_ = 1.6f;
            if (e.source >= 0) vis_[e.source].lunge = 1.0f;
            sfx::play(sfx::Id::Skill, 0.7f);
            return 0.42f;
        case EventType::Damage: {
            if (e.target < 0) return 0.1f;
            Vis& v = vis_[e.target];
            v.hp = std::fmax(0.0f, v.hp - e.amount);
            v.flash = 1.0f;
            bool hero = battle_->unit(e.target).side == Side::Hero;
            Vector2 c = unitCenter(e.target);
            floats_.push_back({{c.x + GetRandomValue(-20, 20), c.y}, std::to_string(e.amount),
                               hero ? Color{255, 120, 100, 255} : col::ink, e.amount >= 60 ? 46.0f : 36.0f, 0});
            burst(c, hero ? col::blood : col::ember, 14, 220);
            shake(hero ? 7.0f : 4.0f);
            sfx::play(e.amount >= 60 ? sfx::Id::HeavyHit : sfx::Id::Hit);
            return 0.3f;
        }
        case EventType::Heal: {
            if (e.target < 0) return 0.1f;
            vis_[e.target].hp += e.amount;
            Vector2 c = unitCenter(e.target);
            floats_.push_back({c, "+" + std::to_string(e.amount), Color{150, 230, 130, 255}, 32, 0});
            burst(c, Color{150, 230, 130, 255}, 10, 120);
            sfx::play(sfx::Id::Heal, 0.8f);
            return 0.25f;
        }
        case EventType::Break: {
            Vector2 c = unitCenter(e.target);
            floats_.push_back({{c.x, c.y - 40}, "BREAK", col::gold, 52, 0});
            burst(c, col::gold, 40, 320);
            shake(12.0f);
            flash_ = 0.25f;
            sfx::play(sfx::Id::Break);
            return 0.6f;
        }
        case EventType::AvShift: {
            if (e.target < 0) return 0.1f;
            Vector2 c = unitCenter(e.target);
            std::string s = (e.amount > 0 ? "Tunda " : "Maju ") + std::to_string(std::abs(e.amount)) + "%";
            floats_.push_back({{c.x, c.y + 30}, s, Color{140, 180, 255, 255}, 24, 0});
            return 0.28f;
        }
        case EventType::StatusApplied: {
            if (e.target < 0) return 0.1f;
            Vector2 c = unitCenter(e.target);
            floats_.push_back({{c.x, c.y + 56}, e.text.substr(0, 28), col::arcane, 18, 0});
            return 0.16f;
        }
        case EventType::Death:
            if (e.target >= 0) {
                vis_[e.target].dying = true;
                burst(unitCenter(e.target), col::emberHot, 50, 260);
                sfx::play(sfx::Id::Death, 0.8f);
            }
            return 0.6f;
        case EventType::Summon:
            if (e.target >= 0) {
                Vis& v = vis_[e.target];
                v.present = true;
                v.dying = false;
                v.alpha = 0;
                v.hp = v.ghost = static_cast<float>(battle_->unit(e.target).maxHp);
                v.x = enemyFoot(e.target).x;
                v.tokenY = -1;
                burst(unitCenter(e.target), Color{140, 180, 255, 255}, 30, 200);
                sfx::play(sfx::Id::Summon);
            }
            return 0.45f;
        case EventType::Interject:
        case EventType::ExtraTurn:
            banner_ = e.text;
            bannerT_ = 1.6f;
            flash_ = 0.18f;
            sfx::play(sfx::Id::Bell, 0.8f);
            return 0.5f;
        case EventType::Rewind:
            syncAll();
            banner_ = "Pasir waktu berbalik...";
            bannerT_ = 1.6f;
            flash_ = 0.5f;
            sfx::play(sfx::Id::Bell);
            return 0.6f;
        case EventType::Info:
            banner_ = e.text;
            bannerT_ = 1.8f;
            return 0.5f;
        case EventType::Victory:
            sfx::play(sfx::Id::Victory);
            return 0.8f;
        case EventType::Defeat:
            sfx::play(sfx::Id::Defeat);
            return 0.8f;
        default:
            return 0.08f;
    }
}

// ============================================================================
// Update & input
// ============================================================================

void BattleView::update(float dt) {
    float speed = IsKeyDown(KEY_SPACE) ? 4.0f : 1.0f;
    eventTimer_ -= dt * speed;
    while (eventTimer_ <= 0 && !pending_.empty()) {
        BattleEvent e = pending_.front();
        pending_.pop_front();
        eventTimer_ += process(e);
    }
    if (eventTimer_ < 0) eventTimer_ = 0;

    for (int i = 0; i < battle_->unitCount(); ++i) {
        Vis& v = vis_[i];
        v.flash = std::fmax(0.0f, v.flash - dt * 4.0f);
        v.lunge = std::fmax(0.0f, v.lunge - dt * 3.0f);
        v.ghost = v.ghost > v.hp ? approach(v.ghost, v.hp, 3.0f, dt) : v.hp;
        if (v.dying) {
            v.alpha -= dt * 1.6f;
            if (v.alpha <= 0) { v.alpha = 0; v.present = false; v.dying = false; }
        } else if (v.present && v.alpha < 1) {
            v.alpha = std::fmin(1.0f, v.alpha + dt * 2.0f);
        }
    }
    layoutEnemies(dt);

    for (auto& f : floats_) f.t += dt;
    for (size_t i = 0; i < floats_.size();) {
        if (floats_[i].t > 1.3f) { floats_[i] = floats_.back(); floats_.pop_back(); }
        else ++i;
    }
    bannerT_ = std::fmax(0.0f, bannerT_ - dt);
    flash_ = std::fmax(0.0f, flash_ - dt * 1.5f);

    if (!busy()) {
        if (battle_->isOver()) {
            endT_ += dt;
            return;
        }
        if (battle_->state() == BattleState::WaitingForPlayer) {
            if (battle_->turnNumber() != lastTurn_ || battle_->currentIndex() < 0) {
                lastTurn_ = battle_->turnNumber();
                selectedSlot_ = battle_->unit(battle_->currentIndex()).empowered ? ULT_SLOT : 0;
            }
            handleInput();
        }
    }
}

void BattleView::tryUse(int slot, int target) {
    if (battle_->playerUseSkill(slot, target)) {
        pullEvents();
        hoverTarget_ = -1;
    }
}

void BattleView::handleInput() {
    int cur = battle_->currentIndex();
    const Unit& actor = battle_->unit(cur);

    // pilih skill lewat keyboard
    if (actor.empowered) {
        selectedSlot_ = ULT_SLOT;
    } else {
        for (int k = 0; k < actor.skillCount; ++k)
            if (IsKeyPressed(KEY_ONE + k) && battle_->canUseSkill(k)) {
                const Skill* s = battle_->skillAt(k);
                if (!needsTarget(s->target)) { tryUse(k, -1); return; }
                selectedSlot_ = k;
            }
    }
    if (IsKeyPressed(KEY_U) && actor.empowered) {
        const Skill* s = battle_->skillAt(ULT_SLOT);
        if (s && !needsTarget(s->target)) { tryUse(ULT_SLOT, -1); return; }
    }
    if (IsKeyPressed(KEY_R) && battle_->rewindTurn()) { pullEvents(); return; }

    const Skill* sel = battle_->skillAt(selectedSlot_);
    hoverTarget_ = -1;
    if (sel && needsTarget(sel->target)) {
        for (int i = 0; i < battle_->unitCount(); ++i) {
            if (!battle_->isValidTarget(cur, sel->target, i)) continue;
            Rectangle r = battle_->unit(i).side == Side::Enemy ? enemyRect(i) : heroCard(i);
            if (hover(r)) hoverTarget_ = i;
        }
        if (hoverTarget_ >= 0 && clicked()) { tryUse(selectedSlot_, hoverTarget_); return; }
    }
}

// ============================================================================
// Tata letak
// ============================================================================

Vector2 BattleView::enemyFoot(int idx) const {
    // Musuh hadir disebar merata di area tengah
    int order = 0, count = 0;
    for (int i = 0; i < battle_->unitCount(); ++i) {
        const Unit& u = battle_->unit(i);
        if (u.side != Side::Enemy) continue;
        if (!(vis_[i].present || u.alive) && i != idx) continue;
        if (i == idx) order = count;
        ++count;
    }
    if (count == 0) count = 1;
    float spacing = std::fmin(250.0f, 820.0f / count);
    float center = 600.0f;
    float x = center + (order - (count - 1) * 0.5f) * spacing;
    return {x, 455.0f};
}

void BattleView::layoutEnemies(float dt) {
    for (int i = 0; i < battle_->unitCount(); ++i)
        if (battle_->unit(i).side == Side::Enemy) vis_[i].x = approach(vis_[i].x, enemyFoot(i).x, 6.0f, dt);
}

Rectangle BattleView::enemyRect(int idx) const {
    Vector2 s = art::enemySize(static_cast<EnemyKind>(battle_->unit(idx).kind));
    float w = std::fmin(s.x, 200.0f);
    return {vis_[idx].x - w / 2, 455.0f - s.y, w, s.y + 50};
}

Rectangle BattleView::heroCard(int idx) const { return {130.0f + idx * 260.0f, 572.0f, 250.0f, 134.0f}; }

Vector2 BattleView::unitCenter(int idx) const {
    const Unit& u = battle_->unit(idx);
    if (u.side == Side::Hero) {
        Rectangle r = heroCard(idx);
        return {r.x + r.width * 0.6f, r.y + 30};
    }
    Vector2 s = art::enemySize(static_cast<EnemyKind>(u.kind));
    return {vis_[idx].x, 455.0f - s.y * 0.55f};
}

// ============================================================================
// Gambar
// ============================================================================

void BattleView::draw() {
    float t = gfx::time();
    art::drawBattleBackground(floor_, t);
    ambientEmbers(6.0f);
    drawEnemies();
    drawParticles();
    drawTimeline();
    drawHeroes();
    drawSkillPanel();
    drawTopBar();
    drawFloats();

    // log singkat
    size_t first = log_.size() > 3 ? log_.size() - 3 : 0;
    for (size_t i = first; i < log_.size(); ++i) {
        float a = 0.35f + 0.65f * (i - first + 1) / static_cast<float>(log_.size() - first);
        text(body(), log_[i], 134, 512 + (i - first) * 18.0f, 15, fade(col::inkDim, a));
    }

    if (bannerT_ > 0) {
        float a = std::fmin(1.0f, bannerT_ * 2.0f);
        DrawRectangleGradientH(240, 66, 400, 40, fade(BLACK, 0), fade(BLACK, 0.6f * a));
        DrawRectangleGradientH(640, 66, 400, 40, fade(BLACK, 0.6f * a), fade(BLACK, 0));
        textShadow(head(), banner_, 640, 72, 24, fade(col::ink, a), Center);
    }
    if (flash_ > 0) DrawRectangle(0, 0, VW, VH, fade(Color{255, 240, 220, 255}, flash_ * 0.5f));
    if (battle_->isOver() && !busy()) drawEnd();
}

void BattleView::drawTopBar() {
    DrawRectangleGradientV(0, 0, VW, 60, fade(BLACK, 0.75f), fade(BLACK, 0));
    textShadow(head(), title_, 640, 14, 26, col::gold, Center);

    // Ember
    float ex = 1000;
    text(body(), "EMBER", ex - 70, 20, 16, col::inkDim);
    for (int i = 0; i < MAX_EMBER; ++i) {
        Vector2 p = {ex + i * 22.0f, 29};
        bool on = i < battle_->ember();
        if (on) glow(p, 16, fade(col::ember, 0.5f));
        diamond(p, 8, on ? col::emberHot : col::inkFaint, on);
    }
    // Rewind
    bool canRewind = !busy() && battle_->state() == BattleState::WaitingForPlayer && battle_->hourglass() > 0 &&
                     battle_->historySize() >= 2;
    if (button({1130, 12, 136, 34}, "Rewind (" + std::to_string(battle_->hourglass()) + ")", canRewind, false, 18)) {
        if (battle_->rewindTurn()) pullEvents();
    }
    text(body(), "Giliran " + std::to_string(battle_->turnNumber()), 130, 20, 18, col::inkDim);
}

void BattleView::drawTimeline() {
    float dt = GetFrameTime();
    DrawRectangleGradientH(0, 0, 130, VH, fade(BLACK, 0.7f), fade(BLACK, 0));
    text(head(), "GILIRAN", kTimelineX + kTokenW / 2, 64, 16, col::gold, Center);

    auto token = [&](const Unit& u, float y, float av, bool current, float alpha) {
        Rectangle r = {kTimelineX, y, kTokenW, current ? kTokenH + 10 : kTokenH};
        Color edgeC = u.side == Side::Hero ? col::hero : col::enemy;
        DrawRectangleRec(r, fade(Color{18, 14, 18, 255}, 0.92f * alpha));
        if (u.side == Side::Hero) {
            art::drawPortrait(static_cast<HeroKind>(u.kind), {r.x + 3, r.y + 3, r.height - 6, r.height - 6}, alpha, current);
        } else {
            DrawRectangle(static_cast<int>(r.x + 3), static_cast<int>(r.y + 3), static_cast<int>(r.height - 6),
                          static_cast<int>(r.height - 6), fade(Color{60, 24, 24, 255}, alpha));
            text(head(), u.name.substr(0, 1), r.x + r.height / 2, r.y + r.height / 2 - 12, 22, fade(col::enemy, alpha), Center);
        }
        text(body(), art::shortName(u.name, 8), r.x + r.height + 2, r.y + 5, 13, fade(col::ink, alpha));
        if (!current) text(body(), std::to_string(static_cast<int>(av + 0.5f)), r.x + r.height + 2, r.y + 23, 15, fade(col::inkDim, alpha));
        else text(head(), "AKSI", r.x + r.height + 2, r.y + 26, 15, fade(col::emberHot, alpha));
        DrawRectangleLinesEx(r, current ? 2.0f : 1.0f, fade(current ? col::gold : edgeC, alpha));
    };

    float y = 86;
    int cur = battle_->currentIndex();
    if (cur >= 0) {
        token(battle_->unit(cur), y, 0, true, 1.0f);
        glow({kTimelineX + kTokenW / 2, y + 28}, 70, fade(col::ember, 0.15f));
    }
    y += kTokenH + 22;
    // Node Doubly Linked List: HEAD → TAIL (setiap unit tepat satu node)
    int slot = 0;
    battle_->timeline().forEach([&](const Timeline<Unit>::Node& n) {
        int idx = battle_->indexOf(n.item);
        float target = y + slot * (kTokenH + 8);
        Vis& v = vis_[idx];
        v.tokenY = v.tokenY < 0 ? target : approach(v.tokenY, target, 9.0f, dt);
        if (slot > 0) DrawLineEx({kTimelineX + 20, v.tokenY - 8}, {kTimelineX + 20, v.tokenY}, 2, col::inkFaint);
        token(*n.item, v.tokenY, n.av, false, slot < 9 ? 1.0f : 0.0f);
        ++slot;
    });
}

void BattleView::drawEnemies() {
    float t = gfx::time();
    for (int i = 0; i < battle_->unitCount(); ++i) {
        const Unit& u = battle_->unit(i);
        const Vis& v = vis_[i];
        if (u.side != Side::Enemy || !(v.present || v.dying) || v.alpha <= 0) continue;
        Vector2 foot = {v.x, 455.0f + v.lunge * 24.0f};
        bool hl = (i == hoverTarget_);
        art::drawEnemy(static_cast<EnemyKind>(u.kind), foot, v.alpha, v.flash, t + i * 1.3f, hl);

        float w = 160;
        float bx = v.x - w / 2, by = 470;
        bar({bx, by, w, 10}, v.hp / u.maxHp, col::blood, Color{30, 10, 10, 230}, v.ghost / u.maxHp);
        if (u.maxToughness > 0) {
            if (u.broken) {
                text(head(), "BREAK", v.x, by + 13, 15, fade(col::gold, v.alpha), Center);
            } else {
                bar({bx, by + 13, w, 5}, static_cast<float>(u.toughness) / u.maxToughness, Color{220, 210, 190, 255}, Color{30, 30, 30, 200});
            }
        }
        text(body(), u.name, v.x, by + 22, 16, fade(hl ? col::emberHot : col::ink, v.alpha), Center);
        std::string st;
        for (int s = 0; s < u.statusCount; ++s) {
            if (!st.empty()) st += "  ";
            st += std::string(statusName(u.statuses[s].type)) + " " + std::to_string(u.statuses[s].turns);
        }
        if (!st.empty()) text(body(), st, v.x, by + 40, 14, fade(col::arcane, v.alpha), Center);
        if (hl) {
            Rectangle r = enemyRect(i);
            diamond({v.x, r.y - 8}, 9, col::emberHot, true);
        }
    }
}

void BattleView::drawHeroes() {
    int cur = battle_->currentIndex();
    for (int i = 0; i < battle_->unitCount(); ++i) {
        const Unit& u = battle_->unit(i);
        if (u.side != Side::Hero) continue;
        const Vis& v = vis_[i];
        Rectangle r = heroCard(i);
        r.y -= v.lunge * 14.0f;
        bool active = (i == cur && !busy());
        bool targetable = (i == hoverTarget_);
        if (active) glow({r.x + r.width / 2, r.y + r.height / 2}, 200, fade(col::ember, 0.18f));
        panel(r);
        if (v.flash > 0) DrawRectangleRec(r, fade(col::blood, v.flash * 0.4f));
        if (targetable) DrawRectangleLinesEx(r, 3, col::emberHot);
        else if (active) DrawRectangleLinesEx(r, 2, col::gold);

        art::drawPortrait(static_cast<HeroKind>(u.kind), {r.x + 8, r.y + 8, 88, r.height - 16}, u.alive ? 1.0f : 0.4f, active);
        float tx = r.x + 106;
        text(head(), u.name, tx, r.y + 8, 22, u.alive ? col::ink : col::inkFaint);
        bar({tx, r.y + 40, 132, 12}, v.hp / u.maxHp, col::hp, Color{20, 26, 16, 230}, v.ghost / u.maxHp);
        text(body(), std::to_string(static_cast<int>(v.hp + 0.5f)) + " / " + std::to_string(u.maxHp), tx, r.y + 54, 15, col::inkDim);
        // Resolve
        float res = u.resolve / static_cast<float>(MAX_RESOLVE);
        bar({tx, r.y + 76, 132, 6}, res, res >= 1 ? col::emberHot : col::gold, Color{30, 24, 16, 230});
        text(body(), "Resolve", tx, r.y + 84, 13, col::inkFaint);
        std::string st;
        for (int s = 0; s < u.statusCount; ++s) {
            if (!st.empty()) st += " ";
            st += statusName(u.statuses[s].type);
        }
        if (!st.empty()) text(body(), st, tx + 52, r.y + 84, 13, col::arcane);

        if (!u.alive) {
            DrawRectangleRec(r, fade(BLACK, 0.55f));
            text(head(), "TUMBANG", r.x + r.width / 2, r.y + r.height / 2 - 12, 22, col::blood, Center);
        } else if (battle_->canInterject(i) && !busy()) {
            float pulse = 0.6f + 0.4f * std::sin(gfx::time() * 6);
            Rectangle b = {tx, r.y + 102, 132, 24};
            glow({b.x + b.width / 2, b.y + 12}, 50, fade(col::ember, 0.35f * pulse));
            if (button(b, "SELA (Interject)", true, false, 15)) {
                if (battle_->interject(i)) pullEvents();
            }
        }
    }
}

void BattleView::drawSkillPanel() {
    Rectangle p = {kSkillX, 520, 346, 186};
    panel(p);
    int cur = battle_->currentIndex();
    if (cur < 0 || battle_->isOver() || battle_->unit(cur).side != Side::Hero) {
        text(body(), busy() ? "..." : "", p.x + p.width / 2, p.y + 80, 20, col::inkDim, Center);
        return;
    }
    const Unit& u = battle_->unit(cur);
    text(head(), u.name, p.x + 14, p.y + 10, 20, col::gold);
    bool inputOk = !busy() && battle_->state() == BattleState::WaitingForPlayer;

    auto skillRow = [&](int slot, const Skill* s, float y, int key) {
        Rectangle b = {p.x + 12, y, p.width - 24, 34};
        bool usable = battle_->canUseSkill(slot) && inputOk;
        bool sel = (slot == selectedSlot_);
        std::string label = (key > 0 ? std::to_string(key) + "  " : "U  ") + s->name;
        bool h = hover(b);
        if (button(b, "", usable, sel)) {
            if (!needsTarget(s->target)) tryUse(slot, -1);
            else selectedSlot_ = slot;
        }
        text(head(), label, b.x + 12, b.y + 7, 19, usable ? (sel || h ? col::emberHot : col::ink) : col::inkFaint);
        for (int c = 0; c < s->emberCost; ++c) diamond({b.x + b.width - 16 - c * 16.0f, b.y + 17}, 6, col::ember, true);
        if (s->emberGain > 0) text(body(), "+1", b.x + b.width - 30, b.y + 9, 15, col::ember);
    };

    const Skill* shown = battle_->skillAt(selectedSlot_);
    if (u.empowered) {
        skillRow(ULT_SLOT, u.ultimate, p.y + 42, 0);
        shown = u.ultimate;
    } else {
        for (int k = 0; k < u.skillCount; ++k) {
            skillRow(k, u.skills[k], p.y + 40 + k * 38.0f, k + 1);
            if (hover({p.x + 12, p.y + 40 + k * 38.0f, p.width - 24, 34})) shown = u.skills[k];
        }
    }
    if (shown) textWrap(body(), shown->desc, p.x + 14, p.y + 156, p.width - 28, 14, col::inkDim);
    if (inputOk && shown && needsTarget(shown->target) && shown == battle_->skillAt(selectedSlot_))
        text(body(), "Pilih target...", p.x + p.width - 14, p.y + 12, 15, col::emberHot, Right);
}

void BattleView::drawFloats() {
    for (const auto& f : floats_) {
        float a = f.t < 1.0f ? 1.0f : 1.0f - (f.t - 1.0f) / 0.3f;
        float pop = 1.0f + 0.4f * std::fmax(0.0f, 1.0f - f.t * 6.0f);
        textShadow(head(), f.text, f.pos.x, f.pos.y - easeOut(f.t) * 50.0f, f.size * pop, fade(f.color, a), Center);
    }
}

void BattleView::drawEnd() {
    float a = std::fmin(1.0f, endT_ * 1.5f);
    bool won = battle_->state() == BattleState::Victory;
    DrawRectangle(0, 0, VW, VH, fade(BLACK, 0.6f * a));
    DrawRectangleGradientH(0, 250, VW / 2, 140, fade(BLACK, 0), fade(BLACK, 0.7f * a));
    DrawRectangleGradientH(VW / 2, 250, VW / 2, 140, fade(BLACK, 0.7f * a), fade(BLACK, 0));
    const char* msg = won ? "KEMENANGAN" : "NYALA PADAM";
    Color c = won ? col::gold : col::blood;
    textShadow(title(), msg, VW / 2.0f, 268, 76, fade(c, a), Center);
    text(body(), won ? "Musuh tumbang. Ash terkumpul." : "Party tumbang. Ash tertinggal di ruangan ini...",
         VW / 2.0f, 356, 20, fade(col::inkDim, a), Center);
    if (endT_ > 0.6f && (button({VW / 2.0f - 90, 420, 180, 44}, "Lanjut", true, false, 22) || IsKeyPressed(KEY_ENTER)))
        done_ = true;
}
