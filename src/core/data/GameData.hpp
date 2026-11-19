#pragma once
// ============================================================================
// GameData — tabel data permainan (jurus, status, musuh, troop, kelas).
//
// Angka mengikuti skala Black Souls 1 (RPG Maker VX Ace): HP ratusan,
// serangan fisik `atk*4 - def*2`, ilmu `dasar + mag*5 - res*2`, variance 20%.
// Data disimpan sebagai array statis (bukan container) dan diakses lewat id.
// ============================================================================

#include <string>

// ---------------------------------------------------------------- parameter
struct Stats {
    int mhp = 1, mmp = 0, atk = 1, def = 1, mag = 1, res = 1, agi = 1, luk = 1;
};

enum Element { EL_NONE = 0, EL_FISIK, EL_API, EL_AIR, EL_ANGIN, EL_TANAH, EL_GAIB, EL_COUNT };
const char* elementName(int el);

// ---------------------------------------------------------------- status
enum StatusId {
    ST_NONE = 0,
    ST_RACUN,       // -5% HP tiap giliran
    ST_BERDARAH,    // -5% HP tiap giliran, fisik masuk x1.5
    ST_TERBAKAR,    // -8% HP tiap giliran, 2 giliran
    ST_LUMPUH,      // kehilangan giliran
    ST_TIDUR,       // kehilangan giliran, bangun saat kena damage
    ST_BISU,        // tidak bisa memakai ilmu
    ST_TAKUT,       // AGI x0.5
    ST_RAPUH,       // damage diterima x1.5
    ST_CEPAT,       // AGI x2
    ST_LAMBAT,      // AGI x0.5
    ST_BERTAHAN,    // damage diterima x0.5 sampai giliran berikutnya
    ST_ELAK,        // menghindari semua serangan sampai giliran berikutnya
    ST_TANGKIS,     // membalas serangan fisik x1.5 sampai giliran berikutnya
    ST_COUNT
};

struct StatusDef {
    const char* name;
    const char* shortName;   // untuk ikon teks kecil
    int  turns;              // durasi default (giliran pemilik)
    float dotPct;            // damage per giliran (persen MaxHP)
    bool skipTurn;
    float agiMul;
    float dmgTakenMul;
    bool removeOnHit;
    bool battleOnly;         // hilang setelah battle
};
const StatusDef& statusDef(int id);

// ---------------------------------------------------------------- jurus
enum SkillType { SK_SERANG = 0, SK_JURUS, SK_ILMU, SK_DUKUNG };
enum Scope { SC_MUSUH_SATU = 0, SC_MUSUH_SEMUA, SC_DIRI, SC_KAWAN_SATU, SC_KAWAN_SEMUA };
enum FormulaKind { FM_NONE = 0, FM_FISIK, FM_ILMU, FM_PULIH };

struct SkillDef {
    int         id;
    const char* name;
    const char* desc;
    int         type;        // SkillType
    int         scope;       // Scope
    int         mpCost;
    int         formula;     // FormulaKind
    int         base;        // nilai dasar (ilmu/pulih)
    float       mul;         // pengali hasil rumus
    int         element;
    int         chant;       // waktu rapal dalam AP (0 = langsung, 4000 = satu giliran penuh)
    int         status;      // status yang diberikan (ke target / diri bila SC_DIRI)
    int         statusChance;// persen
    int         delayPct;    // dorong giliran target mundur (% dari 4000 AP)
    int         hits;
};

enum SkillIdList {
    SKL_SERANG = 1, SKL_BERTAHAN, SKL_ELAK, SKL_TANGKIS,
    SKL_TEBAS_BERAT, SKL_SAPUAN, SKL_TUSUK_CEPAT, SKL_SERANGAN_GUNCANG,
    SKL_PANAH_SUKMA, SKL_API_TUNGKU, SKL_SEMBUH,
    // jurus musuh
    SKL_GIGIT, SKL_CAKAR_BUSUK, SKL_LUDAH_RACUN, SKL_RAUNG, SKL_HANTAM_GUNCANG, SKL_RAPAL_KUTUK,
    SKL_RAHANG_SANGIT, SKL_PECAH_BUSUK, SKL_CAKAR_JERAMI, SKL_TEBAR_BIJI, SKL_RITUAL_PANEN, SKL_LOLONG_SAWAH,
    SKL_COUNT
};
const SkillDef* skillDef(int id);

// ---------------------------------------------------------------- musuh
struct EnemyAction {
    int skill;
    int rating;         // 1..10 seperti VX Ace
    int hpBelowPct;     // syarat: HP sendiri <= persen ini (100 = selalu)
};

struct EnemyDef {
    int         id;
    const char* name;
    Stats       stats;
    int         sukma;              // hadiah sukma
    float       elementRate[EL_COUNT];
    EnemyAction actions[4];
    int         actionCount;
    const char* battler;            // gambar battler (assets/battlers/...)
    const char* walker;             // sprite peta (assets/chars/...)
};

enum EnemyIdList {
    EN_TUMBAL_GAGAL = 1, EN_TIKUS_LUMBUNG, EN_BELATUNG, EN_PENJAGA_LUMBUNG,
    EN_BELALANG_SANGIT, EN_TIMUN_BUSUK, EN_MEMEDI_SAWAH,
    EN_COUNT
};
const EnemyDef* enemyDef(int id);

struct TroopDef {
    int         id;
    const char* name;
    int         members[4];
    int         count;
};
enum TroopIdList {
    TR_TUMBAL_GAGAL = 1, TR_TIKUS_2, TR_TUMBAL_TIKUS, TR_BELATUNG_3, TR_PENJAGA,
    TR_BELALANG_2, TR_TIMUN_BELALANG, TR_TIMUN_2, TR_MEMEDI,
    TR_COUNT
};
const TroopDef* troopDef(int id);

// ---------------------------------------------------------------- kelas
enum ClassId { KL_PENDEKAR = 0, KL_MALING, KL_PAWANG, KL_COUNT };
struct ClassDef {
    const char* name;
    const char* desc;
    Stats       base;           // level 1
    Stats       growth;         // tambahan per level
    int         skills[6];      // jurus yang dikuasai sejak awal
    int         skillCount;
};
const ClassDef& classDef(int id);
Stats statsAtLevel(int classId, int level);

// ---------------------------------------------------------------- barang
enum ItemKind { IK_PULIH_PERSEN = 0, IK_PULIH_MP, IK_OBAT, IK_SUKMA, IK_SERANG, IK_PULANG, IK_KUNCI };

struct ItemDef {
    int         id;
    const char* key;        // nama pendek untuk data peta/event ("jamu")
    const char* name;
    const char* desc;
    int         kind;       // ItemKind
    int         value;      // persen pulih / jumlah sukma / damage dasar
    int         status;     // status yang disembuhkan (IK_OBAT)
    int         element;    // elemen serangan (IK_SERANG)
    bool        inField;    // bisa dipakai di peta
    bool        inBattle;   // bisa dipakai di battle
};

enum ItemIdList {
    IT_JAMU = 1, IT_PENAWAR, IT_SUKMA_KECIL, IT_SUKMA_SEDANG, IT_BOM_MINYAK, IT_ABU_PULANG, IT_KUNCI_SEL,
    IT_BIJI_TIMUN, IT_COUNT
};
const ItemDef* itemDef(int id);
int itemIdByKey(const char* key);   // 0 bila tidak ada
