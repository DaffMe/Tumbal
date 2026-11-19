#pragma once
// ============================================================================
// TitleView — layar judul dan pembuatan karakter (seperti awal Black Souls):
//   Mulai Baru -> nama -> kelas (3) -> hadiah awal (4)
//   Lanjutkan  -> pilih slot simpanan
// Hanya mengumpulkan pilihan; Game::newGame / loadFrom yang menjalankannya.
// ============================================================================

#include <string>

class TitleView {
public:
    enum Result { None, NewGame, LoadSlot, Quit };

    void open();
    Result update(float dt);
    void draw(double time);

    const std::string& heroName() const { return name_; }
    int classId() const { return classIndex_; }
    int gift() const { return giftIndex_; }
    int slot() const { return slotIndex_ + 1; }

    static std::string slotPath(int slot);   // assets root /saves/slotN.json

private:
    enum Page { Root, Name, Class, Gift, Slots };
    Page  page_ = Root;
    int   index_ = 0;
    std::string name_ = "Tumbal";
    int   classIndex_ = 0;
    int   giftIndex_ = 0;
    int   slotIndex_ = 0;
    std::string slotInfo_[3];
    float fadeIn_ = 1.0f;

    void refreshSlots();
};
