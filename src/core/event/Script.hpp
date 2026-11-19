#pragma once
// ============================================================================
// Script — skrip event (cutscene, dialog NPC, peti) seperti daftar perintah
// event RPG Maker yang dipakai Black Souls.
//
// Satu skrip = linked list Command (field `next`). Perintah bercabang
// (pilihan, if) menyimpan daftar anak di `child[]`, sehingga keseluruhan
// skrip membentuk pohon. Interpreter menelusurinya memakai Stack frame.
//
// Skrip dimuat dari JSON (assets/events/*.json):
//   { "nama_event": [ {"cmd": "text", "who": "Mbok Srini", "text": "..."},
//                     {"cmd": "choice", "options": ["Ya", "Tidak"],
//                      "branches": [[...], [...]]}, ... ] }
// ============================================================================

#include <string>

class JsonValue;

enum CommandOp {
    CMD_TEXT = 0,    // s1 = pembicara, s2 = teks                    (menunggu)
    CMD_CHOICE,      // opt[] + child[]                               (menunggu)
    CMD_ITEM,        // n1 = id barang, n2 = jumlah (negatif = ambil)
    CMD_SUKMA,       // n1 = jumlah
    CMD_FLAG,        // s1 = nama, n1 = nilai
    CMD_IF_FLAG,     // s1 = nama, n1 = nilai pembanding; child[0] = ya, child[1] = tidak
    CMD_IF_ITEM,     // n1 = id barang, n2 = jumlah minimal; child[0]/child[1]
    CMD_BATTLE,      // n1 = troop                                     (menunggu)
    CMD_CALL,        // s1 = nama skrip lain
    CMD_HEAL,        // pulihkan pemain
    CMD_UNLOCK,      // n1, n2 = id peta: buka jalan pintas
    CMD_TRANSFER,    // n1 = peta, s1 = spawn                          (menunggu)
    CMD_NOTICE,      // s1 = teks notifikasi singkat (tidak menunggu)
    CMD_END,         // hentikan skrip
    CMD_COUNT
};

struct Command {
    static constexpr int MAX_CHILD = 4;
    int         op = CMD_END;
    std::string s1, s2;
    int         n1 = 0, n2 = 0;
    std::string opt[MAX_CHILD];
    Command*    child[MAX_CHILD] = {};
    int         childCount = 0;
    Command*    next = nullptr;
};

// Hapus satu daftar perintah beserta semua cabangnya.
void freeCommands(Command* head);

struct Script {
    std::string name;
    Command*    head = nullptr;
    Script*     next = nullptr;
};

// Kumpulan skrip (linked list), dicari berdasarkan nama.
class EventLibrary {
public:
    EventLibrary() = default;
    ~EventLibrary();
    EventLibrary(const EventLibrary&) = delete;
    EventLibrary& operator=(const EventLibrary&) = delete;

    bool loadFile(const std::string& path, std::string* error = nullptr);
    bool loadText(const std::string& json, std::string* error = nullptr);
    const Script* find(const std::string& name) const;
    int count() const { return count_; }

private:
    Script* head_ = nullptr;
    int     count_ = 0;
    bool    loadRoot(const JsonValue* root, std::string* error);
};
