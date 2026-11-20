#include "Settings.hpp"

#include <cstdio>
#include <string>
#include "Assets.hpp"
#include "TitleView.hpp"
#include "core/util/Json.hpp"

namespace settings {

static std::string path() {
    std::string slot = TitleView::slotPath(1);              // folder saves yang sama dengan slot simpan
    return slot.substr(0, slot.rfind('/')) + "/settings.json";
}

static void save() {
    std::FILE* f = std::fopen(path().c_str(), "wb");
    if (f == nullptr) return;
    std::fprintf(f, "{\"lang\":\"%s\"}\n", lang::language() == LANG_ID ? "id" : "en");
    std::fclose(f);
}

void load() {
    lang::clear();
    std::string err;
    if (!lang::load(Assets::get().path("assets/lang/en.json"), &err))
        std::fprintf(stderr, "Kamus bahasa Inggris tidak termuat: %s\n", err.c_str());
    Language l = LANG_EN;                                    // bawaan: English
    if (JsonValue* v = JsonValue::parseFile(path(), nullptr)) {
        if (v->getString("lang", "en") == "id") l = LANG_ID;
        delete v;
    }
    lang::setLanguage(l);
}

void setLanguage(Language l) {
    lang::setLanguage(l);
    save();
}

void toggleLanguage() { setLanguage(lang::language() == LANG_EN ? LANG_ID : LANG_EN); }

}  // namespace settings
