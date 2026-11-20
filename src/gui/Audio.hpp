#pragma once
// ============================================================================
// Audio — musik latar (streaming, crossfade antar-lagu) dan efek suara.
// Musik dipilih dari property peta "musik" (lumbung, desa, ladang, ...),
// battle memakai "battle" / "boss". Semua berkas CC0 (lihat CREDITS.md).
// ============================================================================

#include <string>

namespace audio {

enum Sfx {
    KURSOR = 0, PILIH, BATAL, GAGAL, PULIH, RAPAL, ENCOUNTER, PUKUL, KRITIS, TUMBANG, BUYAR,
    LANGKAH1, LANGKAH2, TEBAS, ELAK, DAPAT, PINTU, TUNGKU, SFX_COUNT
};

void init();
void shutdown();
void update(float dt);                  // panggil tiap frame (stream + crossfade)
void playMusic(const std::string& name); // "" = senyap; nama sama = tidak diulang
void play(Sfx s, float volume = 1.0f);
void setMuted(bool m);
void setMusicVolume(float v);           // 0..1 (pengaturan pemain)
void setSfxVolume(float v);             // 0..1
bool muted();

}  // namespace audio
