"""Sumber terjemahan Bahasa Inggris TUMBAL -> assets/lang/en.json.

Kunci = teks Indonesia persis seperti di kode/data/skrip (lihat tools/lang_keys.py).
Gaya: lokalisasi muram dan ringkas ala JRPG kelam (orang kedua "you", kalimat pendek,
sedikit arkais). Istilah tetap:
    sukma -> Souls        Tungku -> Hearth        tumbal -> Offering
    Ramuan Jamu -> Jamu Draught      bekal (Timun Mas) -> Provision
    lesung -> rice mortar  rapal -> chant         Abu Tungku -> Hearth Ash
Nama diri (Timun Mas, Buto Ijo, Mbok Srini, Empu Karta, Karangtumbal) tidak diterjemahkan.

    /c/msys64/ucrt64/bin/python.exe tools/lang_en.py      # tulis assets/lang/en.json
"""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

T = {
    # ------------------------------------------------------------------ console
    "\n\n        T A M A T\n\n  Buto Ijo runtuh. Desa Karangtumbal tidak lagi memberi tumbal.\n":
        "\n\n        T H E   E N D\n\n  Buto Ijo has fallen. Karangtumbal will offer no more children.\n",
    "\n  %s  (Enter)": "\n  %s  (Enter)",
    "\n  == PANDAI BESI ==  %s +%d   Sukma %d   Bijih Besi %d\n": "\n  == BLACKSMITH ==  %s +%d   Souls %d   Iron Ore %d\n",
    "\n  == PEDAGANG ==  Sukma %d\n": "\n  == MERCHANT ==  Souls %d\n",
    "\n  == TUNGKU ==  Level %d   Sukma %d\n  1) Naik level (butuh %d sukma)\n  2) Warp\n  3) Pergi\n":
        "\n  == HEARTH ==  Level %d   Souls %d\n  1) Level up (%d souls)\n  2) Warp\n  3) Leave\n",
    "\n  KAMU MATI. Sukmamu tercecer di tempat itu.\n": "\n  YOU DIED. Your souls lie scattered where you fell.\n",
    "\n  b=Bertahan  j=Ramuan Jamu(%d)  k=Kabur   pilih [jurus target]: ":
        "\n  b=Guard  j=Jamu Draught(%d)  k=Escape   choose [skill target]: ",
    "\n  pilih: ": "\n  choose: ",
    "\n%s  (posisi %d,%d)\n": "\n%s  (position %d,%d)\n",
    "\n== Graph dunia (adjacency multi linked list) ==\n": "\n== World Graph (adjacency multi linked list) ==\n",
    "\n=== BATTLE: %s ===\n": "\n=== BATTLE: %s ===\n",
    "\nTUMBAL - %s\n": "\nTUMBAL - %s\n",
    "\n[HP %d/%d  Jamu %d  Sukma %d] > ": "\n[HP %d/%d  Jamu %d  Souls %d] > ",
    "   %d) %s  (%d jalan)\n": "   %d) %s  (%d paths)\n",
    "  %d) %-20s %4d sukma%s\n": "  %d) %-20s %4d souls%s\n",
    "  %d=%s(%dMP)%s": "  %d=%s(%dMP)%s",
    "  %s menerima %d damage%s\n": "  %s takes %d damage%s\n",
    "  %s pulih %d HP\n": "  %s recovers %d HP\n",
    "  %s slot %d.\n": "  %s slot %d.\n",
    "  %s: %s hilang\n": "  %s: %s wears off\n",
    "  (aksi tidak valid)\n": "  (invalid action)\n",
    "  (habis)": "  (sold out)",
    "  (sisa %d)": "  (%d left)",
    "  *Tungku": "  *Hearth",
    "  -  Level %d": "  -  Level %d",
    "  0) Pergi\n  pilih: ": "  0) Leave\n  choose: ",
    "  1) Tempa ke +%d (%d sukma, %d bijih)\n": "  1) Forge to +%d (%d souls, %d ore)\n",
    "  4) Timun Mas naik level (Lv %d, butuh %d sukma)\n  5) Panggil / lepas Timun Mas (%s)\n":
        "  4) Level up Timun Mas (Lv %d, %d souls)\n  5) Summon / dismiss Timun Mas (%s)\n",
    "  Antrean giliran (Timeline DLL):": "  Turn order (Timeline DLL):",
    "  Gagal: %s\n": "  Failed: %s\n",
    "  Kematian: %d   Level: %d\n\n": "  Deaths: %d   Level: %d\n\n",
    "  [%d] %-18s (tumbang)\n": "  [%d] %-18s (fallen)\n",
    "  [%d] %-18s HP %4d/%-4d MP %3d  %s[%-10.*s]": "  [%d] %-18s HP %4d/%-4d MP %3d  %s[%-10.*s]",
    "  [rapal]": "  [chant]",
    "  beli nomor (0 = pergi): ": "  buy number (0 = leave): ",
    "  cheat <nomor> [stat]   (stat: 0 HP, 1 MP, 2 Serang, 3 Tahan, 4 Ilmu, 5 Tolak, 6 Gesit, 7 Untung)\n":
        "  cheat <number> [stat]   (stat: 0 HP, 1 MP, 2 Attack, 3 Defense, 4 Magic, 5 Resist, 6 Agility, 7 Luck)\n",
    "  pilih: ": "  choose: ",
    "  tujuan (0 = batal): ": "  destination (0 = cancel): ",
    " (KRITIS!)": " (CRITICAL!)",
    " MP": " MP",
    "AP   ": "AP   ",
    "Giliran %s. Jurus:": "%s's turn. Skills:",
    "Perintah tidak dikenal.\n": "Unknown command.\n",
    "Perintah: w/a/s/d (+angka langkah, mis. 'd 5'), q/e/z/c = diagonal, x = periksa/bicara,\n          jamu, graph, tungku, buka <a> <b>, lihat, simpan <1-3>, muat <1-3>, cheat, keluar\nPeta: @ kamu, M musuh, N orang, ? periksa, * barang, T Tungku, D pintu, ~ sukma tercecer\n":
        "Commands: w/a/s/d (+steps, e.g. 'd 5'), q/e/z/c = diagonal, x = examine/talk,\n          jamu, graph, tungku, buka <a> <b>, lihat, simpan <1-3>, muat <1-3>, cheat, keluar\nMap: @ you, M enemy, N person, ? examine, * item, T Hearth, D door, ~ lost souls\n",
    "Tungku terdekat yang sudah dikunjungi: %s\n": "Nearest visited Hearth: %s\n",
    "mode console": "console mode",
    "RAPAL": "CHANT",
    "Tersimpan di": "Saved to",
    "Dimuat dari": "Loaded from",
    ", ada Tungku": ", has a Hearth",

    # ------------------------------------------------------------------ format & pesan core
    "%d Bijih Besi": "%d Iron Ore",
    "%d jalan": "%d paths",
    "%d sukma": "%d souls",
    "%d sukma lama hilang selamanya.": "%d old souls are lost forever.",
    "%s bersiap: %s...": "%s readies %s...",
    "%s ditempa menjadi +%d.": "%s has been forged to +%d.",
    "%s memakai %s": "%s uses %s",
    "%s menangkis dan membalas!": "%s parries and strikes back!",
    "%s menderita karena %s.": "%s suffers from %s.",
    "%s mengelak!": "%s evades!",
    "%s menjadi +%d.": "%s becomes +%d.",
    "%s merapal %s...": "%s begins chanting %s...",
    "%s merasa lebih baik.": "%s feels better.",
    "%s roboh!": "%s collapses!",
    "%s terhuyung!": "%s staggers!",
    "%s terkena %s.": "%s is afflicted with %s.",
    "%s tidak bisa bergerak!": "%s cannot move!",
    "%s tumbang.": "%s falls.",
    "%s: rapal buyar!": "%s: the chant is broken!",
    "%s +%d.": "%s +%d.",
    "Abu Tungku membawamu pulang.": "The Hearth Ash carries you home.",
    "Bekal barang ditambahkan.": "Supplies added.",
    "Bijih Besi kurang (butuh %d).": "Not enough Iron Ore (%d needed).",
    "Barang itu sudah habis.": "That item is sold out.",
    "Dibeli: %s.": "Bought: %s.",
    "Didapat: %s": "Obtained: %s",
    "Didapat: %s x%d": "Obtained: %s x%d",
    "Dimuat dari slot %d.": "Loaded from slot %d.",
    "Diserahkan: %s": "Handed over: %s",
    "Gagal kabur!": "Could not escape!",
    "Gagal menyimpan: %s": "Save failed: %s",
    "HP pulih %d.": "Recovered %d HP.",
    "Jalan pintas terbuka.": "A shortcut has opened.",
    "Kamu berhasil kabur.": "You escaped.",
    "Kamu beristirahat di Tungku. HP pulih, Ramuan Jamu terisi.": "You rest at the Hearth. HP restored, Jamu Draughts refilled.",
    "Kamu disergap!": "You were ambushed!",
    "Kamu mati.": "You died.",
    "Kamu menyerang lebih dulu!": "You strike first!",
    "Kamu sudah berada di sini.": "You are already here.",
    "Kebal: MATI.": "Invincible: OFF.",
    "Kebal: NYALA.": "Invincible: ON.",
    "Level %d   -   Mati %d kali   -   Bekal %d/4": "Level %d   -   Died %d times   -   Provisions %d/4",
    "Level %d  -  Sukma %d  -  peta %d": "Level %d  -  Souls %d  -  map %d",
    "Level %d  ->  %d": "Level %d  ->  %d",
    "Menang! Sukma +%d": "Victory! Souls +%d",
    "Naik ke level %d.": "Reached level %d.",
    "Racun dan luka mereda.": "The poison and bleeding subside.",
    "Sekali pukul: MATI.": "One-hit kill: OFF.",
    "Sekali pukul: NYALA.": "One-hit kill: ON.",
    "Semua jalan terbuka (%d) dan semua Tungku menyala.": "All paths opened (%d) and every Hearth lit.",
    "Senjata: %s +%d    Wadah jamu: %d": "Weapon: %s +%d    Jamu flasks: %d",
    "Senjatamu sudah tidak bisa ditempa lagi.": "Your weapon cannot be forged any further.",
    "Sukma %d": "Souls %d",
    "Sukma +%d": "Souls +%d",
    "Sukma +%d   (dibawa: %d)": "Souls +%d   (carried: %d)",
    "Sukma +%d.": "Souls +%d.",
    "Sukma +1000": "Souls +1000",
    "Sukma +1000 (%d).": "Souls +1000 (%d).",
    "Sukma tercecer: %d": "Lost souls: %d",
    "Sukma tidak cukup (butuh %d).": "Not enough souls (%d needed).",
    "Sukma yang tercecer kembali: +%d": "Recovered lost souls: +%d",
    "Terkunci dari sisi lain.": "Locked from the other side.",
    "Tersimpan di slot %d.": "Saved to slot %d.",
    "Tidak ada gunanya sekarang.": "That would do nothing now.",
    "Tidak ada jalan untuk kabur!": "There is no escape!",
    "Tidak bisa membawa lebih banyak.": "You cannot carry any more.",
    "Timun Mas bergabung.": "Timun Mas has joined you.",
    "Timun Mas berjalan bersamamu.": "Timun Mas walks with you.",
    "Timun Mas menunggu di Tungku.": "Timun Mas waits at the Hearth.",
    "Timun Mas naik ke level %d.": "Timun Mas reached level %d.",
    "Timun Mas sudah bersamamu.": "Timun Mas is already with you.",
    "Tubuhmu pulih sepenuhnya.": "Your body is fully restored.",
    "Wadah Ramuan Jamu bertambah (%d).": "Jamu flasks increased (%d).",
    "berkas simpan rusak": "the save file is corrupted",
    "tidak bisa menulis %s": "cannot write %s",
    "...musuh-musuh bangkit kembali.": "...and the dead rise again.",
    "Bara Tungku menelanmu...": "The Hearth's embers swallow you...",

    # ------------------------------------------------------------------ antarmuka
    "   -   memakai cheat": "   -   cheats used",
    "(belum ada)": "(none yet)",
    "(berjalan)": "(running)",
    "(diam)": "(idle)",
    "(kosong)": "(empty)",
    "(tidak membawa apa-apa)": "(you carry nothing)",
    "Aset: Sevarihk, Redshrike, Nidhoggn, Kenney & lainnya (lihat CREDITS.md)": "Assets: Sevarihk, Redshrike, Nidhoggn, Kenney & others (see CREDITS.md)",
    "Aset: lihat CREDITS.md": "Assets: see CREDITS.md",
    "BUYAR!": "BROKEN!",
    "Bahan belum cukup.    [X] pergi": "Not enough materials.    [X] leave",
    "Bahasa": "Language",
    "Barang": "Item",
    "Batal": "Cancel",
    "Bekal barang": "Supplies",
    "Bertahan": "Guard",
    "Biaya": "Cost",
    "Buka semua jalan & Tungku": "Open all paths & Hearths",
    "Butuh %d sukma": "%d souls needed",
    "CHEAT  -  untuk uji coba / cepat tamat (layar tamat mencatat kamu curang)":
        "CHEATS  -  for testing / a quick finish (the ending will remember)",
    "Cheat": "Cheats",
    "Dulu, sebelum ditanam, kamu adalah...": "Before you were planted, you were...",
    "Elak!": "Evaded!",
    "Giliran:": "Turns:",
    "Graph dunia (vertex = peta, edge = jalan):": "World graph (vertex = map, edge = path):",
    "INSPECTOR  (F1)": "INSPECTOR  (F1)",
    "Interpreter event: Stack kedalaman %d %s": "Event interpreter: Stack depth %d %s",
    "Jurus": "Skill",
    "KAMU MATI": "YOU DIED",
    "KRITIS!": "CRITICAL!",
    "Kabur": "Escape",
    "Kebal": "Invincible",
    "Keluar": "Quit",
    "Keluar dari game": "Quit the game",
    "Kemajuan sejak terakhir menyimpan akan hilang.": "Progress since your last save will be lost.",
    "Kembali ke layar judul": "Return to the title screen",
    "Lanjutkan": "Continue",
    "Level": "Level",
    "Lihat dagangan": "Browse wares",
    "MENANG": "VICTORY",
    "Mati: %d kali": "Deaths: %d",
    "Mulai Baru": "New Game",
    "NYALA": "ON",
    "Naik Level": "Level Up",
    "Naik level (gratis)": "Level up (free)",
    "Operasi struktur data terbaru:": "Latest data structure operations:",
    "PEDAGANG KELILING": "WANDERING MERCHANT",
    "Panggil Timun Mas": "Summon Timun Mas",
    "Pengaturan": "Settings",
    "Pergi": "Leave",
    "Pulih penuh": "Full recovery",
    "Satu benda yang kau genggam saat ditanam:": "The one thing you clutched as they buried you:",
    "Sekali pukul tumbang": "One-hit kill",
    "Senjata +3": "Weapon +3",
    "Senjata ini sudah ditempa sampai batasnya.": "This weapon has been forged to its limit.",
    "Serang": "Attack",
    "Siapa namamu, tumbal?": "What is your name, Offering?",
    "Simpan": "Save",
    "Simpan ke slot (mati tidak pernah menyimpan otomatis):": "Save to slot (death never saves automatically):",
    "Slot %d": "Slot %d",
    "Status": "Status",
    "Status: berjalan bersamamu": "Status: walking with you",
    "Status: menunggu di Tungku": "Status: waiting at the Hearth",
    "Sukma": "Souls",
    "Sukma tidak cukup.    [X] kembali": "Not enough souls.    [X] back",
    "Sukma yang kamu bawa tercecer di tempat ini.": "The souls you carried lie scattered here.",
    "TERKUNCI": "LOCKED",
    "TUNGKU": "HEARTH",
    "TUNGKU TEMPA EMPU KARTA": "EMPU KARTA'S FORGE",
    "Tambah stat": "Raise a stat",
    "Tangkis!": "Parried!",
    "Tapi lesung di Karangtumbal masih berbunyi.": "But in Karangtumbal, the rice mortar still pounds.",
    "Tempa ke +%d": "Forge to +%d",
    "Tempa senjata": "Forge weapon",
    "Terhuyung": "Staggered",
    "Terima kasih sudah bermain.   [Z] kembali ke judul": "Thank you for playing.   [Z] return to title",
    "Timeline (Doubly Linked List), head = giliran:": "Timeline (Doubly Linked List), head = next turn:",
    "Timun Mas  -  Level %d  ->  %d": "Timun Mas  -  Level %d  ->  %d",
    "Tungku yang menyala & terjangkau (BFS Graph dunia):": "Lit Hearths within reach (BFS on the world graph):",
    "Tutup": "Close",
    "Warp": "Warp",
    "Z pilih   X batal   panah gerak": "Z select   X cancel   arrows move",
    "Z: periksa / pilih    X: batal & menu    Shift: berlari": "Z: examine / select    X: cancel & menu    Shift: run",
    "[Z] beli    [X] pergi": "[Z] buy    [X] leave",
    "[Z] pakai   [X] kembali": "[Z] use   [X] back",
    "[Z] pakai   [kiri/kanan] pilih stat   [X] kembali": "[Z] use   [left/right] choose stat   [X] back",
    "[Z] tempa    [X] pergi": "[Z] forge    [X] leave",
    "[Z] tukar sukma    [X] kembali": "[Z] offer souls    [X] back",
    "[Z] tukar sukma   [C] panggil / lepas   [X] kembali": "[Z] offer souls   [C] summon / dismiss   [X] back",
    "akhir dari versi demo": "end of the demo",
    "Buto Ijo tenggelam untuk terakhir kalinya.": "Buto Ijo sinks for the last time.",
    "di sini": "here",
    "habis": "sold out",
    "ketik nama lalu tekan Enter": "type a name, then press Enter",
    "kiri/kanan ganti    X kembali": "left/right change    X back",
    "kiri/kanan pilih    Z lanjut    X kembali": "left/right choose    Z continue    X back",
    "sebuah dongeng tumbal dari tanah Jawa": "a tale of offerings from the land of Java",
    "sisa %d": "%d left",
    "terbuka": "open",
    "rapal": "chant",
    "tunda": "delay",
    "mati": "off",
    "ikut": "with you",
    "menunggu": "waiting",
    "dikunjungi": "visited",
    "belum": "unvisited",

    # ------------------------------------------------------------------ stat & elemen
    "Air": "Water",
    "Angin": "Wind",
    "Api": "Fire",
    "Fisik": "Physical",
    "Gaib": "Unseen",
    "Tanah": "Earth",
    "Gesit": "Agility",
    "Ilmu": "Magic",
    "Tahan": "Defense",
    "Tolak": "Resist",
    "Untung": "Luck",

    # ------------------------------------------------------------------ status
    "Berdarah": "Bleeding",
    "Bisu": "Silence",
    "Cepat": "Haste",
    "Lambat": "Slow",
    "Lumpuh": "Paralysis",
    "Racun": "Poison",
    "Rapuh": "Fragile",
    "Takut": "Fear",
    "Terbakar": "Burning",
    "Tidur": "Sleep",
    "Elak": "Evade",
    "Tangkis": "Parry",
    "BKR": "BRN",
    "BSU": "SIL",
    "CPT": "HST",
    "DRH": "BLD",
    "ELK": "EVA",
    "LBT": "SLW",
    "LMP": "PAR",
    "RCN": "PSN",
    "RPH": "FRG",
    "TDR": "SLP",
    "THN": "GRD",
    "TKS": "PRY",
    "TKT": "FER",

    # ------------------------------------------------------------------ kelas & hadiah awal
    "Pendekar": "Warrior",
    "Maling": "Thief",
    "Pawang": "Shaman",
    "Kuat dan tahan pukul.": "Strong, and hard to bring down.",
    "Gesit; bertindak lebih sering. Membawa sukma curian.": "Nimble; acts more often. Carries stolen souls.",
    "Menguasai ilmu sukma.": "Wields the arts of the soul.",
    "Gadis yang dulu lolos dari Buto Ijo. Kini tak sepenuhnya hidup.": "The girl who once escaped Buto Ijo. No longer wholly alive.",
    "Dua Ramuan Jamu": "Two Jamu Draughts",
    "Sukma Penjaga": "Warden's Soul",
    "Tiga Bom Minyak Jarak": "Three Castor Oil Bombs",
    "Tiga Abu Tungku": "Three Hearth Ashes",
    "Dua ramuan tambahan. Ramuan bawaan tetap terisi ulang di Tungku.": "Two extra draughts. Your own still refill at the Hearth.",
    "Sukma pekat seorang penjaga: 200 sukma untuk naik level lebih awal.": "A warden's dense soul: 200 souls to grow stronger early.",
    "Kendi minyak jarak yang menyala. Kuat melawan makhluk jerami dan serangga.": "Burning jars of castor oil. Strong against straw things and insects.",
    "Abu yang membawamu pulang ke Tungku terakhir kapan saja.": "Ash that carries you back to the last Hearth, whenever you wish.",

    # ------------------------------------------------------------------ barang
    "Ramuan Jamu": "Jamu Draught",
    "Memulihkan separuh HP. Terisi ulang setiap beristirahat di Tungku.": "Restores half of your HP. Refilled whenever you rest at a Hearth.",
    "Daun Penawar": "Antidote Leaf",
    "Menyembuhkan racun dan darah yang tak berhenti mengalir.": "Cures poison and blood that will not stop flowing.",
    "Sukma Tumbal Kecil": "Small Offering's Soul",
    "Sisa sukma seorang anak tumbal. Pecahkan untuk mendapat 40 sukma.": "What remains of a child offering. Crush it to gain 40 souls.",
    "Sukma pekat seorang penjaga lumbung. Pecahkan untuk 200 sukma.": "The dense soul of a granary warden. Crush it for 200 souls.",
    "Bom Minyak Jarak": "Castor Oil Bomb",
    "Kendi minyak jarak yang dilempar dan terbakar. Damage api 90.": "A jar of castor oil, thrown to burst into flame. 90 fire damage.",
    "Abu Tungku": "Hearth Ash",
    "Segenggam abu Tungku. Membawamu kembali ke Tungku terakhir.": "A handful of Hearth ash. Returns you to the last Hearth.",
    "Kunci Sel Berkarat": "Rusted Cell Key",
    "Kunci kecil berkarat dari lumbung Buto.": "A small rusted key from Buto's granary.",
    "Biji Timun Mas": "Timun Mas's Seed",
    "Bekal pertama Timun Mas. Hangat, berdenyut, dan masih lapar akan tanah.": "Timun Mas's first provision. Warm, pulsing, still hungry for soil.",
    "Jarum Timun Mas": "Timun Mas's Needle",
    "Bekal kedua. Sebatang jarum emas yang dulu menumbuhkan hutan bambu.": "The second provision. A golden needle that once grew a bamboo forest.",
    "Garam Timun Mas": "Timun Mas's Salt",
    "Bekal ketiga. Segenggam garam yang dulu menjadi lautan.": "The third provision. A fistful of salt that once became a sea.",
    "Terasi Timun Mas": "Timun Mas's Terasi",
    "Bekal keempat. Bau busuknya dulu menenggelamkan raksasa.": "The fourth provision. Its stench once drowned a giant.",
    "Bijih Besi": "Iron Ore",
    "Bongkah besi hitam dari dasar rawa. Pandai besi desa bisa menempanya.": "A lump of black iron from the mire's floor. The village smith can work it.",
    "Kendi Jamu": "Jamu Jug",
    "Kendi tanah liat kosong. Menambah satu wadah Ramuan Jamu.": "An empty clay jug. Adds one more Jamu Draught flask.",

    # ------------------------------------------------------------------ senjata
    "Golok Penebas": "Hewing Machete",
    "Keris Luk Tujuh": "Seven-Wave Keris",
    "Tongkat Pawang": "Shaman's Staff",
    "Selendang Emas": "Golden Shawl",

    # ------------------------------------------------------------------ jurus
    "Serangan biasa dengan senjata.": "A plain strike with your weapon.",
    "Damage yang diterima berkurang separuh sampai giliran berikutnya.": "Halves the damage you take until your next turn.",
    "Menghindari semua serangan sampai giliran berikutnya.": "Evades every attack until your next turn.",
    "Membalas setiap serangan fisik x1.5 sampai giliran berikutnya.": "Answers every physical attack at x1.5 until your next turn.",
    "Tebas Berat": "Heavy Slash",
    "Ayunan penuh tenaga. Perlu ancang-ancang sebentar.": "A blow with all your weight behind it. Needs a moment to wind up.",
    "Sapuan": "Sweep",
    "Menyapu semua musuh dan menunda giliran mereka.": "Sweeps every enemy and pushes back their turns.",
    "Tusuk Cepat": "Quick Stab",
    "Dua tusukan kilat. Bisa membuat berdarah.": "Two lightning thrusts. May cause bleeding.",
    "Hantaman Guncang": "Concussive Blow",
    "Pukulan ke kepala: menunda giliran target jauh ke belakang.": "A blow to the head: pushes the target's turn far back.",
    "Panah Sukma": "Soul Arrow",
    "Melontarkan sukma sebagai anak panah.": "Looses a soul as an arrow.",
    "Api Tungku": "Hearth Fire",
    "Bara Tungku menyembur ke semua musuh. Rapal lama.": "Hearth embers burst over every enemy. A long chant.",
    "Jampi Sembuh": "Healing Incantation",
    "Memulihkan HP satu kawan.": "Restores the HP of one ally.",
    "Gigit": "Bite",
    "Gigitan kotor yang bisa meracuni.": "A filthy bite that may poison.",
    "Cakar Busuk": "Rotten Claw",
    "Cakaran kuku hitam. Bisa membuat berdarah.": "A rake of black nails. May cause bleeding.",
    "Ludah Racun": "Venom Spit",
    "Menyemburkan lendir beracun.": "Spews poisonous slime.",
    "Raung": "Roar",
    "Raungan yang membuat ciut nyali.": "A roar that withers the courage.",
    "Hantam Tanah": "Ground Slam",
    "Mengangkat gada tinggi-tinggi... lalu menghantam. Bersiaplah!": "Raises the club high... then brings it down. Brace yourself!",
    "Kutuk Lumbung": "Granary Curse",
    "Mantra busuk yang membuat tubuh rapuh.": "A rotten spell that leaves the body fragile.",
    "Lempar Garam": "Salt Throw",
    "Segenggam garam bekal Timun Mas: membakar makhluk gaib dan membuyarkan rapal.":
        "A fistful of Timun Mas's salt: sears unseen things and breaks their chants.",
    "Sulur Timun": "Cucumber Vines",
    "Sulur timun mengikat semua musuh dan memperlambat mereka.": "Cucumber vines bind every enemy and slow them.",
    "Rahang Sangit": "Stinkbug Jaws",
    "Gigitan rahang belalang yang merobek kulit.": "Locust mandibles that tear the skin.",
    "Pecah Busuk": "Rotten Burst",
    "Tubuhnya merekah, menyemburkan getah busuk ke semua lawan.": "Its body splits, spraying rotten sap over every foe.",
    "Cakar Jerami": "Straw Talons",
    "Jari-jari jerami mencakar dua kali.": "Fingers of straw rake you twice.",
    "Tebar Biji Busuk": "Sow Rotten Seeds",
    "Biji timun busuk ditaburkan ke tubuhmu.": "Rotten cucumber seeds are sown into your flesh.",
    "Ritual Panen": "Harvest Rite",
    "Ia merentangkan tangan dan mulai menembang lagu panen... Hentikan sebelum selesai!":
        "It spreads its arms and begins the harvest song... Stop it before it ends!",
    "Lolong Sawah": "Paddy Howl",
    "Lolongan dari dalam labu kering membuat lututmu gemetar.": "A howl from inside the dried gourd sets your knees shaking.",
    "Hujan Jarum": "Needle Rain",
    "Ribuan jarum bambu berjatuhan ke semua lawan.": "A thousand bamboo needles rain upon every foe.",
    "Patuk Bangkai": "Carrion Peck",
    "Paruh hitam mematuk luka yang terbuka.": "A black beak pecks at open wounds.",
    "Ombak Asin": "Brine Wave",
    "Gelombang air garam menghantam semua lawan.": "A wave of brine crashes over every foe.",
    "Belit Tentakel": "Tentacle Coil",
    "Tentakel membelit dan menyeret giliranmu mundur.": "Tentacles coil around you and drag your turn back.",
    "Sabet Jala": "Net Lash",
    "Jala nelayan yang membusuk melilit tubuhmu.": "A rotting fishing net winds around your body.",
    "Lilit Akar": "Root Snare",
    "Akar rawa mengikat kakimu.": "Mire roots bind your legs.",
    "Terkam": "Maul",
    "Rahang bangkai buaya terbuka lebar... lalu mengatup.": "The crocodile carcass opens its jaws wide... then snaps shut.",
    "Raung Buto": "Buto's Roar",
    "Raungan raksasa mengguncang rawa.": "The giant's roar shakes the mire.",
    "Lapar Buto": "Buto's Hunger",
    "Buto Ijo membuka mulutnya lebar-lebar. Ia akan menelanmu! Hentikan, atau elakkan!":
        "Buto Ijo opens its maw wide. It means to swallow you! Stop it, or evade!",
    "Napas Terasi": "Terasi Breath",
    "Napas busuk udang fermentasi menyelimuti semua lawan.": "A breath of fermented shrimp rot smothers every foe.",
    "Cengkeram": "Crushing Grip",
    "Tangan raksasa meremas tubuhmu.": "A giant's hand crushes your body.",
    "Hantam Gada": "Club Smash",
    "Tangan kanan mengangkat gada tinggi-tinggi... Tunda atau buyarkan sebelum jatuh!":
        "The right hand raises its club high... Delay it or break it before it falls!",

    # ------------------------------------------------------------------ musuh & kelompok
    "Tumbal Gagal": "Failed Offering",
    "Tikus Lumbung": "Granary Rat",
    "Belatung Lumbung": "Granary Maggot",
    "Penjaga Lumbung": "Granary Warden",
    "Belalang Sangit": "Stinkbug Locust",
    "Timun Busuk": "Rotten Cucumber",
    "Memedi Sawah": "Memedi of the Paddy",
    "Arwah Jarum": "Needle Wraith",
    "Gagak Bangkai": "Carrion Crow",
    "Ular Bambu": "Bamboo Serpent",
    "Hantu Laut": "Sea Ghost",
    "Tengkorak Nelayan": "Fisherman's Skull",
    "Akar Rawa": "Mire Root",
    "Buaya Bangkai": "Carcass Crocodile",
    "Tangan Kiri": "Left Hand",
    "Tangan Kanan": "Right Hand",
    "Tikus Lumbung x2": "Granary Rat x2",
    "Tumbal & Tikus": "Offering & Rat",
    "Belatung x3": "Maggot x3",
    "Belalang Sangit x2": "Stinkbug Locust x2",
    "Timun & Belalang": "Cucumber & Locust",
    "Timun Busuk x2": "Rotten Cucumber x2",
    "Arwah Jarum x2": "Needle Wraith x2",
    "Ular & Arwah": "Serpent & Wraith",
    "Tengkorak Nelayan x2": "Fisherman's Skull x2",
    "Hantu & Tengkorak": "Ghost & Skull",
    "Akar Rawa x2": "Mire Root x2",
    "Akar & Belatung": "Root & Maggot",

    # ------------------------------------------------------------------ peta
    "Sel Tumbal": "Offering Cell",
    "Lorong Lumbung": "Granary Corridor",
    "Desa Karangtumbal": "Karangtumbal Village",
    "Ladang Timun Busuk": "Rotten Cucumber Fields",
    "Hutan Bambu Jarum": "Needle Bamboo Forest",
    "Laut Garam": "The Salt Sea",
    "Rawa Terasi": "Terasi Mire",
    "Sarang Buto": "Buto's Lair",
    "Sulur timun yang menggeliat menutup jalan. Sesuatu di tengah ladang menahannya.":
        "Writhing cucumber vines bar the way. Something in the middle of the field holds them there.",

    # ------------------------------------------------------------------ pembicara
    # nama diri sama di kedua bahasa (tetap dicatat agar uji cakupan bisa memastikannya)
    "Mbok Srini": "Mbok Srini",
    "Timun Mas": "Timun Mas",
    "Buto Ijo": "Buto Ijo",
    "Empu Karta": "Empu Karta",
    "Lelaki Tua": "Old Man",
    "Perempuan Tua": "Old Woman",
    "Penumbuk Lesung": "Mortar Pounder",
    "Gadis di Dermaga": "Girl on the Pier",
    "Pandai Besi": "Blacksmith",
    "Pedagang Keliling": "Wandering Merchant",
    "Pemuda Desa": "Village Youth",

    # ------------------------------------------------------------------ cerita: lumbung
    "Dingin. Bau timun busuk bercampur anyir darah.": "Cold. The smell of rotten cucumber, and beneath it, blood.",
    "Kamu terbangun di atas jerami basah. Tali ijuk di lehermu putus, ujungnya masih basah oleh sesuatu.":
        "You wake on wet straw. The palm-fibre rope around your neck has snapped, its frayed end still damp with something.",
    "Di luar sel, sesuatu menyeret kakinya pelan-pelan. Lumbung ini bukan tempat menyimpan padi.":
        "Outside the cell, something drags its feet. This granary was never meant for rice.",
    "...Kau bangun? Mereka tidak pernah bangun.": "...You woke? They never wake.",
    "Desa di atas sana menanam kita seperti benih timun. Saat musimnya tiba, Buto Ijo turun memanen.":
        "The village above plants us like cucumber seed. When the season turns, Buto Ijo comes down to harvest.",
    "Masih di sini, Nak? Lumbung ini lapar. Jangan biarkan dia mengunyahmu dua kali.":
        "Still here, child? This granary hungers. Do not let it chew you twice.",
    "Siapa kau?": "Who are you?",
    "Bagaimana aku keluar?": "How do I get out?",
    "Tumbal tiga musim lalu. Buto tidak suka daging tua, jadi aku dibiarkan membusuk di sini.":
        "An offering, three seasons past. Buto has no taste for old meat, so they left me here to rot.",
    "Kadang aku lupa namaku sendiri. Yang kuingat hanya bunyi lesung di desa pada malam panen.":
        "Some days I forget my own name. All I remember is the rice mortar, pounding on harvest nights.",
    "Gerbang lengkung di ujung lorong menuju aula. Di sana ada Tungku: bara yang tidak pernah padam.":
        "The arched gate at the corridor's end leads to the hall. There burns a Hearth: embers that never die.",
    "Beristirahatlah di dekatnya bila tubuhmu remuk. Tapi ingat... setiap kau beristirahat, yang mati di lumbung ini ikut bangun lagi.":
        "Rest beside it when your body breaks. But remember... each time you rest, the dead of this granary rise with you.",
    "Dan bila kau mati, sukmamu tercecer di tempat kau jatuh. Pungut kembali sebelum kau mati lagi.":
        "And should you die, your souls will scatter where you fell. Gather them before you die again.",
    "Ambil ini. Ramuan Jamu. Dulu ada perempuan desa yang selalu menyelipkannya ke tangan tumbal... entah untuk menolong, entah untuk menenangkan hatinya sendiri.":
        "Take this. A Jamu Draught. There was a village woman who always slipped one into an offering's hand... whether to help them, or to quiet her own heart, I cannot say.",
    "Mayat seorang anak, meringkuk menghadap dinding. Jemarinya menggenggam sesuatu.":
        "The corpse of a child, curled toward the wall. Its fingers are clenched around something.",
    "Buka genggamannya": "Open its hand",
    "Biarkan": "Leave it",
    "Jemari kecil itu dingin dan kaku. Kamu membukanya satu per satu.": "The small fingers are cold and stiff. You open them, one by one.",
    "Kamu meninggalkannya. Untuk sekarang.": "You leave it be. For now.",
    "Goresan kuku memenuhi dinding. Sebagian sudah tertutup lumut.": "Fingernail scratches cover the wall. Some are already lost beneath the moss.",
    "\"EMPAT BEKAL. TIMUN, JARUM, GARAM, TERASI. JANGAN BERHENTI BERLARI.\"":
        "\"FOUR PROVISIONS. CUCUMBER, NEEDLE, SALT, TERASI. NEVER STOP RUNNING.\"",
    "Di bawahnya, dengan tulisan lain yang lebih kasar: \"Dia tidak pernah berhenti mengejar.\"":
        "Beneath it, in a cruder hand: \"He never stops chasing.\"",
    "Lantai di depan penuh retakan. Sesuatu yang besar bernapas di kegelapan aula atas.":
        "The floor ahead is riddled with cracks. Something large breathes in the darkness of the upper hall.",
    "Ia mengangkat senjatanya tinggi-tinggi sebelum menghantam. Saat itulah ia lengah.":
        "It raises its weapon high before every blow. That is when it is open.",

    # ------------------------------------------------------------------ cerita: desa
    "...Kau keluar dari lumbung. Belum pernah ada yang keluar dari lumbung.": "...You came out of the granary. No one has ever come out of the granary.",
    "Jangan berdiri di situ seperti hantu. Duduklah dekat Tungku. Baranya kunyalakan dari kayu nangka dan... yang lain.":
        "Don't stand there like a ghost. Sit by the Hearth. I lit its embers with jackfruit wood and... other things.",
    "Tungku ini tidak pernah padam. Kalau padam, desa ini akan ingat semua yang sudah dilakukannya.":
        "This Hearth never goes out. If it ever did, this village would remember everything it has done.",
    "Bajumu bau jerami terbakar. Kau membakar penjaga ladang itu?": "Your clothes reek of burnt straw. You put the field's guardian to the torch?",
    "Aku yang menjahit kepalanya dari labu, dulu, waktu desa masih percaya ia hanya untuk menakuti burung.":
        "I stitched its head from a gourd, long ago, back when the village still believed it was only for frightening birds.",
    "Kau bertemu dengannya. Jangan bohong, aku bisa mencium garam di rambutmu.": "You met her. Don't lie to me. I can smell the salt in your hair.",
    "...Apa ia masih membenciku?": "...Does she still hate me?",
    "Ia sudah memaafkanmu": "She has forgiven you",
    "Ia tidak menyebut namamu": "She never spoke your name",
    "Bohong. Tapi terima kasih. Sudah lama tidak ada yang mau berbohong demi aku.":
        "A lie. But thank you. It has been a long time since anyone cared enough to lie for me.",
    "...Ya. Itu lebih mirip anakku.": "...Yes. That sounds more like my daughter.",
    "Tentang Buto Ijo": "About Buto Ijo",
    "Tentang Tungku": "About the Hearth",
    "Orang memanggilku Mbok Srini. Dulu aku punya seorang anak perempuan. Kulitnya kuning keemasan, seperti timun yang matang di pohon.":
        "They call me Mbok Srini. I had a daughter once. Her skin was gold, like a cucumber ripened on the vine.",
    "Ia lari, membawa empat bekal yang kuselipkan ke tangannya. Sejak itu desa ini membayar larinya dengan anak-anak lain.":
        "She ran, carrying the four provisions I pressed into her hands. Ever since, this village has paid for her escape with other children.",
    "Jangan menatapku begitu. Aku tidak meminta maaf kepada tumbal.": "Don't look at me like that. I do not apologise to offerings.",
    "Orang bilang ia tenggelam di rawa terasi. Raksasa tidak tenggelam, Nak. Mereka hanya lapar lebih lama.":
        "They say he drowned in the terasi mire. Giants do not drown, child. They simply stay hungry longer.",
    "Ladang di timur tumbuh dari biji timun yang dilempar anakku. Tanahnya masih ingat rasa daging. Hutan bambu di selatan dulunya segenggam jarum.":
        "The field to the east grew from the cucumber seed my daughter threw. Its soil still remembers the taste of flesh. The bamboo forest to the south was once a handful of needles.",
    "Beristirahatlah di sana bila tubuhmu remuk. Ramuanmu akan terisi, lukamu menutup.":
        "Rest there when your body breaks. Your draughts will refill, your wounds will close.",
    "Bawa sukma yang kau rampas ke bara itu. Tungku akan menukarnya menjadi tenaga. Dan Tungku yang sudah menyala akan saling memanggil, selama jalannya tidak tertutup.":
        "Bring the souls you take to those embers. The Hearth will turn them into strength. And lit Hearths call to one another, so long as the road between them is open.",
    "Tok... tok... tok... Lesung tidak boleh berhenti. Kalau sepi, ia tahu ada yang lari.":
        "Tok... tok... tok... The mortar must never stop. If it falls silent, he knows someone has run.",
    "Kau... kau seharusnya sudah di perutnya. Ambil ini dan pergilah sebelum orang lain melihatmu.":
        "You... you should be in his belly by now. Take this, and go before anyone else sees you.",
    "Tok... tok... Jangan bicara padaku. Anakku dipanen dua musim lalu. Aku masih menumbuk untuknya.":
        "Tok... tok... Don't speak to me. My child was harvested two seasons ago. I still pound for him.",
    "Jalan setapak ke timur menuju Ladang Timun Busuk. Bau manis yang membusuk terbawa angin dari sana.":
        "The footpath east leads to the Rotten Cucumber Fields. A sweet, rotting smell drifts in on the wind.",
    "Di selatan, batang-batang bambu berdiri rapat seperti jarum yang ditancapkan ke tanah.":
        "To the south, bamboo stands packed tight, like needles driven into the earth.",
    "Terlalu rapat untuk ditembus dari sisi ini.": "Too dense to break through from this side.",
    "Jangan lihat palunya. Lihat tanganmu sendiri. Ada bekas tali di sana, bukan?":
        "Don't look at the hammer. Look at your own hands. There are rope marks there, aren't there?",
    "Aku yang menempa rantai lumbung. Tiga puluh musim, tiga puluh anak. Bayarannya beras dan diam.":
        "I forged the granary's chains. Thirty seasons, thirty children. I was paid in rice, and in silence.",
    "Bawakan sukma dan bijih besi dari tanah busuk itu. Akan kutempa senjatamu. Anggap saja aku sedang mencicil.":
        "Bring me souls, and iron ore from that rotten land. I will forge your weapon. Call it a debt I am paying off.",
    "Besi yang baik mengingat setiap pukulan. Begitu juga desa ini.": "Good iron remembers every blow. So does this village.",
    "Pembeli! Sudah lama sekali. Orang desa tidak berbelanja, mereka cuma berdoa dan menumbuk.":
        "A customer! It's been so long. The villagers don't buy anything. They only pray, and pound.",
    "Barangku dari mana? Dari kantong-kantong yang tidak akan dipakai lagi. Tumbal tidak butuh bekal, kan?":
        "Where do my wares come from? From pockets that won't be needing them. Offerings have no use for supplies, do they?",
    "Sukma, sukma. Di desa ini cuma itu yang laku.": "Souls, souls. In this village, that's the only coin that counts.",

    # ------------------------------------------------------------------ cerita: ladang
    "Ladang timun terbentang sampai ke kegelapan. Batang-batangnya merambat di ajir bambu seperti urat.":
        "The cucumber field stretches into the dark. Its vines climb the bamboo stakes like veins.",
    "Buahnya terlalu besar. Sebagian merekah, dan dari dalamnya terdengar sesuatu bernapas.":
        "The fruit has grown too large. Some have split open, and from within, something breathes.",
    "Seorang petani duduk bersandar di ajir, caping menutupi wajahnya. Ia sudah lama berhenti bernapas.":
        "A farmer sits against a stake, his straw hat over his face. He stopped breathing long ago.",
    "Di pangkuannya ada sabit berkarat dan secarik kain bertuliskan: \"Jangan panen sebelum ia bernyanyi. Kalau ia bernyanyi, pukul kepalanya.\"":
        "In his lap lie a rusted sickle and a scrap of cloth: \"Do not harvest before it sings. If it sings, strike its head.\"",
    "Orang-orangan sawah itu tidak bergerak. Lalu labu keringnya berputar menghadapmu.":
        "The scarecrow does not move. Then its dried gourd of a head turns to face you.",
    "...Tumbal lepas... dari lumbung... Ladang lapar... ladang selalu lapar...":
        "...An offering... loose from the granary... The field hungers... the field always hungers...",
    "Biji yang dilempar si gadis emas... AKU yang menjaganya. Kembalikan dagingmu ke tanah.":
        "The seed the golden girl threw... I am its keeper. Return your flesh to the soil.",
    "Jerami terbakar habis. Di antara abunya, sesuatu berkilau keemasan.": "The straw burns away. Among the ashes, something glints gold.",
    "Sebutir biji timun, hangat seperti baru dipetik. Ia berdenyut pelan di telapak tanganmu.":
        "A single cucumber seed, warm as if freshly picked. It pulses faintly in your palm.",
    "Di ujung timur ladang, sulur-sulur timun layu dan rebah. Jalan ke hutan bambu terbuka.":
        "At the field's eastern edge, the cucumber vines wither and fall. The way to the bamboo forest lies open.",
    "Bekal pertama: Biji Timun Mas": "First provision: Timun Mas's Seed",
    "Di balik ladang, bambu-bambu tumbuh rapat dan tajam. Hutan Bambu Jarum menunggu.":
        "Beyond the field, bamboo grows dense and sharp. The Needle Bamboo Forest awaits.",
    "Angin dari sana berbau besi dan getah.": "The wind from there smells of iron and sap.",
    "Jalan setapak tertutup sulur timun yang menggeliat. Sesuatu di tengah ladang menahannya.":
        "The path is choked with writhing cucumber vines. Something in the middle of the field holds them there.",

    # ------------------------------------------------------------------ cerita: hutan bambu, laut, rawa, sarang
    "Bambu-bambu di sini tumbuh terlalu rapat, terlalu lurus, seperti jarum yang ditancapkan dari langit.":
        "The bamboo here grows too close, too straight, like needles driven down from the sky.",
    "Angin lewat di antara batangnya dan berbisik dengan suara banyak orang.": "The wind passes between the stalks and whispers in many voices.",
    "Di balik rumpun bambu ini terdengar suara lesung desa. Batang-batangnya bisa ditebas.":
        "Beyond this thicket you can hear the village mortar. The stalks could be cut down.",
    "Tebas": "Cut through",
    "Kamu menebas bambu sampai tanganmu berdarah. Cahaya Tungku desa terlihat di ujung jalan.":
        "You hack at the bamboo until your hands bleed. The glow of the village Hearth appears at the end of the path.",
    "Gundukan tanah kecil dikelilingi bambu yang tumbuh melingkar. Di puncaknya menancap sebatang jarum emas.":
        "A small mound of earth ringed by bamboo grown in a circle. A golden needle is driven into its peak.",
    "Dulu jarum ini dilempar seorang gadis yang berlari. Seluruh hutan tumbuh dari lukanya.":
        "A fleeing girl once threw this needle. The whole forest grew from the wound it made.",
    "Bekal kedua: Jarum Timun Mas": "Second provision: Timun Mas's Needle",
    "Laut ini tidak berombak. Airnya pekat, asin sampai pedih di mata, dan tidak ada satu pun burung di atasnya.":
        "This sea has no waves. Its water is thick, salt enough to sting the eyes, and not a single bird flies above it.",
    "Di ujung dermaga busuk, seseorang berdiri memandangi air.": "At the end of the rotting pier, someone stands staring at the water.",
    "...Kau juga lari dari lumbung? Aku bisa mencium bau jerami dan darah di tubuhmu.":
        "...You ran from the granary too? I can smell straw and blood on you.",
    "Dulu aku melempar segenggam garam ke tanah, dan tanah berubah menjadi laut ini. Raksasa itu tenggelam... lalu merangkak keluar lagi.":
        "I once threw a fistful of salt upon the earth, and the earth became this sea. The giant drowned... and then crawled out again.",
    "Namaku Timun Mas. Desa memujaku sebagai anak yang selamat. Mereka tidak tahu, sejak hari itu aku tidak pernah benar-benar hidup.":
        "My name is Timun Mas. The village worships me as the child who survived. They do not know that since that day, I have never truly lived.",
    "Ikutlah denganku": "Come with me",
    "Kenapa kau di sini?": "Why are you here?",
    "Menunggu. Selama Buto lapar, desa akan terus menanam anak-anak. Aku sudah terlalu lelah untuk lari lagi, tapi belum cukup berani untuk kembali.":
        "Waiting. As long as Buto hungers, the village will keep planting children. I am too tired to run again, and not yet brave enough to go back.",
    "Kau punya bekal-bekalku. Biji, jarum... Aku akan ikut. Panggil aku di Tungku bila kau membutuhkanku.":
        "You carry my provisions. The seed, the needle... I will come. Call for me at a Hearth when you need me.",
    "Timun Mas bergabung. Bekal ketiga: Garam Timun Mas": "Timun Mas joins you. Third provision: Timun Mas's Salt",
    "Pintu rumah nelayan terkunci dari dalam. Lewat celah papan tercium bau ikan asin yang sudah lama menjadi debu.":
        "The fisherman's door is barred from within. Through the gaps in the planks drifts the smell of salted fish long since turned to dust.",
    "Di bawah tangga terselip sebuah kendi. Isinya garam kasar dan beberapa butir sukma yang menggumpal seperti kristal.":
        "A clay jug is tucked beneath the steps. Inside: coarse salt, and a few souls clotted together like crystal.",
    "Bau udang membusuk menusuk hidung. Lumpur rawa ini hangat, dan sesekali bergelembung seperti bernapas.":
        "The stench of rotting shrimp stings your nose. The mud of this mire is warm, and now and then it bubbles, as if breathing.",
    "Di sinilah aku melempar bekal terakhir. Sarangnya ada di selatan. Sekali turun, tidak ada jalan kembali sampai salah satu dari kita mati.":
        "This is where I threw the last provision. His lair lies to the south. Once we descend, there is no way back until one of us is dead.",
    "Jangan mendekat! ...Oh. Kau bukan dia. Kau cuma tumbal.": "Stay back! ...Oh. You're not him. You're only an offering.",
    "Kepala desa menyuruhku memastikan Buto masih tidur. Kalau ia bangun sebelum musim panen, desa harus mengirim dua anak, bukan satu.":
        "The headman sent me to make sure Buto still sleeps. If he wakes before the harvest, the village must send two children, not one.",
    "Bau garam itu... Kau bertemu gadis di dermaga? Kakekku bilang ia sudah mati tujuh musim lalu. Di desa, namanya kami nyanyikan setiap panen.":
        "That smell of salt... You met the girl on the pier? My grandfather said she died seven seasons ago. In the village, we sing her name every harvest.",
    "Kau masih di sini? Sarangnya di tenggara, di ujung jalan yang menurun. Sekali turun, lumpurnya tidak akan membiarkanmu naik lagi.":
        "You're still here? The lair is to the southeast, where the path slopes down. Once you descend, the mud won't let you climb back.",
    "Kenapa tidak lari saja?": "Why not just run?",
    "Lari ke mana? Ladang, hutan bambu, laut, rawa ini... semuanya tumbuh dari bekal gadis itu. Semua tanah di sekeliling desa adalah jejak larinya.":
        "Run where? The field, the bamboo, the sea, this mire... all of it grew from that girl's provisions. Every land around the village is the trail of her flight.",
    "Tidak ada jalan keluar dari sebuah pelarian. Hanya jalan yang lebih panjang.": "There is no way out of an escape. Only a longer road.",
    "...Ini. Kalau kau memang mau turun, kau lebih butuh daripada aku.": "...Here. If you really mean to go down there, you need this more than I do.",
    "Sebongkah terasi kering terbungkus daun pisang, tidak tersentuh lumpur di sekitarnya.":
        "A block of dried terasi wrapped in banana leaf, untouched by the mud around it.",
    "Bekal keempat: Terasi Timun Mas": "Fourth provision: Timun Mas's Terasi",
    "Kamu tergelincir menuruni lereng lumpur. Di atas, jalan kembali tertutup.": "You slide down a slope of mud. Above you, the way back closes.",
    "Kolam terasi di tengah sarang bergolak. Sesuatu yang sangat besar sedang tidur di bawahnya.":
        "The terasi pool at the heart of the lair churns. Something very large sleeps beneath it.",
    "Tulang-tulang kecil ditumpuk rapi seperti padi di lumbung. Terlalu rapi untuk pekerjaan seekor raksasa.":
        "Small bones, stacked as neatly as rice in a granary. Far too neat to be the work of a giant.",
    "Di puncaknya terikat gelang benang merah, sama dengan yang dulu diikatkan ke pergelanganmu sebelum kau dibawa ke lumbung.":
        "At the top is tied a bracelet of red thread, the same one they tied around your wrist before they carried you to the granary.",
    "Desa tidak hanya menyerahkan anak-anak. Desa menyusun mereka.": "The village did not merely give up its children. It arranged them.",
    "Kolam terasi terbelah. Kepala hijau sebesar rumah bangkit, disusul dua tangan yang mencengkeram tepi lumpur.":
        "The terasi pool splits apart. A green head as large as a house rises, followed by two hands that seize the edge of the mud.",
    "HMMMM... BAU TIMUN... BAU TUMBAL YANG LEPAS DARI LUMBUNGKU...": "HMMMM... I SMELL CUCUMBER... I SMELL AN OFFERING LOOSE FROM MY GRANARY...",
    "DAN KAU, GADIS EMAS. SUDAH KUBILANG, TIDAK ADA YANG LARI DARIKU SELAMANYA.": "AND YOU, GOLDEN GIRL. I TOLD YOU. NO ONE RUNS FROM ME FOREVER.",
    "Aku tidak lari lagi. Kali ini aku yang datang.": "I am not running anymore. This time, I came to you.",
    "DESA BERJANJI MEMBERIKANMU KEPADAKU. AKU HANYA MENAGIH JANJI.": "THE VILLAGE PROMISED YOU TO ME. I ONLY COLLECT WHAT IS OWED.",
    "Raksasa itu tenggelam untuk terakhir kalinya. Kolam terasi mengering menjadi kerak hitam.":
        "The giant sinks for the last time. The terasi pool dries into a black crust.",
    "...Sudah. Untuk pertama kalinya sejak musim itu, aku tidak mendengar langkahnya.":
        "...It's over. For the first time since that season, I cannot hear his footsteps.",
    "Pulanglah ke desa, Tumbal. Katakan pada Mbok Srini: tidak ada lagi yang perlu ditanam.":
        "Go home to the village, Offering. Tell Mbok Srini: no one else needs to be planted.",
    "Tapi di kejauhan, lesung desa masih berbunyi. Tok... tok... tok...": "But in the distance, the village mortar still pounds. Tok... tok... tok...",
}

if __name__ == '__main__':
    out = {'_catatan': 'Dibuat tools/lang_en.py; jangan disunting langsung. Kunci = teks Indonesia di kode/data/skrip.'}
    out.update(dict(sorted(T.items())))
    path = os.path.join(ROOT, 'assets', 'lang', 'en.json')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    print('en.json:', len(T), 'entri')
