#pragma once
// ============================================================================
// DialogView — kotak pesan & pilihan untuk Interpreter event (M3).
// Kotak di bawah layar dengan jendela nama terpisah (seperti BS1), teks
// muncul huruf demi huruf, pilihan di jendela kecil di atas kanan kotak.
// Hanya membaca state Interpreter dan memanggil resume()/choose().
// ============================================================================

#include <string>
#include "core/event/Interpreter.hpp"

class DialogView {
public:
    void update(Interpreter& it, float dt);
    void draw(const Interpreter& it, double time);

private:
    std::string shownText_;     // teks yang sedang diketik
    float       typed_ = 0.0f;  // jumlah huruf yang tampil
    int         choiceIndex_ = 0;
    bool        wasChoice_ = false;
};
