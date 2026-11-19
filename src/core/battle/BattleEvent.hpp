#pragma once
// ============================================================================
// BattleEvent — satu kejadian battle yang dihasilkan logika core dan
// dimasukkan ke Queue<BattleEvent> (FIFO). GUI mengambilnya satu per satu
// untuk dianimasikan; mode console mencetaknya. Urutan kejadian = urutan
// enqueue, sehingga animasi selalu sesuai urutan logika.
// ============================================================================

#include <string>

struct BattleEvent {
    enum Kind {
        Message,        // teks log
        TurnStart,      // src mendapat giliran
        UseSkill,       // src memakai skill ke dst (dst = -1: semua)
        ChantStart,     // src mulai merapal skill
        ChantBroken,    // rapal dst buyar
        Damage,         // dst menerima value damage
        Heal,           // dst pulih value
        Miss,           // serangan ke dst meleset / dielakkan
        Counter,        // dst menangkis & membalas src
        StatusAdd,      // dst mendapat status
        StatusRemove,   // status dst hilang
        Delay,          // giliran dst didorong mundur (value = persen)
        Death,          // dst tumbang
        Victory,        // value = sukma
        Defeat,
        Escaped,
        EscapeFail,
    };
    Kind kind = Message;
    int  src = -1;
    int  dst = -1;
    int  value = 0;
    int  skill = 0;
    int  status = 0;
    bool critical = false;
    std::string text;
};
