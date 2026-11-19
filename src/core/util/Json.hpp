#pragma once
// ============================================================================
// Json — parser JSON kecil buatan sendiri untuk membaca berkas peta Tiled
// (.tmj) dan data dunia (world.json).
//
// Representasi: pohon node dengan pointer "anak pertama" + "saudara
// berikutnya" (first-child / next-sibling). Object dan array sama-sama
// menyimpan anaknya sebagai linked list; anak object punya `key`.
// Tidak memakai std::vector / std::map (aturan src/core).
//
// Untuk array besar (data tile 100x100), telusuri lewat `first()` dan
// `next` agar O(n), bukan `at(i)` berulang.
// ============================================================================

#include <string>

class JsonValue {
public:
    enum Type { Null, Bool, Number, String, Array, Object };

    JsonValue() = default;
    ~JsonValue();
    JsonValue(const JsonValue&) = delete;
    JsonValue& operator=(const JsonValue&) = delete;

    // Parse teks JSON. Mengembalikan nullptr bila gagal; pesan di errorOut.
    static JsonValue* parse(const std::string& text, std::string* errorOut = nullptr);
    // Baca berkas lalu parse. nullptr bila berkas tidak ada / rusak.
    static JsonValue* parseFile(const std::string& path, std::string* errorOut = nullptr);

    Type type() const { return type_; }
    bool isNull() const { return type_ == Null; }
    bool isArray() const { return type_ == Array; }
    bool isObject() const { return type_ == Object; }

    // Akses object: anak dengan key tertentu (nullptr jika tidak ada).
    const JsonValue* get(const char* key) const;
    // Akses array: elemen ke-i (O(i)).
    const JsonValue* at(int i) const;
    int size() const { return count_; }

    // Iterasi anak: for (auto* c = v->first(); c; c = c->next()) ...
    const JsonValue* first() const { return firstChild_; }
    const JsonValue* next() const { return next_; }
    const std::string& key() const { return key_; }

    int         asInt(int def = 0) const;
    double      asNumber(double def = 0.0) const;
    bool        asBool(bool def = false) const;
    std::string asString(const std::string& def = "") const;

    // Pintasan: nilai anak `key` dengan default.
    int         getInt(const char* key, int def = 0) const;
    double      getNumber(const char* key, double def = 0.0) const;
    bool        getBool(const char* key, bool def = false) const;
    std::string getString(const char* key, const std::string& def = "") const;

private:
    friend class JsonParser;
    Type        type_ = Null;
    bool        bool_ = false;
    double      num_ = 0.0;
    std::string str_;
    std::string key_;
    JsonValue*  firstChild_ = nullptr;
    JsonValue*  lastChild_ = nullptr;
    JsonValue*  next_ = nullptr;
    int         count_ = 0;

    void append(JsonValue* child);
};
