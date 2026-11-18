#pragma once
// ============================================================================
// Rng — pembangkit bilangan acak xorshift32 sederhana.
// Dibuat sendiri supaya hasil battle deterministik untuk seed yang sama
// (penting untuk unit test dan fitur rewind: state RNG ikut disimpan).
// ============================================================================

#include <cstdint>

class Rng {
public:
    explicit Rng(std::uint32_t seed = 0x9E3779B9u) : state_(seed != 0 ? seed : 1u) {}

    std::uint32_t next() {
        std::uint32_t x = state_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state_ = x;
    }

    // Bilangan bulat acak di [lo, hi]
    int range(int lo, int hi) {
        return lo + static_cast<int>(next() % static_cast<std::uint32_t>(hi - lo + 1));
    }

    // Bilangan real acak di [0, 1)
    float unit() { return static_cast<float>(next() & 0xFFFFFFu) / 16777216.0f; }

    bool chance(float p) { return unit() < p; }

private:
    std::uint32_t state_;
};
