#include "GameData.hpp"

const char* elementName(int el) {
    static const char* names[EL_COUNT] = {"-", "Fisik", "Api", "Air", "Angin", "Tanah", "Gaib"};
    return (el >= 0 && el < EL_COUNT) ? names[el] : "?";
}

// ------------------------------------------------------------------ status
static const StatusDef STATUS[ST_COUNT] = {
    //  nama        singkat turn dot    skip   agi    dmgIn  hitRm  battleOnly
    {"-",          "",     0,  0.00f, false, 1.0f,  1.0f,  false, true},
    {"Racun",      "RCN",  5,  0.05f, false, 1.0f,  1.0f,  false, false},
    {"Berdarah",   "DRH",  4,  0.05f, false, 1.0f,  1.0f,  false, true},
    {"Terbakar",   "BKR",  2,  0.08f, false, 1.0f,  1.0f,  false, true},
    {"Lumpuh",     "LMP",  1,  0.00f, true,  1.0f,  1.0f,  false, true},
    {"Tidur",      "TDR",  3,  0.00f, true,  1.0f,  1.0f,  true,  true},
    {"Bisu",       "BSU",  3,  0.00f, false, 1.0f,  1.0f,  false, true},
    {"Takut",      "TKT",  3,  0.00f, false, 0.5f,  1.0f,  false, true},
    {"Rapuh",      "RPH",  3,  0.00f, false, 1.0f,  1.5f,  false, true},
    {"Cepat",      "CPT",  3,  0.00f, false, 2.0f,  1.0f,  false, true},
    {"Lambat",     "LBT",  3,  0.00f, false, 0.5f,  1.0f,  false, true},
    {"Bertahan",   "THN",  1,  0.00f, false, 1.0f,  0.5f,  false, true},
    {"Elak",       "ELK",  1,  0.00f, false, 1.0f,  1.0f,  false, true},
    {"Tangkis",    "TKS",  1,  0.00f, false, 1.0f,  1.0f,  false, true},
};

const StatusDef& statusDef(int id) {
    if (id <= 0 || id >= ST_COUNT) return STATUS[0];
    return STATUS[id];
}

// ------------------------------------------------------------------ jurus
// id, nama, deskripsi, tipe, scope, mp, rumus, dasar, mul, elemen, rapal, status, peluang, delay%, hits
static const SkillDef SKILLS[] = {
    {SKL_SERANG, "Serang", "Serangan biasa dengan senjata.",
     SK_SERANG, SC_MUSUH_SATU, 0, FM_FISIK, 0, 1.0f, EL_FISIK, 0, ST_NONE, 0, 0, 1},
    {SKL_BERTAHAN, "Bertahan", "Damage yang diterima berkurang separuh sampai giliran berikutnya.",
     SK_DUKUNG, SC_DIRI, 0, FM_NONE, 0, 0.0f, EL_NONE, 0, ST_BERTAHAN, 100, 0, 1},
    {SKL_ELAK, "Elak", "Menghindari semua serangan sampai giliran berikutnya.",
     SK_JURUS, SC_DIRI, 3, FM_NONE, 0, 0.0f, EL_NONE, 0, ST_ELAK, 100, 0, 1},
    {SKL_TANGKIS, "Tangkis", "Membalas setiap serangan fisik x1.5 sampai giliran berikutnya.",
     SK_JURUS, SC_DIRI, 5, FM_NONE, 0, 0.0f, EL_NONE, 0, ST_TANGKIS, 100, 0, 1},
    {SKL_TEBAS_BERAT, "Tebas Berat", "Ayunan penuh tenaga. Perlu ancang-ancang sebentar.",
     SK_JURUS, SC_MUSUH_SATU, 6, FM_FISIK, 0, 1.6f, EL_FISIK, 1200, ST_NONE, 0, 15, 1},
    {SKL_SAPUAN, "Sapuan", "Menyapu semua musuh dan menunda giliran mereka.",
     SK_JURUS, SC_MUSUH_SEMUA, 8, FM_FISIK, 0, 0.8f, EL_FISIK, 0, ST_NONE, 0, 20, 1},
    {SKL_TUSUK_CEPAT, "Tusuk Cepat", "Dua tusukan kilat. Bisa membuat berdarah.",
     SK_JURUS, SC_MUSUH_SATU, 5, FM_FISIK, 0, 0.7f, EL_FISIK, 0, ST_BERDARAH, 35, 0, 2},
    {SKL_SERANGAN_GUNCANG, "Hantaman Guncang", "Pukulan ke kepala: menunda giliran target jauh ke belakang.",
     SK_JURUS, SC_MUSUH_SATU, 7, FM_FISIK, 0, 0.9f, EL_FISIK, 0, ST_NONE, 0, 45, 1},
    {SKL_PANAH_SUKMA, "Panah Sukma", "Melontarkan sukma sebagai anak panah.",
     SK_ILMU, SC_MUSUH_SATU, 6, FM_ILMU, 60, 1.0f, EL_GAIB, 0, ST_NONE, 0, 0, 1},
    {SKL_API_TUNGKU, "Api Tungku", "Bara Tungku menyembur ke semua musuh. Rapal lama.",
     SK_ILMU, SC_MUSUH_SEMUA, 12, FM_ILMU, 120, 1.0f, EL_API, 2400, ST_TERBAKAR, 40, 0, 1},
    {SKL_SEMBUH, "Jampi Sembuh", "Memulihkan HP satu kawan.",
     SK_ILMU, SC_KAWAN_SATU, 5, FM_PULIH, 150, 1.0f, EL_NONE, 800, ST_NONE, 0, 0, 1},
    // ---- musuh
    {SKL_GIGIT, "Gigit", "Gigitan kotor yang bisa meracuni.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 1.0f, EL_FISIK, 0, ST_RACUN, 30, 0, 1},
    {SKL_CAKAR_BUSUK, "Cakar Busuk", "Cakaran kuku hitam. Bisa membuat berdarah.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 1.15f, EL_FISIK, 0, ST_BERDARAH, 30, 0, 1},
    {SKL_LUDAH_RACUN, "Ludah Racun", "Menyemburkan lendir beracun.",
     SK_ILMU, SC_MUSUH_SATU, 0, FM_ILMU, 30, 0.8f, EL_AIR, 0, ST_RACUN, 60, 0, 1},
    {SKL_RAUNG, "Raung", "Raungan yang membuat ciut nyali.",
     SK_JURUS, SC_MUSUH_SEMUA, 0, FM_NONE, 0, 0.0f, EL_NONE, 0, ST_TAKUT, 70, 0, 1},
    {SKL_HANTAM_GUNCANG, "Hantam Tanah", "Mengangkat gada tinggi-tinggi... lalu menghantam. Bersiaplah!",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 2.2f, EL_FISIK, 2000, ST_NONE, 0, 40, 1},
    {SKL_LEMPAR_GARAM, "Lempar Garam", "Segenggam garam bekal Timun Mas: membakar makhluk gaib dan membuyarkan rapal.",
     SK_ILMU, SC_MUSUH_SATU, 6, FM_ILMU, 70, 0.9f, EL_GAIB, 0, ST_NONE, 0, 35, 1},
    {SKL_SULUR_TIMUN, "Sulur Timun", "Sulur timun mengikat semua musuh dan memperlambat mereka.",
     SK_ILMU, SC_MUSUH_SEMUA, 10, FM_ILMU, 30, 0.6f, EL_TANAH, 1000, ST_LAMBAT, 70, 0, 1},
    {SKL_RAHANG_SANGIT, "Rahang Sangit", "Gigitan rahang belalang yang merobek kulit.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 0.9f, EL_FISIK, 0, ST_BERDARAH, 40, 0, 1},
    {SKL_PECAH_BUSUK, "Pecah Busuk", "Tubuhnya merekah, menyemburkan getah busuk ke semua lawan.",
     SK_ILMU, SC_MUSUH_SEMUA, 0, FM_ILMU, 30, 0.7f, EL_AIR, 0, ST_RACUN, 45, 0, 1},
    {SKL_CAKAR_JERAMI, "Cakar Jerami", "Jari-jari jerami mencakar dua kali.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 0.75f, EL_FISIK, 0, ST_BERDARAH, 25, 0, 2},
    {SKL_TEBAR_BIJI, "Tebar Biji Busuk", "Biji timun busuk ditaburkan ke tubuhmu.",
     SK_ILMU, SC_MUSUH_SATU, 0, FM_ILMU, 50, 0.9f, EL_TANAH, 0, ST_RACUN, 70, 0, 1},
    {SKL_RITUAL_PANEN, "Ritual Panen", "Ia merentangkan tangan dan mulai menembang lagu panen... Hentikan sebelum selesai!",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 2.6f, EL_FISIK, 2600, ST_RAPUH, 100, 0, 1},
    {SKL_LOLONG_SAWAH, "Lolong Sawah", "Lolongan dari dalam labu kering membuat lututmu gemetar.",
     SK_JURUS, SC_MUSUH_SEMUA, 0, FM_NONE, 0, 0.0f, EL_NONE, 0, ST_TAKUT, 80, 0, 1},
    {SKL_HUJAN_JARUM, "Hujan Jarum", "Ribuan jarum bambu berjatuhan ke semua lawan.",
     SK_ILMU, SC_MUSUH_SEMUA, 0, FM_ILMU, 40, 0.8f, EL_ANGIN, 0, ST_BERDARAH, 30, 0, 1},
    {SKL_PATUK, "Patuk Bangkai", "Paruh hitam mematuk luka yang terbuka.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 1.2f, EL_FISIK, 0, ST_RAPUH, 35, 0, 1},
    {SKL_OMBAK_ASIN, "Ombak Asin", "Gelombang air garam menghantam semua lawan.",
     SK_ILMU, SC_MUSUH_SEMUA, 0, FM_ILMU, 60, 0.85f, EL_AIR, 0, ST_NONE, 0, 0, 1},
    {SKL_BELIT, "Belit Tentakel", "Tentakel membelit dan menyeret giliranmu mundur.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 0.9f, EL_FISIK, 0, ST_NONE, 0, 35, 1},
    {SKL_SABET_JALA, "Sabet Jala", "Jala nelayan yang membusuk melilit tubuhmu.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 1.0f, EL_FISIK, 0, ST_LAMBAT, 60, 0, 1},
    {SKL_LILIT_AKAR, "Lilit Akar", "Akar rawa mengikat kakimu.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 0.9f, EL_TANAH, 0, ST_LUMPUH, 30, 0, 1},
    {SKL_TERKAM, "Terkam", "Rahang bangkai buaya terbuka lebar... lalu mengatup.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 2.1f, EL_FISIK, 1300, ST_BERDARAH, 50, 0, 1},
    {SKL_RAUNG_BUTO, "Raung Buto", "Raungan raksasa mengguncang rawa.",
     SK_JURUS, SC_MUSUH_SEMUA, 0, FM_FISIK, 0, 0.5f, EL_FISIK, 0, ST_TAKUT, 70, 15, 1},
    {SKL_LAPAR_BUTO, "Lapar Buto", "Buto Ijo membuka mulutnya lebar-lebar. Ia akan menelanmu! Hentikan, atau elakkan!",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 3.0f, EL_FISIK, 3000, ST_NONE, 0, 0, 1},
    {SKL_NAPAS_TERASI, "Napas Terasi", "Napas busuk udang fermentasi menyelimuti semua lawan.",
     SK_ILMU, SC_MUSUH_SEMUA, 0, FM_ILMU, 50, 0.9f, EL_AIR, 0, ST_RACUN, 60, 0, 1},
    {SKL_CENGKERAM, "Cengkeram", "Tangan raksasa meremas tubuhmu.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 1.2f, EL_FISIK, 0, ST_LUMPUH, 35, 0, 1},
    {SKL_HANTAM_GADA, "Hantam Gada", "Gada kayu menghantam dan membuatmu terhuyung.",
     SK_JURUS, SC_MUSUH_SATU, 0, FM_FISIK, 0, 1.6f, EL_FISIK, 0, ST_NONE, 0, 30, 1},
    {SKL_RAPAL_KUTUK, "Kutuk Lumbung", "Mantra busuk yang membuat tubuh rapuh.",
     SK_ILMU, SC_MUSUH_SATU, 0, FM_ILMU, 40, 0.9f, EL_GAIB, 1600, ST_RAPUH, 80, 0, 1},
};
static const int SKILL_COUNT = sizeof(SKILLS) / sizeof(SKILLS[0]);

const SkillDef* skillDef(int id) {
    for (int i = 0; i < SKILL_COUNT; ++i)
        if (SKILLS[i].id == id) return &SKILLS[i];
    return nullptr;
}

// ------------------------------------------------------------------ musuh
//                        -     fisik api   air   angin tanah gaib
#define RATES_NORMAL     {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
#define RATES_MAYAT      {1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.5f}
#define RATES_SERANGGA   {1.0f, 1.0f, 2.0f, 0.5f, 1.0f, 1.0f, 1.0f}

static const EnemyDef ENEMIES[] = {
    {EN_TUMBAL_GAGAL, "Tumbal Gagal", {90, 0, 30, 6, 5, 5, 30, 8}, 15, RATES_MAYAT,
     {{SKL_SERANG, 5, 100}, {SKL_CAKAR_BUSUK, 4, 100}}, 2,
     "assets/battlers/tumbal_gagal.png", "assets/chars/tumbal_gagal.png"},
    {EN_TIKUS_LUMBUNG, "Tikus Lumbung", {60, 0, 26, 4, 5, 5, 55, 10}, 8, RATES_NORMAL,
     {{SKL_SERANG, 5, 100}, {SKL_GIGIT, 5, 100}}, 2,
     "assets/battlers/tikus_lumbung.png", "assets/chars/tikus.png"},
    {EN_BELATUNG, "Belatung Lumbung", {70, 20, 22, 10, 10, 5, 20, 5}, 10, RATES_SERANGGA,
     {{SKL_SERANG, 5, 100}, {SKL_LUDAH_RACUN, 4, 100}}, 2,
     "assets/battlers/belatung.png", "assets/chars/belatung.png"},
    {EN_BELALANG_SANGIT, "Belalang Sangit", {110, 0, 31, 8, 5, 6, 56, 12}, 22, RATES_SERANGGA,
     {{SKL_SERANG, 5, 100}, {SKL_RAHANG_SANGIT, 5, 100}}, 2,
     "assets/battlers/belalang_sangit.png", "assets/chars/tikus.png"},
    {EN_TIMUN_BUSUK, "Timun Busuk", {190, 30, 30, 16, 22, 12, 22, 8}, 30, RATES_SERANGGA,
     {{SKL_SERANG, 4, 100}, {SKL_LUDAH_RACUN, 5, 100}, {SKL_PECAH_BUSUK, 4, 60}}, 3,
     "assets/battlers/timun_busuk.png", "assets/chars/tikus.png"},
    {EN_MEMEDI_SAWAH, "Memedi Sawah", {760, 60, 40, 14, 26, 12, 30, 12}, 420,
     {1.0f, 1.0f, 2.0f, 1.0f, 1.2f, 0.8f, 1.0f},
     {{SKL_CAKAR_JERAMI, 5, 100}, {SKL_TEBAR_BIJI, 4, 100}, {SKL_RITUAL_PANEN, 5, 100}, {SKL_LOLONG_SAWAH, 6, 50}}, 4,
     "assets/battlers/memedi_sawah.png", "assets/chars/tikus.png"},
#define RATES_ARWAH      {1.0f, 0.7f, 1.2f, 1.0f, 1.0f, 1.0f, 1.6f}
#define RATES_TULANG     {1.0f, 1.0f, 1.3f, 1.0f, 1.0f, 1.2f, 1.6f}
#define RATES_BUTO       {1.0f, 1.0f, 1.2f, 0.8f, 1.0f, 1.0f, 1.3f}
    {EN_ARWAH_JARUM, "Arwah Jarum", {160, 40, 24, 10, 29, 22, 48, 12}, 42, RATES_ARWAH,
     {{SKL_SERANG, 4, 100}, {SKL_HUJAN_JARUM, 5, 100}}, 2,
     "assets/battlers/arwah_jarum.png", "assets/chars/arwah.png"},
    {EN_GAGAK_BANGKAI, "Gagak Bangkai", {210, 0, 41, 13, 10, 12, 56, 14}, 52, RATES_NORMAL,
     {{SKL_SERANG, 5, 100}, {SKL_PATUK, 5, 100}}, 2,
     "assets/battlers/gagak_bangkai.png", "assets/chars/gagak.png"},
    {EN_ULAR_BAMBU, "Ular Bambu", {175, 0, 38, 16, 10, 12, 40, 10}, 46, RATES_NORMAL,
     {{SKL_SERANG, 5, 100}, {SKL_GIGIT, 5, 100}}, 2,
     "assets/battlers/ular_bambu.png", "assets/chars/ular.png"},
    {EN_HANTU_LAUT, "Hantu Laut", {450, 70, 46, 20, 52, 28, 36, 12}, 90, RATES_ARWAH,
     {{SKL_SERANG, 3, 100}, {SKL_OMBAK_ASIN, 5, 100}, {SKL_BELIT, 4, 100}}, 3,
     "assets/battlers/hantu_laut.png", "assets/chars/arwah.png"},
    {EN_TENGKORAK_NELAYAN, "Tengkorak Nelayan", {380, 0, 66, 28, 10, 14, 38, 10}, 80, RATES_TULANG,
     {{SKL_SERANG, 5, 100}, {SKL_SABET_JALA, 4, 100}}, 2,
     "assets/battlers/tengkorak_nelayan.png", "assets/chars/tumbal_gagal.png"},
    {EN_AKAR_RAWA, "Akar Rawa", {540, 50, 62, 24, 46, 20, 28, 10}, 110, RATES_SERANGGA,
     {{SKL_SERANG, 4, 100}, {SKL_LILIT_AKAR, 5, 100}, {SKL_PECAH_BUSUK, 4, 100}}, 3,
     "assets/battlers/akar_rawa.png", "assets/chars/timun_busuk.png"},
    {EN_BUAYA_BANGKAI, "Buaya Bangkai", {700, 0, 80, 30, 10, 18, 32, 10}, 130, RATES_TULANG,
     {{SKL_SERANG, 5, 100}, {SKL_TERKAM, 4, 100}}, 2,
     "assets/battlers/buaya_bangkai.png", "assets/chars/tikus.png"},
    {EN_BUTO_KEPALA, "Buto Ijo", {2400, 240, 70, 26, 52, 22, 32, 14}, 1500, RATES_BUTO,
     {{SKL_RAUNG_BUTO, 3, 100}, {SKL_LAPAR_BUTO, 5, 100}, {SKL_NAPAS_TERASI, 4, 100}, {SKL_RAUNG_BUTO, 6, 30}}, 4,
     "assets/battlers/buto_kepala.png", "assets/chars/tikus.png", true},
    {EN_BUTO_KIRI, "Tangan Kiri", {820, 0, 64, 30, 10, 18, 42, 10}, 200, RATES_BUTO,
     {{SKL_SERANG, 4, 100}, {SKL_CENGKERAM, 5, 100}}, 2,
     "assets/battlers/buto_kiri.png", "assets/chars/tikus.png"},
    {EN_BUTO_KANAN, "Tangan Kanan", {820, 0, 72, 30, 10, 18, 36, 10}, 200, RATES_BUTO,
     {{SKL_SERANG, 4, 100}, {SKL_HANTAM_GADA, 5, 100}}, 2,
     "assets/battlers/buto_kanan.png", "assets/chars/tikus.png"},
    {EN_PENJAGA_LUMBUNG, "Penjaga Lumbung", {440, 40, 36, 12, 18, 14, 26, 10}, 220, RATES_MAYAT,
     {{SKL_SERANG, 5, 100}, {SKL_HANTAM_GUNCANG, 5, 100}, {SKL_RAUNG, 3, 60}, {SKL_RAPAL_KUTUK, 4, 100}}, 4,
     "assets/battlers/penjaga_lumbung.png", "assets/chars/penjaga_lumbung.png"},
};
static const int ENEMY_COUNT = sizeof(ENEMIES) / sizeof(ENEMIES[0]);

const EnemyDef* enemyDef(int id) {
    for (int i = 0; i < ENEMY_COUNT; ++i)
        if (ENEMIES[i].id == id) return &ENEMIES[i];
    return nullptr;
}

static const TroopDef TROOPS[] = {
    {TR_TUMBAL_GAGAL, "Tumbal Gagal", {EN_TUMBAL_GAGAL}, 1},
    {TR_TIKUS_2, "Tikus Lumbung x2", {EN_TIKUS_LUMBUNG, EN_TIKUS_LUMBUNG}, 2},
    {TR_TUMBAL_TIKUS, "Tumbal & Tikus", {EN_TUMBAL_GAGAL, EN_TIKUS_LUMBUNG}, 2},
    {TR_BELATUNG_3, "Belatung x3", {EN_BELATUNG, EN_BELATUNG, EN_BELATUNG}, 3},
    {TR_PENJAGA, "Penjaga Lumbung", {EN_PENJAGA_LUMBUNG}, 1},
    {TR_BELALANG_2, "Belalang Sangit x2", {EN_BELALANG_SANGIT, EN_BELALANG_SANGIT}, 2},
    {TR_TIMUN_BELALANG, "Timun & Belalang", {EN_TIMUN_BUSUK, EN_BELALANG_SANGIT}, 2},
    {TR_TIMUN_2, "Timun Busuk x2", {EN_TIMUN_BUSUK, EN_TIMUN_BUSUK}, 2},
    {TR_MEMEDI, "Memedi Sawah", {EN_MEMEDI_SAWAH}, 1},
    {TR_ARWAH_2, "Arwah Jarum x2", {EN_ARWAH_JARUM, EN_ARWAH_JARUM}, 2},
    {TR_GAGAK, "Gagak Bangkai", {EN_GAGAK_BANGKAI, EN_ULAR_BAMBU}, 2},
    {TR_ULAR_ARWAH, "Ular & Arwah", {EN_ULAR_BAMBU, EN_ARWAH_JARUM, EN_ULAR_BAMBU}, 3},
    {TR_HANTU_LAUT, "Hantu Laut", {EN_HANTU_LAUT}, 1},
    {TR_TENGKORAK_2, "Tengkorak Nelayan x2", {EN_TENGKORAK_NELAYAN, EN_TENGKORAK_NELAYAN}, 2},
    {TR_HANTU_TENGKORAK, "Hantu & Tengkorak", {EN_TENGKORAK_NELAYAN, EN_HANTU_LAUT}, 2},
    {TR_AKAR_2, "Akar Rawa x2", {EN_AKAR_RAWA, EN_AKAR_RAWA}, 2},
    {TR_BUAYA, "Buaya Bangkai", {EN_BUAYA_BANGKAI, EN_BELATUNG}, 2},
    {TR_AKAR_BELATUNG, "Akar & Belatung", {EN_BELATUNG, EN_AKAR_RAWA, EN_BELATUNG}, 3},
    {TR_BUTO, "Buto Ijo", {EN_BUTO_KIRI, EN_BUTO_KEPALA, EN_BUTO_KANAN}, 3},
};
static const int TROOP_COUNT = sizeof(TROOPS) / sizeof(TROOPS[0]);

const TroopDef* troopDef(int id) {
    for (int i = 0; i < TROOP_COUNT; ++i)
        if (TROOPS[i].id == id) return &TROOPS[i];
    return nullptr;
}

// ------------------------------------------------------------------ kelas
// Stat awal mengikuti tiga kelas awal Black Souls (Ksatria/Maling/Penyihir).
static const ClassDef CLASSES[KL_COUNT] = {
    {"Pendekar", "Kuat dan tahan pukul.",
     {386, 100, 16, 16, 10, 10, 40, 10}, {52, 2, 3, 3, 2, 2, 4, 3},
     {SKL_SERANG, SKL_ELAK, SKL_TANGKIS, SKL_TEBAS_BERAT, SKL_SAPUAN}, 5},
    {"Maling", "Gesit; bertindak lebih sering. Membawa sukma curian.",
     {386, 100, 10, 10, 10, 10, 50, 14}, {48, 2, 3, 2, 2, 2, 5, 4},
     {SKL_SERANG, SKL_ELAK, SKL_TANGKIS, SKL_TUSUK_CEPAT, SKL_SERANGAN_GUNCANG}, 5},
    {"Pawang", "Menguasai ilmu sukma.",
     {386, 120, 10, 10, 16, 16, 40, 10}, {46, 4, 2, 2, 4, 3, 4, 3},
     {SKL_SERANG, SKL_ELAK, SKL_TANGKIS, SKL_PANAH_SUKMA, SKL_API_TUNGKU, SKL_SEMBUH}, 6},
    {"Timun Mas", "Gadis yang dulu lolos dari Buto Ijo. Kini tak sepenuhnya hidup.",
     {300, 120, 12, 10, 18, 18, 46, 16}, {40, 5, 2, 2, 4, 4, 4, 3},
     {SKL_SERANG, SKL_SEMBUH, SKL_LEMPAR_GARAM, SKL_SULUR_TIMUN}, 4},
};

const ClassDef& classDef(int id) {
    if (id < 0 || id >= KL_COUNT) return CLASSES[0];
    return CLASSES[id];
}

Stats statsAtLevel(int classId, int level) {
    const ClassDef& c = classDef(classId);
    int n = level > 1 ? level - 1 : 0;
    Stats s = c.base;
    s.mhp += c.growth.mhp * n; s.mmp += c.growth.mmp * n;
    s.atk += c.growth.atk * n; s.def += c.growth.def * n;
    s.mag += c.growth.mag * n; s.res += c.growth.res * n;
    s.agi += c.growth.agi * n; s.luk += c.growth.luk * n;
    return s;
}

// ------------------------------------------------------------------ barang
// Ramuan Jamu = Herb Flask Black Souls: pulih 50% HP, terisi ulang di Tungku.
static const ItemDef ITEMS[] = {
    {IT_JAMU, "jamu", "Ramuan Jamu", "Memulihkan separuh HP. Terisi ulang setiap beristirahat di Tungku.",
     IK_PULIH_PERSEN, 50, ST_NONE, EL_NONE, true, true},
    {IT_PENAWAR, "penawar", "Daun Penawar", "Menyembuhkan racun dan darah yang tak berhenti mengalir.",
     IK_OBAT, 0, ST_RACUN, EL_NONE, true, true},
    {IT_SUKMA_KECIL, "sukma_kecil", "Sukma Tumbal Kecil", "Sisa sukma seorang anak tumbal. Pecahkan untuk mendapat 40 sukma.",
     IK_SUKMA, 40, ST_NONE, EL_NONE, true, false},
    {IT_SUKMA_SEDANG, "sukma_sedang", "Sukma Penjaga", "Sukma pekat seorang penjaga lumbung. Pecahkan untuk 200 sukma.",
     IK_SUKMA, 200, ST_NONE, EL_NONE, true, false},
    {IT_BOM_MINYAK, "bom_minyak", "Bom Minyak Jarak", "Kendi minyak jarak yang dilempar dan terbakar. Damage api 90.",
     IK_SERANG, 90, ST_TERBAKAR, EL_API, false, true},
    {IT_ABU_PULANG, "abu_pulang", "Abu Tungku", "Segenggam abu Tungku. Membawamu kembali ke Tungku terakhir.",
     IK_PULANG, 0, ST_NONE, EL_NONE, true, false},
    {IT_JARUM, "jarum", "Jarum Timun Mas", "Bekal kedua. Sebatang jarum emas yang dulu menumbuhkan hutan bambu.",
     IK_KUNCI, 0, ST_NONE, EL_NONE, false, false},
    {IT_GARAM, "garam", "Garam Timun Mas", "Bekal ketiga. Segenggam garam yang dulu menjadi lautan.",
     IK_KUNCI, 0, ST_NONE, EL_NONE, false, false},
    {IT_TERASI, "terasi", "Terasi Timun Mas", "Bekal keempat. Bau busuknya dulu menenggelamkan raksasa.",
     IK_KUNCI, 0, ST_NONE, EL_NONE, false, false},
    {IT_BIJI_TIMUN, "biji_timun", "Biji Timun Mas", "Bekal pertama Timun Mas. Hangat, berdenyut, dan masih lapar akan tanah.",
     IK_KUNCI, 0, ST_NONE, EL_NONE, false, false},
    {IT_KUNCI_SEL, "kunci_sel", "Kunci Sel Berkarat", "Kunci kecil berkarat dari lumbung Buto.",
     IK_KUNCI, 0, ST_NONE, EL_NONE, false, false},
};
static const int ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);

const ItemDef* itemDef(int id) {
    for (int i = 0; i < ITEM_COUNT; ++i)
        if (ITEMS[i].id == id) return &ITEMS[i];
    return nullptr;
}

int itemIdByKey(const char* key) {
    if (key == nullptr) return 0;
    for (int i = 0; i < ITEM_COUNT; ++i) {
        const char* a = ITEMS[i].key;
        const char* b = key;
        while (*a && *a == *b) { ++a; ++b; }
        if (*a == '\0' && *b == '\0') return ITEMS[i].id;
    }
    return 0;
}
