#pragma once
// ============================================================================
// Interpreter — Mekanik M3: menjalankan skrip event seperti interpreter
// event RPG Maker (Game_Interpreter) yang menggerakkan cerita Black Souls.
//
// Struktur data:
//   - Stack<Frame> : setiap frame = posisi baris (pointer Command) di satu
//     daftar perintah. Masuk ke cabang pilihan / if / CALL skrip lain =
//     PUSH frame baru; daftar habis = POP, lanjut ke frame di bawahnya.
//     Karena itu cabang bersarang & skrip yang memanggil skrip lain bisa
//     kembali ke baris pemanggil dengan benar (LIFO).
//   - Queue<std::string> : notifikasi singkat ("Didapat: Ramuan Jamu x1")
//     yang ditampilkan GUI satu per satu tanpa menghentikan permainan.
//
// Perintah yang butuh pemain (teks, pilihan, battle, pindah peta) membuat
// interpreter MENUNGGU sampai lapisan atas memanggil resume()/choose().
// ============================================================================

#include <string>
#include "../adt/Queue.hpp"
#include "../adt/Stack.hpp"
#include "Script.hpp"

class GameState;

// Efek yang harus dikerjakan "dunia" (dipenuhi oleh kelas Game).
class EventHost {
public:
    virtual ~EventHost() = default;
    virtual void hostStartBattle(int troopId) = 0;
    virtual void hostTransfer(int mapId, const std::string& spawn) = 0;
    virtual void hostUnlock(int a, int b) = 0;
    virtual void hostHeal() = 0;
    // Buka menu pedagang (kind "dagang") atau pandai besi ("tempa"). Bawaan: abaikan.
    virtual void hostOpenShop(const std::string& kind) { (void)kind; }
};

class Interpreter {
public:
    enum State { Idle, Running, WaitMessage, WaitChoice, WaitBattle, WaitTransfer };

    Interpreter(GameState& gs, const EventLibrary& lib, EventHost& host) : gs_(gs), lib_(lib), host_(host) {}

    // M3 subprogram: mulai skrip bernama `name`. false bila tidak ada / sedang sibuk.
    bool runEvent(const std::string& name);
    // M3 subprogram: jalankan perintah sampai harus menunggu atau selesai.
    void update();
    // Pemain menutup kotak pesan / battle selesai / tiba di peta.
    void resume();
    // M3 subprogram: pemain memilih opsi i → masuk cabang (push frame).
    void choose(int i);
    // Batalkan skrip yang sedang berjalan (mis. pemain mati di battle event).
    void abort() { stack_.clear(); choice_ = nullptr; state_ = Idle; }

    State state() const { return state_; }
    bool  busy() const { return state_ != Idle; }
    const std::string& speaker() const { return who_; }
    const std::string& speakerKey() const { return whoKey_; }   // nama sumber (tak diterjemahkan), untuk potret
    const std::string& message() const { return text_; }
    int   optionCount() const { return choice_ ? choice_->childCount : 0; }
    std::string option(int i) const;   // teks opsi dalam bahasa aktif
    int   battleTroop() const { return battleTroop_; }
    int   depth() const { return static_cast<int>(stack_.size()); }

    Queue<std::string>& notices() { return notices_; }

private:
    struct Frame {
        const Command* pc;
    };

    GameState&          gs_;
    const EventLibrary& lib_;
    EventHost&          host_;
    Stack<Frame>        stack_;
    Queue<std::string>  notices_;
    State               state_ = Idle;
    std::string         who_, text_, whoKey_;
    const Command*      choice_ = nullptr;
    int                 battleTroop_ = 0;
    int                 steps_ = 0;

    void pushList(const Command* head, const char* why);
    void execute(const Command* c);   // M3 subprogram
};
