#include "SettingsView.hpp"

#include <cstdio>
#include <string>
#include "Audio.hpp"
#include "Settings.hpp"
#include "Ui.hpp"
#include "core/util/Lang.hpp"

namespace {

enum Item { LANGUAGE = 0, DISPLAY, WINDOW_SIZE, SCALING, VSYNC, FPS, MUSIC, SFX, SHOW_FPS, BACK, ITEM_COUNT };

const char* LABELS[ITEM_COUNT] = {"Bahasa", "Mode Layar", "Ukuran Jendela", "Skala Gambar", "VSync", "Batas FPS",
                                  "Volume Musik", "Volume Efek", "Tampilkan FPS", "Kembali"};
const char* DISPLAY_NAMES[] = {"Jendela", "Borderless", "Layar Penuh (eksklusif)"};
const char* SCALING_NAMES[] = {"Pixel-perfect", "Penuhi layar"};
const char* HINTS[ITEM_COUNT] = {
    "Bahasa teks di dalam game.",
    "Borderless: layar penuh tanpa ganti resolusi. Eksklusif: resolusi monitor penuh. Alt+Enter / F11 = cepat.",
    "Ukuran jendela saat mode Jendela.",
    "Pixel-perfect: piksel tajam, ada bingkai hitam. Penuhi layar: gambar mengisi layar, sedikit lebih halus.",
    "Menyamakan frame dengan monitor agar tidak ada sobekan layar.",
    "Turunkan ke 30 atau 60 untuk menghemat baterai di laptop.",
    "", "", "Menampilkan angka FPS di pojok layar.", ""};

bool confirm() { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
bool cancel() { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }

int wrapi(int x, int n) { return ((x % n) + n) % n; }

std::string valueText(int item) {
    const settings::Values& v = settings::get();
    char buf[64];
    switch (item) {
        case LANGUAGE: return lang::languageName(v.lang);
        case DISPLAY: return tr(DISPLAY_NAMES[v.display]);
        case WINDOW_SIZE:
            std::snprintf(buf, sizeof buf, "%dx  (%d x %d)", v.windowScale, 640 * v.windowScale, 360 * v.windowScale);
            return buf;
        case SCALING: return tr(SCALING_NAMES[v.scaling]);
        case VSYNC: return tr(v.vsync ? "Nyala" : "Mati");
        case FPS: {
            int f = settings::fpsOption(v.fpsCap);
            if (f == 0) return tr("Ikut monitor");
            if (f < 0) return tr("Tanpa batas");
            return std::to_string(f);
        }
        case SHOW_FPS: return tr(v.showFps ? "Nyala" : "Mati");
        default: return "";
    }
}

}  // namespace

void SettingsView::change(int dir) {
    settings::Values& v = settings::get();
    switch (index_) {
        case LANGUAGE: settings::toggleLanguage(); return;
        case DISPLAY: settings::setDisplay(wrapi(v.display + dir, settings::DISPLAY_COUNT)); return;
        case WINDOW_SIZE:
            if (v.display != settings::WINDOWED) return;
            v.windowScale = wrapi(v.windowScale - 1 + dir, settings::maxWindowScale()) + 1;
            break;
        case SCALING: v.scaling = wrapi(v.scaling + dir, settings::SCALING_COUNT); break;
        case VSYNC: v.vsync = !v.vsync; break;
        case FPS: v.fpsCap = wrapi(v.fpsCap + dir, settings::FPS_OPTION_COUNT); break;
        case MUSIC: v.musicVolume = v.musicVolume + dir < 0 ? 0 : (v.musicVolume + dir > 10 ? 10 : v.musicVolume + dir); break;
        case SFX: v.sfxVolume = v.sfxVolume + dir < 0 ? 0 : (v.sfxVolume + dir > 10 ? 10 : v.sfxVolume + dir); break;
        case SHOW_FPS: v.showFps = !v.showFps; break;
        default: return;
    }
    settings::apply();
    settings::save();
    if (index_ == SFX) audio::play(audio::PILIH);
}

bool SettingsView::update() {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) index_ = wrapi(index_ - 1, ITEM_COUNT);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) index_ = wrapi(index_ + 1, ITEM_COUNT);
    if (cancel()) return true;
    if (index_ == BACK) return confirm();
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) change(-1);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || confirm()) change(+1);
    return false;
}

void SettingsView::draw(Rectangle area, double time) {
    ui::window(area, 0.96f);
    ui::text("Pengaturan", area.x + 16, area.y + 8, ui::AMBER);
    const settings::Values& v = settings::get();
    float y = area.y + 34;
    for (int i = 0; i < ITEM_COUNT; ++i, y += 21) {
        bool sel = i == index_;
        bool disabled = i == WINDOW_SIZE && v.display != settings::WINDOWED;
        Color c = disabled ? Fade(ui::DIM, 0.5f) : (sel ? ui::AMBER : ui::BONE);
        if (i == BACK) y += 4;
        ui::text(LABELS[i], area.x + 30, y, c);
        if (sel) ui::cursor(area.x + 16, y + 8, time);
        float vx = area.x + area.width * 0.48f;
        if (i == MUSIC || i == SFX) {
            int level = i == MUSIC ? v.musicVolume : v.sfxVolume;
            for (int k = 0; k < 10; ++k)
                DrawRectangle(static_cast<int>(vx + k * 13), static_cast<int>(y + 4), 10, 10,
                              k < level ? (sel ? ui::AMBER : ui::BONE) : Fade(ui::DIM, 0.4f));
            ui::text(std::to_string(level * 10) + "%", vx + 136, y, c);
        } else if (i != BACK) {
            std::string val = valueText(i);
            ui::text(sel && !disabled ? "<  " + val + "  >" : val, vx, y, c);
        }
    }
    const char* hint = HINTS[index_];
    float hy = area.y + area.height - 34;
    if (index_ == WINDOW_SIZE && v.display != settings::WINDOWED) hint = "Hanya berlaku di mode Jendela.";
    if (hint[0] != '\0') {
        std::vector<std::string> lines = ui::wrap(hint, area.width - 32, 10);
        for (size_t k = 0; k < lines.size() && k < 2; ++k) ui::text(lines[k], area.x + 16, hy + k * 12.0f, ui::DIM, 10);
    }
}
