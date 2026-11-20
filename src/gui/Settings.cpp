#include "Settings.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include "Assets.hpp"
#include "Audio.hpp"
#include "Ui.hpp"
#include "core/util/Json.hpp"
#include "raylib.h"

namespace settings {

static constexpr int VIEW_W = 640, VIEW_H = 360;   // resolusi logis (sama dengan App)
static Values g_values;
static int g_lastFullscreen = BORDERLESS;   // mode yang dipakai Alt+Enter / F11
static const int FPS_VALUES[FPS_OPTION_COUNT] = {0, 30, 60, 120, 144, -1};

Values& get() { return g_values; }
int fpsOption(int index) { return FPS_VALUES[(index >= 0 && index < FPS_OPTION_COUNT) ? index : 0]; }

static std::string path() { return Assets::get().savesDir() + "/settings.json"; }

void save() {
    std::FILE* f = std::fopen(path().c_str(), "wb");
    if (f == nullptr) return;
    const Values& v = g_values;
    std::fprintf(f,
                 "{\"lang\":\"%s\",\"display\":%d,\"windowScale\":%d,\"scaling\":%d,\"vsync\":%d,"
                 "\"fpsCap\":%d,\"musicVolume\":%d,\"sfxVolume\":%d,\"showFps\":%d}\n",
                 v.lang == LANG_ID ? "id" : "en", v.display, v.windowScale, v.scaling, v.vsync ? 1 : 0, v.fpsCap,
                 v.musicVolume, v.sfxVolume, v.showFps ? 1 : 0);
    std::fclose(f);
}

static int clampi(int x, int lo, int hi) { return x < lo ? lo : (x > hi ? hi : x); }

void load() {
    lang::clear();
    std::string err;
    if (!lang::load(Assets::get().path("assets/lang/en.json"), &err))
        std::fprintf(stderr, "Kamus bahasa Inggris tidak termuat: %s\n", err.c_str());
    Values v;                                                // bawaan: English, jendela 2x
    if (JsonValue* j = JsonValue::parseFile(path(), nullptr)) {
        v.lang = j->getString("lang", "en") == "id" ? LANG_ID : LANG_EN;
        v.display = clampi(j->getInt("display", v.display), 0, DISPLAY_COUNT - 1);
        v.windowScale = clampi(j->getInt("windowScale", v.windowScale), 1, 8);
        v.scaling = clampi(j->getInt("scaling", v.scaling), 0, SCALING_COUNT - 1);
        v.vsync = j->getInt("vsync", 1) != 0;
        v.fpsCap = clampi(j->getInt("fpsCap", v.fpsCap), 0, FPS_OPTION_COUNT - 1);
        v.musicVolume = clampi(j->getInt("musicVolume", v.musicVolume), 0, 10);
        v.sfxVolume = clampi(j->getInt("sfxVolume", v.sfxVolume), 0, 10);
        v.showFps = j->getInt("showFps", 0) != 0;
        delete j;
    }
    g_values = v;
    if (v.display != WINDOWED) g_lastFullscreen = v.display;
    apply();
}

int maxWindowScale() {
    int m = GetCurrentMonitor();
    int s = std::min((GetMonitorWidth(m) - 40) / VIEW_W, (GetMonitorHeight(m) - 80) / VIEW_H);
    return s < 1 ? 1 : s;
}

// Pindah mode layar. Keluar dulu dari mode yang aktif, baru masuk ke mode tujuan.
static void applyDisplay() {
    Values& v = g_values;
    if (!IsWindowReady()) return;
    if (IsWindowFullscreen() && v.display != FULLSCREEN) ToggleFullscreen();
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) && v.display != BORDERLESS) ToggleBorderlessWindowed();
    int m = GetCurrentMonitor();
    if (v.display == WINDOWED) {
        v.windowScale = clampi(v.windowScale, 1, maxWindowScale());
        int w = VIEW_W * v.windowScale, h = VIEW_H * v.windowScale;
        SetWindowSize(w, h);
        Vector2 mp = GetMonitorPosition(m);
        SetWindowPosition(static_cast<int>(mp.x) + (GetMonitorWidth(m) - w) / 2,
                          static_cast<int>(mp.y) + (GetMonitorHeight(m) - h) / 2);
    } else if (v.display == BORDERLESS) {
        if (!IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
    } else {
        // fullscreen eksklusif memakai resolusi asli monitor
        if (!IsWindowFullscreen()) {
            SetWindowSize(GetMonitorWidth(m), GetMonitorHeight(m));
            ToggleFullscreen();
        }
    }
}

void apply() {
    Values& v = g_values;
    lang::setLanguage(v.lang);
    applyDisplay();
    if (v.vsync) SetWindowState(FLAG_VSYNC_HINT);
    else ClearWindowState(FLAG_VSYNC_HINT);
    int fps = fpsOption(v.fpsCap);
    if (fps == 0) {
        int hz = GetMonitorRefreshRate(GetCurrentMonitor());
        fps = (hz >= 30 && hz <= 360) ? hz : 60;           // ikut refresh rate monitor
    }
    SetTargetFPS(fps < 0 ? 0 : fps);
    audio::setMusicVolume(v.musicVolume / 10.0f);
    audio::setSfxVolume(v.sfxVolume / 10.0f);
}

void setLanguage(Language l) {
    g_values.lang = l;
    lang::setLanguage(l);
    save();
}

void toggleLanguage() { setLanguage(g_values.lang == LANG_EN ? LANG_ID : LANG_EN); }

void setDisplay(int mode) {
    g_values.display = clampi(mode, 0, DISPLAY_COUNT - 1);
    if (g_values.display != WINDOWED) g_lastFullscreen = g_values.display;
    applyDisplay();
    save();
}

void toggleFullscreen() { setDisplay(g_values.display == WINDOWED ? g_lastFullscreen : WINDOWED); }

}  // namespace settings
