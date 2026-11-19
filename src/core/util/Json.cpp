#include "Json.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// ----------------------------------------------------------------- JsonValue

JsonValue::~JsonValue() {
    // hapus anak secara iteratif (linked list saudara)
    JsonValue* c = firstChild_;
    while (c != nullptr) {
        JsonValue* n = c->next_;
        delete c;
        c = n;
    }
}

void JsonValue::append(JsonValue* child) {
    if (lastChild_ == nullptr) firstChild_ = child;
    else lastChild_->next_ = child;
    lastChild_ = child;
    ++count_;
}

const JsonValue* JsonValue::get(const char* key) const {
    if (type_ != Object) return nullptr;
    for (JsonValue* c = firstChild_; c != nullptr; c = c->next_)
        if (c->key_ == key) return c;
    return nullptr;
}

const JsonValue* JsonValue::at(int i) const {
    if (type_ != Array || i < 0) return nullptr;
    JsonValue* c = firstChild_;
    while (c != nullptr && i-- > 0) c = c->next_;
    return c;
}

int JsonValue::asInt(int def) const {
    if (type_ == Number) return static_cast<int>(num_ < 0 ? num_ - 0.5 : num_ + 0.5);
    if (type_ == Bool) return bool_ ? 1 : 0;
    return def;
}
double JsonValue::asNumber(double def) const { return type_ == Number ? num_ : def; }
bool JsonValue::asBool(bool def) const {
    if (type_ == Bool) return bool_;
    if (type_ == Number) return num_ != 0.0;
    return def;
}
std::string JsonValue::asString(const std::string& def) const { return type_ == String ? str_ : def; }

int JsonValue::getInt(const char* key, int def) const {
    const JsonValue* v = get(key);
    return v ? v->asInt(def) : def;
}
double JsonValue::getNumber(const char* key, double def) const {
    const JsonValue* v = get(key);
    return v ? v->asNumber(def) : def;
}
bool JsonValue::getBool(const char* key, bool def) const {
    const JsonValue* v = get(key);
    return v ? v->asBool(def) : def;
}
std::string JsonValue::getString(const char* key, const std::string& def) const {
    const JsonValue* v = get(key);
    return v ? v->asString(def) : def;
}

// ---------------------------------------------------------------- JsonParser

class JsonParser {
public:
    explicit JsonParser(const std::string& t) : s_(t.c_str()), n_(t.size()) {}

    JsonValue* run(std::string* err) {
        skipWs();
        JsonValue* v = value();
        skipWs();
        if (v != nullptr && p_ != n_) fail("sisa teks setelah nilai utama");
        if (!error_.empty()) {
            delete v;
            if (err) *err = error_;
            return nullptr;
        }
        return v;
    }

private:
    const char* s_;
    std::size_t n_;
    std::size_t p_ = 0;
    std::string error_;

    void fail(const char* msg) {
        if (!error_.empty()) return;
        char buf[160];
        std::snprintf(buf, sizeof buf, "JSON error di posisi %zu: %s", p_, msg);
        error_ = buf;
    }
    bool ok() const { return error_.empty(); }
    char peek() const { return p_ < n_ ? s_[p_] : '\0'; }

    void skipWs() {
        while (p_ < n_ && (s_[p_] == ' ' || s_[p_] == '\n' || s_[p_] == '\r' || s_[p_] == '\t')) ++p_;
    }

    bool literal(const char* word) {
        std::size_t len = std::strlen(word);
        if (p_ + len <= n_ && std::strncmp(s_ + p_, word, len) == 0) { p_ += len; return true; }
        return false;
    }

    JsonValue* value() {
        if (!ok()) return nullptr;
        char c = peek();
        JsonValue* v = new JsonValue();
        if (c == '{') { object(v); }
        else if (c == '[') { array(v); }
        else if (c == '"') { v->type_ = JsonValue::String; v->str_ = string(); }
        else if (c == '-' || (c >= '0' && c <= '9')) { v->type_ = JsonValue::Number; v->num_ = number(); }
        else if (literal("true")) { v->type_ = JsonValue::Bool; v->bool_ = true; }
        else if (literal("false")) { v->type_ = JsonValue::Bool; v->bool_ = false; }
        else if (literal("null")) { v->type_ = JsonValue::Null; }
        else fail("nilai tidak dikenal");
        return v;
    }

    void object(JsonValue* v) {
        v->type_ = JsonValue::Object;
        ++p_;  // {
        skipWs();
        if (peek() == '}') { ++p_; return; }
        while (ok()) {
            skipWs();
            if (peek() != '"') { fail("key object harus string"); return; }
            std::string k = string();
            skipWs();
            if (peek() != ':') { fail("':' hilang"); return; }
            ++p_;
            skipWs();
            JsonValue* child = value();
            if (child == nullptr) return;
            child->key_ = k;
            v->append(child);
            skipWs();
            if (peek() == ',') { ++p_; continue; }
            if (peek() == '}') { ++p_; return; }
            fail("',' atau '}' hilang");
        }
    }

    void array(JsonValue* v) {
        v->type_ = JsonValue::Array;
        ++p_;  // [
        skipWs();
        if (peek() == ']') { ++p_; return; }
        while (ok()) {
            skipWs();
            JsonValue* child = value();
            if (child == nullptr) return;
            v->append(child);
            skipWs();
            if (peek() == ',') { ++p_; continue; }
            if (peek() == ']') { ++p_; return; }
            fail("',' atau ']' hilang");
        }
    }

    static void putUtf8(std::string& out, unsigned cp) {
        if (cp < 0x80) out += static_cast<char>(cp);
        else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    std::string string() {
        std::string out;
        ++p_;  // "
        while (p_ < n_ && s_[p_] != '"') {
            char c = s_[p_++];
            if (c != '\\') { out += c; continue; }
            if (p_ >= n_) break;
            char e = s_[p_++];
            switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    if (p_ + 4 > n_) { fail("escape \\u tidak lengkap"); return out; }
                    char hex[5] = {s_[p_], s_[p_ + 1], s_[p_ + 2], s_[p_ + 3], 0};
                    p_ += 4;
                    putUtf8(out, static_cast<unsigned>(std::strtoul(hex, nullptr, 16)));
                    break;
                }
                default: out += e; break;   // \" \\ \/
            }
        }
        if (p_ >= n_) { fail("string tidak ditutup"); return out; }
        ++p_;  // "
        return out;
    }

    double number() {
        char* end = nullptr;
        double d = std::strtod(s_ + p_, &end);
        if (end == s_ + p_) { fail("angka tidak valid"); return 0.0; }
        p_ = static_cast<std::size_t>(end - s_);
        return d;
    }
};

JsonValue* JsonValue::parse(const std::string& text, std::string* errorOut) {
    JsonParser p(text);
    return p.run(errorOut);
}

JsonValue* JsonValue::parseFile(const std::string& path, std::string* errorOut) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) {
        if (errorOut) *errorOut = "tidak bisa membuka " + path;
        return nullptr;
    }
    std::string text;
    char buf[8192];
    std::size_t got;
    while ((got = std::fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, got);
    std::fclose(f);
    return parse(text, errorOut);
}
