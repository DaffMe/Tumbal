#include "Lang.hpp"

#include <cstdarg>
#include <cstdio>
#include "Json.hpp"

namespace lang {
namespace {

constexpr int CAPACITY = 4096;    // pangkat dua; kamus jauh lebih kecil agar probing tetap pendek

struct Entry {
    bool        used = false;
    std::string key;
    std::string value;
};

Entry    g_table[CAPACITY];
int      g_size = 0;
Language g_lang = LANG_EN;

unsigned hashOf(const std::string& s) {
    unsigned h = 2166136261u;                 // FNV-1a
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}

const Entry* find(const std::string& key) {
    unsigned i = hashOf(key) & (CAPACITY - 1);
    for (int probe = 0; probe < CAPACITY; ++probe, i = (i + 1) & (CAPACITY - 1)) {
        const Entry& e = g_table[i];
        if (!e.used) return nullptr;
        if (e.key == key) return &e;
    }
    return nullptr;
}

void put(const std::string& key, const std::string& value) {
    unsigned i = hashOf(key) & (CAPACITY - 1);
    for (int probe = 0; probe < CAPACITY; ++probe, i = (i + 1) & (CAPACITY - 1)) {
        Entry& e = g_table[i];
        if (!e.used) {
            if (g_size >= CAPACITY * 3 / 4) return;   // tabel penuh: abaikan (teks Indonesia dipakai)
            e.used = true;
            e.key = key;
            e.value = value;
            ++g_size;
            return;
        }
        if (e.key == key) { e.value = value; return; }
    }
}

}  // namespace

void setLanguage(Language l) { g_lang = l; }
Language language() { return g_lang; }
const char* languageName(Language l) { return l == LANG_ID ? "Bahasa Indonesia" : "English"; }

void clear() {
    for (Entry& e : g_table) e = Entry();
    g_size = 0;
}

bool load(const std::string& path, std::string* error) {
    JsonValue* root = JsonValue::parseFile(path, error);
    if (root == nullptr) return false;
    for (const JsonValue* c = root->first(); c != nullptr; c = c->next())
        if (!c->key().empty() && c->key()[0] != '_') put(c->key(), c->asString());   // "_..." = catatan
    delete root;
    return true;
}

int size() { return g_size; }
bool has(const std::string& id) { return find(id) != nullptr; }

std::string tr(const std::string& id) {
    if (g_lang == LANG_ID || id.empty()) return id;
    const Entry* e = find(id);
    return (e != nullptr && !e->value.empty()) ? e->value : id;
}

std::string trf(const char* idFormat, ...) {
    std::string fmt = tr(idFormat);
    char buf[512];
    va_list ap;
    va_start(ap, idFormat);
    std::vsnprintf(buf, sizeof buf, fmt.c_str(), ap);
    va_end(ap);
    return buf;
}

}  // namespace lang
