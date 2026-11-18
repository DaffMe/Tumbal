#pragma once
// ============================================================================
// Sfx — efek suara & ambience yang disintesis secara prosedural saat start
// (tanpa file audio eksternal). Hanya lapisan tampilan/audio.
// ============================================================================

namespace sfx {

enum class Id { Click, Hit, HeavyHit, Break, Heal, Death, Skill, Bell, Step, Summon, Victory, Defeat, COUNT };

void init();
void shutdown();
void play(Id id, float volume = 1.0f);
void updateAmbience(bool enabled, float intensity);   // drone latar, dipanggil tiap frame
void setMuted(bool muted);
bool muted();

}  // namespace sfx
