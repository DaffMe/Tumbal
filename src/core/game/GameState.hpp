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

class JsonValue;

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

    // ---- pendamping (Timun Mas): bergabung lewat cerita, dipanggil di Tungku
    bool companionJoined() const { return companionJoined_; }
    bool companionActive() const { return companionJoined_ && companionActive_; }
    void joinCompanion();
    void setCompanionActive(bool on) { companionActive_ = on; }
    Battler& companion() { return companion_; }
    const Battler& companion() const { return companion_; }
    int  companionLevel() const { return companionLevel_; }
    int  companionLevelUpCost() const;
    bool companionLevelUp();
    // Isi array party untuk battle (pemain + pendamping bila aktif). Mengembalikan jumlahnya.
    int  party(Battler* out) const;
    // Salin hasil battle kembali (HP/MP/status) dari array party.
    void applyParty(const Battler* in, int n);

    // ---- simpan / muat (teks JSON)
    std::string toJson() const;
    bool fromJson(const JsonValue* v);

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

    // Objek peta dengan property hideIf=<flag> hilang bila flag != 0,
    // showIf=<flag> baru muncul bila flag != 0 (mis. boss yang sudah kalah).
    bool hidden(const std::string& hideIf, const std::string& showIf) const {
        return (!hideIf.empty() && flag(hideIf) != 0) || (!showIf.empty() && flag(showIf) == 0);
    }

    // ---- Tungku & kematian (loop Souls)
    void setLastTungku(int map, int x, int y) { tungkuMap_ = map; tungkuX_ = x; tungkuY_ = y; }
    int  tungkuMap() const { return tungkuMap_; }
    int  tungkuX() const { return tungkuX_; }
    int  tungkuY() const { return tungkuY_; }
    // Istirahat: HP/MP penuh, status bersih, Ramuan Jamu terisi ulang.
    void rest();

    // ---- naik level (tukar sukma, seperti BS1)
    int  levelUpCost() const;           // sukma untuk naik ke level berikutnya
    bool canLevelUp() const { return sukma_ >= levelUpCost(); }
    bool levelUp();                     // false bila sukma kurang
    // Mati di (map,x,y): sukma yang dibawa tercecer di sana. Ceceran lama
    // (bila belum diambil) hilang selamanya. Mengembalikan sukma yang hilang.
    int  die(int map, int x, int y);
    // Ambil kembali ceceran bila pemain berdiri di atasnya. Mengembalikan sukma yang didapat.
    int  retrieveBloodstain(int map, int x, int y);
    const Bloodstain& bloodstain() const { return blood_; }
    int  deaths() const { return deaths_; }

    // ---- pandai besi: tempa senjata kelas +0..+3 (sukma + Bijih Besi)
    enum ForgeResult { FORGE_OK = 0, FORGE_MAX, FORGE_NO_SUKMA, FORGE_NO_ORE };
    int  weaponLevel() const { return weaponLevel_; }
    ForgeResult canForge() const;
    ForgeResult forge();
    // Stat tokoh utama = stat level + bonus senjata.
    Stats heroStats(int level) const;

    // ---- pedagang: beli dengan sukma; sebagian barang stoknya terbatas
    enum BuyResult { BUY_OK = 0, BUY_SOLD_OUT, BUY_NO_SUKMA, BUY_FULL };
    int  shopStock(int entry) const;      // sisa stok, -1 = tak terbatas
    BuyResult canBuy(int entry) const;
    BuyResult buy(int entry);

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
    Battler companion_;
    bool    companionJoined_ = false;
    bool    companionActive_ = false;
    int     companionLevel_ = 1;
    int     weaponLevel_ = 0;
    static constexpr int MAX_SHOP = 16;
    int     shopBought_[MAX_SHOP] = {};

    void refreshHeroStats();   // pasang ulang stat (level + senjata) tanpa mengubah HP/MP
};
