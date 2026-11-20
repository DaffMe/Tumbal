#pragma once
// ============================================================================
// Settings — pengaturan pemain yang bertahan di antara sesi (saves/settings.json).
// Saat ini: bahasa (English bawaan, Bahasa Indonesia). Kamus terjemahan
// dimuat dari assets/lang/en.json lewat core/util/Lang.
// ============================================================================

#include "core/util/Lang.hpp"

namespace settings {

void load();                      // muat kamus + pengaturan; bawaan English bila belum ada berkas
void setLanguage(Language l);     // ganti bahasa & langsung simpan
void toggleLanguage();

}  // namespace settings
