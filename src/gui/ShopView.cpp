#include "ShopView.hpp"

#include <cmath>
#include <cstdio>
#include "Ui.hpp"
#include "core/data/GameData.hpp"
#include "raylib.h"

static bool confirm() { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
static bool cancel() { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }

std::string ShopView::update(Game& game) {
    int kind = game.shopMenu();
    if (kind == Game::SHOP_NONE) { index_ = 0; return ""; }
    if (cancel()) { game.closeShopMenu(); index_ = 0; return ""; }
    if (kind == Game::SHOP_TEMPA) return confirm() ? game.forge() : "";
    int n = shopCount();
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) index_ = (index_ + n - 1) % n;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) index_ = (index_ + 1) % n;
    return confirm() ? game.buy(index_) : "";
}

static void drawWallet(const GameState& gs, Rectangle r) {
    ui::window(r, 0.95f);
    ui::text("Sukma", r.x + 10, r.y + 6, ui::DIM);
    ui::textRight(std::to_string(gs.sukma()), r.x + r.width - 10, r.y + 6, ui::AMBER);
    ui::text("Bijih Besi", r.x + 10, r.y + 26, ui::DIM);
    ui::textRight(std::to_string(gs.count(IT_BIJIH_BESI)), r.x + r.width - 10, r.y + 26, ui::BONE);
}

void ShopView::draw(Game& game, double time) {
    int kind = game.shopMenu();
    if (kind == Game::SHOP_NONE) return;
    const GameState& gs = game.state();
    DrawRectangle(0, 0, 640, 360, Fade(BLACK, 0.35f));
    char buf[128];
    if (kind == Game::SHOP_DAGANG) {
        ui::textCentered("PEDAGANG KELILING", 320, 22, ui::AMBER, 30);
        int n = shopCount();
        Rectangle lw{24, 70, 400, static_cast<float>(20 + n * 24)};
        ui::window(lw, 0.95f);
        for (int i = 0; i < n; ++i) {
            const ShopEntry& e = shopEntry(i);
            const ItemDef* d = itemDef(e.item);
            float y = lw.y + 10 + i * 24;
            int stock = gs.shopStock(i);
            bool ok = gs.canBuy(i) == GameState::BUY_OK;
            bool sel = i == index_;
            Color c = stock == 0 ? Fade(ui::DIM, 0.5f) : (sel ? ui::AMBER : (ok ? ui::BONE : ui::DIM));
            ui::text(d ? d->name : "?", lw.x + 26, y, c);
            if (stock >= 0) {
                std::snprintf(buf, sizeof buf, stock == 0 ? "habis" : "sisa %d", stock);
                ui::text(buf, lw.x + 210, y + 4, ui::DIM, 10);
            }
            std::snprintf(buf, sizeof buf, "%d sukma", e.price);
            ui::textRight(buf, lw.x + lw.width - 12, y, gs.sukma() >= e.price ? ui::AMBER : ui::BLOOD);
            if (sel) ui::cursor(lw.x + 12, y + 8, time);
        }
        drawWallet(gs, {436, 70, 180, 52});
        Rectangle hw{24, lw.y + lw.height + 8, 592, 44};
        ui::window(hw, 0.95f);
        const ItemDef* d = itemDef(shopEntry(index_).item);
        for (const std::string& line : ui::wrap(d ? d->desc : "", hw.width - 20))
            { ui::text(line, hw.x + 10, hw.y + 6, ui::BONE); hw.y += 18; }
        ui::textCentered("[Z] beli    [X] pergi", 320, 330, ui::DIM);
        return;
    }
    // pandai besi
    ui::textCentered("TUNGKU TEMPA EMPU KARTA", 320, 22, ui::AMBER, 30);
    const WeaponDef& w = weaponDef(gs.classId());
    int lv = gs.weaponLevel();
    Rectangle tw{24, 70, 400, 196};
    ui::window(tw, 0.95f);
    std::snprintf(buf, sizeof buf, "%s +%d", w.name, lv);
    ui::text(buf, tw.x + 16, tw.y + 10, ui::BONE, 20);
    if (lv >= WEAPON_MAX) {
        ui::text("Senjata ini sudah ditempa sampai batasnya.", tw.x + 16, tw.y + 50, ui::DIM);
    } else {
        std::snprintf(buf, sizeof buf, "Tempa ke +%d", lv + 1);
        ui::text(buf, tw.x + 16, tw.y + 44, ui::DIM);
        const char* names[] = {"Serang", "Ilmu"};
        int now[] = {gs.hero().stats.atk, gs.hero().stats.mag};
        int add[] = {w.atk[lv + 1] - w.atk[lv], w.mag[lv + 1] - w.mag[lv]};
        float y = tw.y + 68;
        for (int i = 0; i < 2; ++i) {
            if (add[i] == 0) continue;
            ui::text(names[i], tw.x + 16, y, ui::DIM);
            ui::textRight(std::to_string(now[i]), tw.x + 150, y, ui::BONE);
            std::snprintf(buf, sizeof buf, "-> %d", now[i] + add[i]);
            ui::text(buf, tw.x + 160, y, Color{150, 220, 150, 255});
            y += 22;
        }
        std::snprintf(buf, sizeof buf, "%d sukma", w.costSukma[lv]);
        ui::text("Biaya", tw.x + 16, tw.y + 124, ui::DIM);
        ui::textRight(buf, tw.x + 250, tw.y + 124, gs.sukma() >= w.costSukma[lv] ? ui::AMBER : ui::BLOOD);
        std::snprintf(buf, sizeof buf, "%d Bijih Besi", w.costOre[lv]);
        ui::textRight(buf, tw.x + 384, tw.y + 124, gs.count(IT_BIJIH_BESI) >= w.costOre[lv] ? ui::AMBER : ui::BLOOD);
        float blink = 0.5f + 0.5f * static_cast<float>(std::sin(time * 5));
        ui::textCentered(gs.canForge() == GameState::FORGE_OK ? "[Z] tempa    [X] pergi" : "Bahan belum cukup.    [X] pergi",
                         tw.x + tw.width / 2, tw.y + tw.height - 28, Fade(ui::BONE, 0.6f + 0.4f * blink));
    }
    drawWallet(gs, {436, 70, 180, 52});
}
