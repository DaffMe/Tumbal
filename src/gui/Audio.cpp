#include "Audio.hpp"

#include "Assets.hpp"
#include "raylib.h"

namespace audio {

static const char* SFX_FILES[SFX_COUNT] = {
    "kursor.ogg", "pilih.ogg", "batal.ogg", "gagal.ogg", "pulih.ogg", "rapal.ogg", "encounter.ogg", "pukul.ogg",
    "kritis.ogg", "tumbang.ogg", "buyar.ogg", "langkah1.ogg", "langkah2.ogg", "tebas.ogg", "elak.ogg", "dapat.ogg",
    "pintu.ogg", "tungku.ogg",
};

static bool  g_ready = false;
static bool  g_muted = false;
static Sound g_sfx[SFX_COUNT];
static Music g_cur{}, g_next{};
static bool  g_hasCur = false, g_hasNext = false;
static std::string g_curName, g_nextName;
static float g_fade = 1.0f;       // volume lagu saat ini
static float g_nextVol = 0.0f;
static constexpr float MUSIC_VOL = 0.55f;

static std::string musicPath(const std::string& name) {
    std::string ogg = Assets::get().path("assets/audio/music/" + name + ".ogg");
    if (FileExists(ogg.c_str())) return ogg;
    return Assets::get().path("assets/audio/music/" + name + ".mp3");
}

void init() {
    InitAudioDevice();
    g_ready = IsAudioDeviceReady();
    if (!g_ready) return;
    for (int i = 0; i < SFX_COUNT; ++i) {
        std::string p = Assets::get().path(std::string("assets/audio/sfx/") + SFX_FILES[i]);
        g_sfx[i] = LoadSound(p.c_str());
    }
}

void shutdown() {
    if (!g_ready) return;
    if (g_hasCur) UnloadMusicStream(g_cur);
    if (g_hasNext) UnloadMusicStream(g_next);
    for (auto& s : g_sfx) UnloadSound(s);
    CloseAudioDevice();
    g_ready = false;
}

void playMusic(const std::string& name) {
    if (!g_ready) return;
    if (name == g_curName && g_hasCur && !g_hasNext) return;
    if (name == g_nextName && g_hasNext) return;
    if (g_hasNext) { UnloadMusicStream(g_next); g_hasNext = false; }
    g_nextName = name;
    if (name.empty()) return;
    std::string p = musicPath(name);
    if (!FileExists(p.c_str())) return;
    g_next = LoadMusicStream(p.c_str());
    g_next.looping = true;
    g_hasNext = g_next.stream.buffer != nullptr;
    if (g_hasNext) {
        g_nextVol = 0.0f;
        SetMusicVolume(g_next, 0.0f);
        PlayMusicStream(g_next);
    }
}

void update(float dt) {
    if (!g_ready) return;
    float master = g_muted ? 0.0f : MUSIC_VOL;
    bool fadingOut = g_hasNext || g_nextName != g_curName;
    if (g_hasCur) {
        if (fadingOut) g_fade -= dt * 1.2f;
        if (g_fade < 0) g_fade = 0;
        SetMusicVolume(g_cur, g_fade * master);
        UpdateMusicStream(g_cur);
        if (g_fade <= 0.0f) {
            UnloadMusicStream(g_cur);
            g_hasCur = false;
            g_curName.clear();
        }
    }
    if (g_hasNext) {
        g_nextVol += dt * 0.8f;
        if (g_nextVol > 1) g_nextVol = 1;
        SetMusicVolume(g_next, g_nextVol * master);
        UpdateMusicStream(g_next);
        if (!g_hasCur) {
            // lagu baru menjadi lagu utama
            g_cur = g_next;
            g_hasCur = true;
            g_curName = g_nextName;
            g_fade = g_nextVol;
            g_hasNext = false;
        }
    } else if (g_hasCur && g_fade < 1.0f && g_nextName == g_curName) {
        g_fade += dt;
        if (g_fade > 1) g_fade = 1;
    }
}

void play(Sfx s, float volume) {
    if (!g_ready || g_muted || s < 0 || s >= SFX_COUNT) return;
    SetSoundVolume(g_sfx[s], volume * 0.8f);
    PlaySound(g_sfx[s]);
}

void setMuted(bool m) { g_muted = m; }
bool muted() { return g_muted; }

}  // namespace audio
