#pragma once
// ============================================================================
// Lang — bahasa permainan (Inggris bawaan, Bahasa Indonesia di Pengaturan).
//
// Teks sumber di kode, data, dan skrip event ditulis dalam Bahasa Indonesia.
// Teks itu sekaligus menjadi KUNCI kamus: tr("Sukma tidak cukup.") mencari
// padanan Inggrisnya di assets/lang/en.json. Bila bahasa = Indonesia, atau
// kunci belum diterjemahkan, teks sumber dikembalikan apa adanya.
//
// Kamus disimpan di tabel hash open addressing berukuran tetap (array mentah,
// tanpa container STL) karena GUI memanggil tr() berkali-kali setiap frame.
// Teks dengan isian memakai format printf: trf("Didapat: %s", nama).
// ============================================================================

#include <string>

enum Language { LANG_EN = 0, LANG_ID = 1 };

namespace lang {

void        setLanguage(Language l);
Language    language();
const char* languageName(Language l);        // "English" / "Bahasa Indonesia"

// Muat kamus Indonesia -> Inggris dari berkas JSON {"teks id": "teks en", ...}.
bool load(const std::string& path, std::string* error = nullptr);
void clear();
int  size();                                  // jumlah entri kamus
bool has(const std::string& id);              // kunci ada di kamus (untuk uji cakupan)

// Terjemahkan teks sumber (Indonesia) ke bahasa aktif.
std::string tr(const std::string& id);
// Terjemahkan format printf lalu isi argumennya.
std::string trf(const char* idFormat, ...) __attribute__((format(printf, 1, 2)));

}  // namespace lang

using lang::tr;
using lang::trf;
