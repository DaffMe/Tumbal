#pragma once
// ============================================================================
// Game — perangkai seluruh logika permainan (tanpa raylib).
//
// Menyatukan World (Graph peta, M2), Field (eksplorasi), GameState (pemain),
// EventLibrary + Interpreter (M3), dan menyiapkan BattleSystem (M1).
// GUI dan mode console sama-sama hanya:
//   1) mengirim Input setiap frame ke update(),
//   2) mengambil "permintaan" (battle, pindah peta) lalu menampilkannya,
//   3) melapor balik (endBattle, finishTransfer, dialog: resume/choose).
//
// Loop Souls yang diatur di sini:
//   - Tungku: istirahat → pulih, Ramuan Jamu terisi, musuh semua peta hidup lagi.
//   - Mati  : sukma tercecer di lokasi kematian, bangkit di Tungku terakhir.
//   - Ceceran diambil dengan menginjaknya; mati lagi sebelum itu = hilang.
// ============================================================================

#include <string>
#include "../adt/Queue.hpp"
#include "../battle/BattleSystem.hpp"
#include "../event/Interpreter.hpp"
#include "../world/Field.hpp"
#include "GameState.hpp"

struct GameInput {
    int  dx = 0, dy = 0;
    bool run = false;
    bool action = false;      // tombol aksi ditekan frame ini
};

class Game : public EventHost {
public:
    Game();
    ~Game() override = default;
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Muat dunia (world.json) dan semua berkas event di daftar `eventFiles`.
    bool load(const std::string& worldJson, const std::string* eventFiles, int eventFileCount, std::string* error);
    void newGame(const std::string& heroName, int classId);

    // Satu langkah permainan di peta. Dibekukan saat dialog/battle/transfer.
    void update(float dt, const GameInput& in);

    // ---- permintaan untuk lapisan tampilan
    bool battlePending() const { return battlePending_; }
    int  battleTroop() const { return battleTroop_; }
    int  battleInitiative() const { return battleInit_; }
    // Siapkan BattleSystem untuk battle yang diminta (party = pemain).
    void setupBattle(BattleSystem& bs, std::uint32_t seed);
    // Battle selesai: terapkan hasil (sukma, musuh hilang, atau mati).
    void endBattle(const BattleSystem& bs);

    bool transferPending() const { return transferPending_; }
    // Dipanggil saat layar sudah gelap: pindahkan pemain.
    void finishTransfer();

    // ---- dialog (diteruskan ke Interpreter)
    Interpreter& interpreter() { return interp_; }
    bool inDialog() const;

    // ---- menu
    std::string useItem(int itemId);

    // ---- menu Tungku (terbuka setelah beristirahat)
    bool tungkuMenuOpen() const { return tungkuMenu_; }
    void closeTungkuMenu() { tungkuMenu_ = false; }
    // Naik level dengan sukma. Mengembalikan teks hasil.
    std::string levelUp();
    // M2 subprogram: daftar tujuan warp (Tungku menyala & terjangkau).
    int warpTargets(int* outMap, int* outDepth, int max);
    // M2 subprogram: warp ke Tungku di peta mapId (lewat transfer).
    bool warpTo(int mapId);

    // ---- notifikasi singkat (Queue): loot, sukma, Tungku, dll.
    bool popNotice(std::string& out);

    // ---- akses state
    World&     world() { return world_; }
    Field&     field() { return field_; }
    const Field& field() const { return field_; }
    GameState& state() { return state_; }
    const GameState& state() const { return state_; }
    bool justEnteredMap() const { return enteredMap_; }
    bool hidden(const MapObject& o) const {
        return state_.hidden(o.props.getString("hideIf"), o.props.getString("showIf"));
    }

    // EventHost (dipakai Interpreter)
    void hostStartBattle(int troopId) override;
    void hostTransfer(int mapId, const std::string& spawn) override;
    void hostUnlock(int a, int b) override;
    void hostHeal() override;

private:
    World        world_;
    Field        field_;
    GameState    state_;
    EventLibrary events_;
    Interpreter  interp_;
    Queue<std::string> notices_;

    bool battlePending_ = false;
    int  battleTroop_ = 0;
    int  battleInit_ = 0;
    int  battleEnemyIndex_ = -1;     // -1 = battle dari event (bukan musuh peta)

    bool transferPending_ = false;
    int  transferMap_ = 0, transferX_ = 0, transferY_ = 0, transferDir_ = 0;
    std::string transferSpawn_;
    bool respawnAfterTransfer_ = false;
    bool enteredMap_ = false;
    bool tungkuMenu_ = false;

    void notice(const std::string& s);
    void interact();
    bool tryLoot(const MapObject* o);
    void restAt(const MapObject& tungku);
    void runAutoEvents();
    void checkBloodstain();
    void requestTransfer(int map, int x, int y, int dir, bool respawn);
};
