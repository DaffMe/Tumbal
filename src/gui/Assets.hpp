#pragma once
// ============================================================================
// Assets — cache tekstur & lokasi folder aset (lapisan GUI).
// Folder dasar dicari sekali: folder kerja, folder exe (rilis: TUMBAL.exe di samping assets/),
// lalu folder di atas exe (pengembangan: bin/../assets).
// ============================================================================

#include <string>
#include <unordered_map>
#include "raylib.h"

class Assets {
public:
    static Assets& get();

    // "assets/chars/tumbal.png" -> path yang benar-benar ada.
    std::string path(const std::string& rel) const;
    // Folder simpanan (saves/) di folder dasar game; dibuat bila belum ada.
    std::string savesDir() const;
    // Tekstur dari path (relatif atau hasil path()). Dimuat sekali.
    Texture2D texture(const std::string& p);
    void unloadAll();

    // Tekstur cahaya radial putih (untuk lightmap), dibuat prosedural.
    Texture2D lightTexture();

private:
    std::unordered_map<std::string, Texture2D> cache_;
    Texture2D light_{};
    bool lightReady_ = false;
    mutable std::string base_;
    mutable bool baseReady_ = false;
    const std::string& base() const;
};
