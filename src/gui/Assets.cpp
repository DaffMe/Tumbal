#include "Assets.hpp"

#include <cmath>

Assets& Assets::get() {
    static Assets a;
    return a;
}

std::string Assets::path(const std::string& rel) const {
    if (FileExists(rel.c_str()) || DirectoryExists(rel.c_str())) return rel;
    std::string alt = std::string(GetApplicationDirectory()) + "../" + rel;
    if (FileExists(alt.c_str()) || DirectoryExists(alt.c_str())) return alt;
    return rel;
}

Texture2D Assets::texture(const std::string& p) {
    auto it = cache_.find(p);
    if (it != cache_.end()) return it->second;
    std::string real = path(p);
    Texture2D t = LoadTexture(real.c_str());
    if (t.id != 0) SetTextureFilter(t, TEXTURE_FILTER_POINT);
    cache_[p] = t;
    return t;
}

Texture2D Assets::lightTexture() {
    if (lightReady_) return light_;
    const int S = 128;
    Image img = GenImageColor(S, S, BLANK);
    for (int y = 0; y < S; ++y) {
        for (int x = 0; x < S; ++x) {
            float dx = (x + 0.5f) / S * 2.0f - 1.0f;
            float dy = (y + 0.5f) / S * 2.0f - 1.0f;
            float d = std::sqrt(dx * dx + dy * dy);
            float a = d >= 1.0f ? 0.0f : (1.0f - d);
            a = a * a * (3.0f - 2.0f * a);   // smoothstep: tepi lembut
            ImageDrawPixel(&img, x, y, Color{255, 255, 255, static_cast<unsigned char>(a * 255)});
        }
    }
    light_ = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(light_, TEXTURE_FILTER_BILINEAR);
    lightReady_ = true;
    return light_;
}

void Assets::unloadAll() {
    for (auto& kv : cache_) UnloadTexture(kv.second);
    cache_.clear();
    if (lightReady_) UnloadTexture(light_);
    lightReady_ = false;
}
