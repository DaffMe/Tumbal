#include "DialogView.hpp"

#include <cmath>
#include "Assets.hpp"
#include "Ui.hpp"
#include "raylib.h"

static bool confirm() { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
static bool cancel() { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }

void DialogView::update(Interpreter& it, float dt) {
    if (it.state() == Interpreter::WaitMessage) {
        if (shownText_ != it.message() || wasChoice_) {
            shownText_ = it.message();
            typed_ = 0.0f;
            wasChoice_ = false;
        }
        bool done = typed_ >= static_cast<float>(shownText_.size());
        typed_ += dt * (IsKeyDown(KEY_Z) ? 120.0f : 45.0f);
        if (confirm()) {
            if (!done) typed_ = static_cast<float>(shownText_.size());
            else it.resume();
        }
        return;
    }
    if (it.state() == Interpreter::WaitChoice) {
        if (!wasChoice_) { choiceIndex_ = 0; wasChoice_ = true; }
        int n = it.optionCount();
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) choiceIndex_ = (choiceIndex_ + n - 1) % n;
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) choiceIndex_ = (choiceIndex_ + 1) % n;
        if (confirm()) { wasChoice_ = false; it.choose(choiceIndex_); }
        else if (cancel()) { wasChoice_ = false; it.choose(n - 1); }   // batal = opsi terakhir
    }
}

// Potret pembicara (assets/faces, dibuat tools/build_assets.py build_faces). Pemetaan nama -> gambar
// hanya urusan tampilan, jadi tinggal di GUI; nama yang tidak ada di daftar tampil tanpa potret.
static const char* portraitFor(const std::string& who) {
    static const char* table[][2] = {
        {"Timun Mas", "timun_mas"},       {"Gadis di Dermaga", "timun_mas"}, {"Mbok Srini", "mbok_srini"},
        {"Perempuan Tua", "mbok_srini"},  {"Lelaki Tua", "lelaki_tua"},      {"Empu Karta", "empu_karta"},
        {"Pandai Besi", "empu_karta"},    {"Pedagang Keliling", "pedagang"}, {"Pemuda Desa", "pemuda_desa"},
        {"Penumbuk Lesung", "penumbuk_lesung"}, {"Buto Ijo", "buto_ijo"},   {"Memedi Sawah", "memedi_sawah"},
    };
    for (auto& row : table)
        if (who == row[0]) return row[1];
    return nullptr;
}

void DialogView::draw(const Interpreter& it, double time) {
    if (it.state() != Interpreter::WaitMessage && it.state() != Interpreter::WaitChoice) return;
    Rectangle box{20, 258, 600, 92};
    ui::window(box, 0.92f);
    // teks terakhir tetap tampil saat memilih
    std::string text = shownText_;
    int visible = static_cast<int>(typed_);
    if (it.state() == Interpreter::WaitMessage) text = text.substr(0, visible < 0 ? 0 : static_cast<size_t>(visible));
    if (!it.speaker().empty() && it.state() == Interpreter::WaitMessage) {
        float w = ui::measure(it.speaker()) + 24;
        Rectangle nb{box.x + 8, box.y - 24, w, 24};
        ui::window(nb, 0.95f);
        ui::text(it.speaker(), nb.x + 12, nb.y + 4, ui::AMBER);
    }
    // potret di kiri kotak (seperti faceset RPG Maker); teks bergeser ke kanan
    float textX = box.x + 18;
    if (const char* face = portraitFor(it.speaker())) {
        Texture2D t = Assets::get().texture(std::string("assets/faces/") + face + ".png");
        if (t.id != 0) {
            DrawTexture(t, static_cast<int>(box.x + 4), static_cast<int>(box.y + 4), WHITE);
            textX = box.x + 4 + t.width + 12;
        }
    }
    std::vector<std::string> lines = ui::wrap(text, box.x + box.width - textX - 18);
    float y = box.y + 14;
    for (size_t i = 0; i < lines.size() && i < 4; ++i, y += 20) ui::text(lines[i], textX, y, ui::BONE);
    if (it.state() == Interpreter::WaitMessage && visible >= static_cast<int>(shownText_.size())) {
        float bob = static_cast<float>(std::sin(time * 6) * 2);
        float cx = box.x + box.width - 20, cy = box.y + box.height - 12 + bob;
        DrawTriangle({cx - 5, cy - 4}, {cx, cy + 2}, {cx + 5, cy - 4}, ui::AMBER);
    }
    if (it.state() == Interpreter::WaitChoice) {
        int n = it.optionCount();
        float w = 0;
        for (int i = 0; i < n; ++i) w = std::fmax(w, ui::measure(it.option(i)));
        Rectangle cb{box.x + box.width - w - 52, box.y - 12 - n * 22.0f, w + 52, n * 22.0f + 10};
        ui::window(cb, 0.95f);
        for (int i = 0; i < n; ++i) {
            float oy = cb.y + 6 + i * 22;
            ui::text(it.option(i), cb.x + 26, oy, i == choiceIndex_ ? ui::AMBER : ui::BONE);
            if (i == choiceIndex_) ui::cursor(cb.x + 12, oy + 8, time);
        }
    }
}
