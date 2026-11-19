#pragma once
// ============================================================================
// GameState — keadaan pemain yang bertahan di antara peta & battle:
// tokoh utama (Battler), sukma, inventaris, flag cerita, Tungku terakhir,
// dan "sukma tercecer" (bloodstain Black Souls).
//
// Inventaris sengaja berupa ARRAY jumlah per id barang (bukan linked list
// "gear", contoh terlarang di PDF). Flag cerita = array pasangan nama/nilai.
// ============================================================================

#include <string>
#include "../battle/BattleSystem.hpp"

struct Bloodstain {
    bool active = false;
    int  map = 0, x = 0, y = 0;
    int  sukma = 0;
};

class GameState : public ItemBag {
public:
    static constexpr int MAX_FLAGS = 64;

    void newGame(const std::string& name, int classId);

    Battler& hero() { return hero_; }
    const Battler& hero() const { return hero_; }
    int classId() const { return classId_; }
    int level() const { return level_; }

    // ---- sukma
    int  sukma() const { return sukma_; }
    void addSukma(int n) { sukma_ += n; if (sukma_ < 0) sukma_ = 0; }

    // ---- inventaris
    int  count(int itemId) const override;
    void addItem(int itemId, int n = 1);
    bool removeItem(int itemId, int n = 1) override;
    int  jamuMax() const { return jamuMax_; }
    // Pakai barang di luar battle. Mengembalikan teks hasil ("" bila gagal).
    std::string useItemInField(int itemId);

    // ---- flag cerita
    int  flag(const std::string& name) const;
    void setFlag(const std::string& name, int value);

    // ---- Tungku & kematian (loop Souls)
    void setLastTungku(int map, int x, int y) { tungkuMap_ = map; tungkuX_ = x; tungkuY_ = y; }
    int  tungkuMap() const { return tungkuMap_; }
    int  tungkuX() const { return tungkuX_; }
    int  tungkuY() const { return tungkuY_; }
    // Istirahat: HP/MP penuh, status bersih, Ramuan Jamu terisi ulang.
    void rest();
    // Mati di (map,x,y): sukma yang dibawa tercecer di sana. Ceceran lama
    // (bila belum diambil) hilang selamanya. Mengembalikan sukma yang hilang.
    int  die(int map, int x, int y);
    // Ambil kembali ceceran bila pemain berdiri di atasnya. Mengembalikan sukma yang didapat.
    int  retrieveBloodstain(int map, int x, int y);
    const Bloodstain& bloodstain() const { return blood_; }
    int  deaths() const { return deaths_; }

private:
    Battler hero_;
    int     classId_ = 0;
    int     level_ = 1;
    int     sukma_ = 0;
    int     items_[IT_COUNT] = {};
    int     jamuMax_ = 3;
    std::string flagName_[MAX_FLAGS];
    int     flagValue_[MAX_FLAGS] = {};
    int     flagCount_ = 0;
    int     tungkuMap_ = 0, tungkuX_ = 0, tungkuY_ = 0;
    Bloodstain blood_;
    int     deaths_ = 0;
};
