#include "Sfx.hpp"

#include <cmath>
#include <cstdlib>
#include <vector>
#include "raylib.h"

namespace sfx {

namespace {

constexpr int   kRate = 44100;
constexpr float kPi   = 3.14159265f;

Sound g_sounds[static_cast<int>(Id::COUNT)];
Sound g_drone;
bool  g_ready = false;
bool  g_muted = false;
float g_droneVol = 0.0f;

float noise() { return (std::rand() / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f; }

// Bangun Sound dari buffer float [-1, 1]
Sound makeSound(const std::vector<float>& buf) {
    std::vector<short> pcm(buf.size());
    for (size_t i = 0; i < buf.size(); ++i) {
        float v = buf[i];
        if (v > 1) v = 1;
        if (v < -1) v = -1;
        pcm[i] = static_cast<short>(v * 32000);
    }
    Wave w;
    w.frameCount = static_cast<unsigned int>(pcm.size());
    w.sampleRate = kRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm.data();
    return LoadSoundFromWave(w);   // raylib menyalin data
}

std::vector<float> synth(float seconds, float (*fn)(float t, float dur)) {
    int n = static_cast<int>(seconds * kRate);
    std::vector<float> b(n);
    for (int i = 0; i < n; ++i) b[i] = fn(i / static_cast<float>(kRate), seconds);
    return b;
}

// filter low-pass sederhana untuk noise
std::vector<float> lowpass(std::vector<float> b, float a) {
    float y = 0;
    for (float& v : b) { y += a * (v - y); v = y; }
    return b;
}

float env(float t, float attack, float decay) {
    if (t < attack) return t / attack;
    return std::exp(-(t - attack) / decay);
}

}  // namespace

void init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    std::srand(1234);

    auto set = [](Id id, const std::vector<float>& b) { g_sounds[static_cast<int>(id)] = makeSound(b); };

    set(Id::Click, synth(0.05f, [](float t, float) { return std::sin(2 * kPi * 1800 * t) * env(t, 0.001f, 0.012f) * 0.25f; }));

    set(Id::Hit, lowpass(synth(0.22f, [](float t, float) {
        return (noise() * 0.8f + std::sin(2 * kPi * 85 * t) * 1.2f) * env(t, 0.002f, 0.05f) * 0.7f;
    }), 0.35f));

    set(Id::HeavyHit, lowpass(synth(0.45f, [](float t, float) {
        float f = 70 - 30 * t;
        return (noise() * 0.9f + std::sin(2 * kPi * f * t) * 1.5f) * env(t, 0.003f, 0.11f) * 0.8f;
    }), 0.25f));

    set(Id::Break, synth(0.9f, [](float t, float) {
        float s = 0;
        const float fr[] = {1320, 1760, 2093, 2637};
        for (int i = 0; i < 4; ++i) s += std::sin(2 * kPi * fr[i] * t * (1 + 0.002f * i)) * std::exp(-t * (3 + i));
        return (s * 0.18f + noise() * env(t, 0.001f, 0.06f) * 0.5f) * 0.8f;
    }));

    set(Id::Heal, synth(0.6f, [](float t, float) {
        float f = t < 0.15f ? 523 : (t < 0.3f ? 659 : 784);
        return std::sin(2 * kPi * f * t) * env(std::fmod(t, 0.15f), 0.005f, 0.12f) * 0.22f * (1 - t / 0.6f);
    }));

    set(Id::Death, lowpass(synth(1.0f, [](float t, float) {
        float f = 110 * std::exp(-t * 1.5f);
        return (std::sin(2 * kPi * f * t) * 0.9f + noise() * 0.4f) * env(t, 0.01f, 0.35f) * 0.6f;
    }), 0.2f));

    set(Id::Skill, lowpass(synth(0.3f, [](float t, float d) {
        float e = std::sin(kPi * t / d);
        return noise() * e * e * 0.45f;
    }), 0.12f));

    set(Id::Bell, synth(1.8f, [](float t, float) {
        const float fr[] = {392, 784 * 1.002f, 1176, 1568 * 0.997f, 2100};
        const float am[] = {1.0f, 0.6f, 0.35f, 0.25f, 0.1f};
        float s = 0;
        for (int i = 0; i < 5; ++i) s += std::sin(2 * kPi * fr[i] * t) * am[i] * std::exp(-t * (1.2f + i * 0.8f));
        return s * 0.22f * env(t, 0.003f, 10.0f);
    }));

    set(Id::Step, lowpass(synth(0.12f, [](float t, float) { return noise() * env(t, 0.002f, 0.03f) * 0.5f; }), 0.15f));

    set(Id::Summon, synth(0.7f, [](float t, float) {
        float f = 200 + 600 * t;
        return std::sin(2 * kPi * f * t) * std::sin(kPi * t / 0.7f) * 0.18f + noise() * 0.05f * std::sin(kPi * t / 0.7f);
    }));

    set(Id::Victory, synth(1.6f, [](float t, float) {
        const float fr[] = {392, 494, 587, 784};
        float s = 0;
        for (int i = 0; i < 4; ++i) {
            float st = t - i * 0.12f;
            if (st > 0) s += std::sin(2 * kPi * fr[i] * st) * std::exp(-st * 2.0f);
        }
        return s * 0.16f;
    }));

    set(Id::Defeat, synth(2.2f, [](float t, float) {
        float s = std::sin(2 * kPi * 98 * t) + std::sin(2 * kPi * 116.5f * t) * 0.7f + std::sin(2 * kPi * 146.8f * t) * 0.5f;
        return s * 0.14f * env(t, 0.05f, 0.9f);
    }));

    // drone ambience 12 detik (diputar ulang terus): nada rendah berdenyut pelan
    std::vector<float> drone = synth(12.0f, [](float t, float d) {
        float lfo = 0.6f + 0.4f * std::sin(2 * kPi * t / d * 2);
        float s = std::sin(2 * kPi * 55 * t) * 0.5f + std::sin(2 * kPi * 82.4f * t) * 0.3f * lfo +
                  std::sin(2 * kPi * 110.3f * t) * 0.15f * (1 - lfo) + std::sin(2 * kPi * 164.8f * t) * 0.05f;
        float edge = std::fmin(1.0f, std::fmin(t, d - t) * 4.0f);   // fade di ujung agar loop mulus
        return s * 0.35f * edge;
    });
    g_drone = makeSound(lowpass(drone, 0.3f));
    g_ready = true;
}

void shutdown() {
    if (!g_ready) return;
    for (auto& s : g_sounds) UnloadSound(s);
    UnloadSound(g_drone);
    CloseAudioDevice();
    g_ready = false;
}

void play(Id id, float volume) {
    if (!g_ready || g_muted) return;
    Sound& s = g_sounds[static_cast<int>(id)];
    SetSoundVolume(s, volume);
    SetSoundPitch(s, 0.94f + (std::rand() % 120) / 1000.0f);   // variasi kecil
    PlaySound(s);
}

void updateAmbience(bool enabled, float intensity) {
    if (!g_ready) return;
    float target = (enabled && !g_muted) ? 0.5f * intensity : 0.0f;
    g_droneVol += (target - g_droneVol) * 0.03f;
    SetSoundVolume(g_drone, g_droneVol);
    if (!IsSoundPlaying(g_drone) && g_droneVol > 0.01f) PlaySound(g_drone);
}

void setMuted(bool m) { g_muted = m; }
bool muted() { return g_muted; }

}  // namespace sfx
