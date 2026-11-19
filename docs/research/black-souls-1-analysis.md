# Bedah Sistem Gameplay Black Souls 1 (bahan desain v2)

> Sumber: data RPG Maker VX Ace di `D:\Game_From_Dev\Black Souls 1\Data` (versi terjemahan Indonesia),
> dibaca dengan parser Ruby Marshal buatan sendiri (`Scripts`, `Actors`, `Classes`, `Skills`, `States`,
> `Items`, `Weapons`, `Armors`, `Enemies`, `Troops`, `System`, `MapInfos`, `CommonEvents`, `Map*.rvdata2`).
> Yang diambil **hanya sistem dan angka desain**. Tidak ada aset, teks cerita, dialog, atau peta yang disalin.
> Konten dewasa (Graphics/Pictures dan event "scene") dilewati.

---

## 0. Ringkasan satu paragraf

Black Souls 1 adalah **JRPG RPG Maker standar yang dibungkus loop Dark Souls**. Pertarungannya sederhana
(menu Attack / Skill / Guard / Item / Escape, angka damage besar, musuh mati dalam 2–4 pukulan), tetapi
semua yang ada di luar battle diambil dari Souls: **bonfire** sebagai checkpoint, **soul** sebagai satu-satunya
mata uang sekaligus EXP, **kehilangan soul saat mati** dan bisa diambil kembali di lokasi kematian, **flask
penyembuh** yang terisi ulang hanya di bonfire, **musuh hidup kembali** setiap istirahat, dan **musuh terlihat di
peta** (bukan random encounter) yang bisa dihindari. Dunianya adalah hub kecil yang bercabang ke beberapa
wilayah, masing-masing berupa rangkaian 3–5 peta yang diakhiri boss. Rasa "wow"-nya berasal dari atmosfer
(fog, cuaca, musik per area), kepadatan isi peta (loot berkilau di mana-mana, NPC aneh, jalan pintas), dan
cerita dongeng yang dipelintir, **bukan** dari kerumitan battle.

---

## 1. Battle

### 1.1 Format
- **Front-view** (tampilan seperti Dragon Quest/Final Fantasy awal): musuh digambar di atas battleback,
  party tidak digambar, hanya muncul di jendela status dengan potongan wajah.
- Resolusi layar dinaikkan ke **640×480** (default VX Ace 544×416).
- Party maksimal 4 (pemain + pendamping yang dipanggil).

### 1.2 Sistem giliran: ATB mode "wait" (sebenarnya terasa seperti CTB)
Bukan turn-based murni ala RPG Maker default, melainkan skrip ATB pihak ketiga:
- Setiap battler punya gauge **AP 0 → 4000**. Tiap frame AP bertambah `AGI + 10` (minimal 5).
  Contoh: AGI 40 → giliran tiap ±80 frame. **AGI langsung menentukan seberapa sering kamu bertindak.**
- Gauge hanya berjalan ketika tidak ada yang sedang memilih perintah atau beraksi → pemain tidak pernah
  dikejar waktu. Efeknya sama dengan CTB (Conditional Turn Battle): urutan giliran ditentukan kecepatan.
- AP awal acak: normal 30–70%, **preemptive** (menyentuh musuh dari belakang) 40–70%, **surprise**
  (disergap) 0–10%. Gagal kabur → AP direset ke 0–10%.
- **Chant (waktu rapal)**: skill dengan `speed` negatif (mis. sihir tingkat II: −5 s.d. −40) tidak langsung keluar;
  battler masuk status merapal, ada gauge kedua, baru menembak saat penuh. Sihir kuat = lambat.
- **Manipulasi AP lewat status**: ada status yang mengurangi/men-set AP target (sapuan, serangan stun),
  status yang **membatalkan rapalan** (magic cancel, petrify), plus aksesori yang menahan efek itu.
- Gauge AP musuh ditampilkan semi-transparan di bawah sprite musuh → pemain bisa membaca "siapa
  bergerak berikutnya".
- Tahan tombol konfirmasi/batal untuk mempercepat animasi battle 4×.

**Pelajaran:** tampilan sesederhana JRPG, tapi kedalamannya ada di **urutan giliran yang terlihat dan bisa
dimanipulasi** (speed, chant, stun, cancel). Ini cocok sekali dengan struktur data antrean/linked list.

### 1.3 Perintah
`Attack`, `Skill` (dua tipe: **Mastery** = teknik fisik, **Sorcery** = sihir; keduanya memakai MP),
`Guard`, `Item`, `Escape`. Party command ("Fight/Escape") dilewati, langsung ke perintah per aktor.

### 1.4 Rumus damage
- Serangan fisik: `a.atk*4 − b.def*2` (variance 20%, bisa kritikal).
- Sihir dasar: `200 + a.mat*5 − b.mdf*2`; tier II: `500 + …` tetapi lebih lambat (chant).
- Heal: `250 + mat*2`, `1000 + mat*3`, `4000 + mat*6`.
- Angka besar sengaja: HP pemain awal ±390, musuh pertama 85–200 HP. Damage per pukulan ±50–90.
  **Musuh biasa mati dalam 2–3 pukulan, pemain bisa menahan 4–5 pukulan.** Battle biasa selesai < 30 detik.
- Elemen: fisik, serap, api, es, petir, air, tanah, angin, suci, gelap (+2 khusus). Musuh punya kelemahan
  ×2 (mis. serangga lemah api). Ini satu-satunya "puzzle" di battle biasa.

### 1.5 Skill pembeda rasa Souls
- **Evade** (MP 3): 2 giliran evasion 100% terhadap fisik dan sihir.
- **Parry** (MP 5): 2 giliran counter-attack 100% dengan damage serangan balik ×1,5.
- Keduanya dimiliki semua kelas sejak level 1. Jadi "dodge/parry" Souls diterjemahkan menjadi **stance
  reaktif berbiaya MP**. Membaca gauge musuh → pasang Parry tepat sebelum musuh bergerak.
- Status buff elemental pada senjata (resin api/es/petir) memberi elemen pada serangan biasa.
- Ada fitur "guts" (bertahan di HP 1), auto-revive (aksesori/status), dan "healing inversion"
  (status yang membalik penyembuhan jadi damage). Bumbu, bukan inti.

### 1.6 Status (dipakai di musuh dan pemain)
Racun (−5% HP/giliran, tetap setelah battle), Bleed (−5%/giliran + lemah fisik ×2), Burn (−10%, 2 giliran),
Blind (hit −60%), Silence (segel sihir), Confuse, Sleep (hilang saat kena damage), Paralyze, Stun (2–3 giliran),
Freeze, Fear (AGI ×0,3), Fragile (damage diterima ×2), Weakened (tidak bisa disembuhkan), Haste/Slow
(AGI ×2/×0,5). Buff/debuff parameter bertumpuk 2 level, durasi 5–10 giliran.

### 1.7 AI musuh
Pola aksi database biasa (skill + kondisi + rating) ditambah skrip "AI cukup cerdas": kondisi HP/MP/status
dievaluasi terhadap **seluruh grup musuh**, jadi musuh pendukung menyembuhkan/menguatkan rekan yang
butuh. Boss tertentu memakai skrip giliran khusus (aba-aba "charge" → serangan besar yang bisa dihindari
dengan Evade/Parry/Guard di giliran berikutnya).

### 1.8 Boss
- Boss multi-bagian (kepala, dua lengan, dua kaki, zirah sebagai target terpisah dengan peran berbeda).
- Boss sering diberi telegraf satu giliran sebelum serangan besar.
- Pertarungan scripted di awal yang memang dirancang untuk kalah (tutorial cerita), dengan opsi dialog
  sebelum battle.

---

## 2. Stat & progresi

### 2.1 Parameter
Delapan parameter standar: MaxHP, MaxMP, ATK, DEF, MAG, RES, SPD (AGI), LCK. Tidak ada stamina, tidak ada
TP (TP disembunyikan). MP berperan sebagai "stamina" teknik fisik maupun sihir.

### 2.2 Kelas awal (dipilih di karakter-kreasi)
Tiga varian kelas yang hampir sama, beda kecil di stat awal:

| Kelas | HP | MP | ATK | DEF | MAG | RES | SPD | Bonus |
|---|---|---|---|---|---|---|---|---|
| Ksatria | 386 | 100 | 16 | 16 | 10 | 10 | 40 | — |
| Maling | 386 | 100 | 10 | 10 | 10 | 10 | 50 | +1000 soul |
| Penyihir | 386 | 120 | 10 | 10 | 16 | 16 | 40 | Soul Arrow |

Kurva tumbuh sama (Lv 30 ≈ 1900 HP, Lv 99 ≈ 5500 HP). Pilihan kelas **hanya mewarnai jam pertama**;
identitas build selanjutnya datang dari senjata, item stat permanen, dan pendamping.

### 2.3 Soul = mata uang = EXP
- Musuh memberi **0 EXP**; yang diberi hanya soul (field "gold").
- Level up dilakukan di NPC/bonfire: pemain mengetik jumlah soul yang mau dikonversi → soul dikurangi,
  EXP bertambah 1:1, level naik mengikuti kurva. Jadi **satu sumber daya untuk belanja dan naik level** →
  setiap keputusan belanja terasa mahal.
- Item "soul" (konsumsi) bisa dipecah jadi soul; item soul berwarna menaikkan **parameter permanen**
  (mis. +3 ATK, +30 HP) → kustomisasi stat terjadi lewat item, bukan alokasi poin.
- Level dan stat bisa menembus batas 99 (fitur pasca-tamat).

### 2.4 Senjata & perlengkapan
- Slot: senjata, perisai, kepala, badan, aksesori (+slot tambahan lewat skrip).
- 10 tipe senjata, masing-masing satu **identitas mekanik kecil**:
  kapak (ATK tinggi, hit −10%, lambat), cakar (pukul 2×, hit +4%), tombak, pedang (netral), katana (ATK tinggi,
  hit −8%), busur (kritikal +3%, lebih cepat), pisau (kritikal + evasi kecil), gada, tongkat (+MAG), senapan.
- Tiap tipe punya 6 tier (+ATK 15 → 110 untuk kapak).
- **Upgrade senjata**: tukar 3 bijih (ore) + senjata tier-N → senjata tier-(N+1) di pandai besi. Ada 4 ukuran
  bijih untuk rentang tier berbeda.
- Ada juga crafting (resep = soul + bahan) lewat skrip sintesis item.

### 2.5 Pendamping (party)
- Pemain mulai sendirian. Pendamping didapat lewat cerita dan **dipanggil** ke party (lalu bisa dilepas).
- Pendamping naik level dengan **diberi soul** oleh pemain → bersaing dengan level pemain sendiri.
- Tiap pendamping punya kelas unik dan 1–2 skill khas.

### 2.6 New Game+
Variabel tingkat kesulitan mengalikan stat, soul, dan drop musuh. Opsi pengulangan, bukan inti demo.

---

## 3. Loop Souls di luar battle

| Sistem | Cara kerja di BS1 | Catatan desain |
|---|---|---|
| **Bonfire** | Disentuh → HP/MP/status pulih penuh, flask diisi ulang, **semua musuh biasa hidup kembali**, posisi disimpan sebagai titik respawn, bonfire tercatat sebagai tujuan warp. Menu: Warp / Pergi. | Jantung loop. Warp hanya ke bonfire yang sudah ditemukan. |
| **Herb Flask** | Item penyembuh 50% HP (varian MP). Jumlahnya diset ulang ke nilai maksimum setiap istirahat. Maksimum bisa ditambah. | Membuat eksplorasi = manajemen sumber daya. |
| **Mati** | Semua soul jatuh di lokasi kematian (map + x + y dicatat), counter kematian +1, pemain kembali ke bonfire terakhir, musuh hidup kembali. | Hukuman jelas tapi bisa ditebus. |
| **Ambil kembali soul** | Penanda muncul di lokasi kematian; disentuh → soul kembali. Mati lagi sebelum mengambil → hilang permanen. | Ketegangan "run" kedua. |
| **Item kembali** | Bubuk tulang: teleport ke bonfire terakhir. "Abu hitam": teleport ke hub, tapi semua soul hilang. | Jalan keluar darurat dengan harga. |
| **Save di mana saja** | Menu Save tidak pernah dinonaktifkan (tidak ada satu pun perintah "disable save" di seluruh peta/event), 16 slot, dan kematian **tidak** menulis ke save. Pemain bisa save sebelum boss, lalu kalau mati: Game End → Judul → Continue, soul utuh kembali. | Hukuman mati keras di atas kertas, tapi pemain punya "jaring pengaman" yang mereka pilih sendiri. Dipertahankan di v2. |
| **Stealth** | Item yang membuat musuh tidak bereaksi selama N langkah. | Opsi menghindari battle. |
| **Pintu & jalan pintas** | Kunci, tuas, pintu sel; area kembali terhubung ke hub. | Rasa "dunia saling tersambung". |

---

## 4. Eksplorasi & encounter

### 4.1 Symbol encounter (musuh terlihat)
- Musuh berupa sprite di peta, berkeliaran acak dengan kecepatan rendah.
- Bereaksi bila pemain dalam **3 tile (5 tile kalau berlari)**: muncul ikon "!", lalu mengejar lebih cepat.
- Arah kontak menentukan awal battle: **menyentuh dari belakang = preemptive**, disentuh dari belakang =
  surprise, selain itu normal.
- Sprite musuh memudar bila pemain menjauh > 5 tile (atmosfer dan petunjuk jarak).
- Grup musuh yang muncul diambil dari daftar encounter peta → satu sprite bisa berarti beberapa formasi.
- Musuh yang sudah dikalahkan hilang sampai bonfire berikutnya.
- Random encounter tidak dipakai di area utama.

### 4.2 Gerak
Gerak **8 arah**, tombol lari, suara langkah berbeda per jenis lantai, fog/kabut bergerak per peta, cuaca
(hujan, salju) per area, nama area muncul saat masuk.

### 4.3 Isi peta
- Kilau loot (`!Flame`) tersebar rapat: di peta tutorial 60×62 tile ada ±10 kilau, di area utama 15–40.
- Item yang diambil memunculkan pop-up "didapat" kecil, tidak menghentikan gerak.
- Peta area 50–80 tile per sisi, dengan 20–100 event: transfer, loot, musuh, NPC, tuas, jebakan.
- Peta khusus berukuran kecil (rumah, ruang boss 20–35 tile).

---

## 5. Struktur dunia

### 5.1 Topologi
```
                [Hub: Hutan Suci]  ← toko, pandai besi, NPC pendamping, bonfire pusat
                 /    |     |    \
            Utara   Timur  Selatan  Barat        (4 wilayah, dipilih bebas setelah tutorial)
              |       |       |       |
         Area A → A1 → A2 → A3 (BOSS)            (tiap area: 3–5 peta berurutan + 1 bonfire di tengah)
         Area B → B1 → B2 (BOSS)
         ...
```
- ±25 area bernama, tiap area = rantai 2–7 peta, peta terakhir berlabel BOSS.
- Bonfire biasanya di peta pertama/tengah setiap area (ditandai ● di pohon peta).
- Beberapa area membuka jalur ke area lain di wilayah berbeda (dunia tidak murni pohon, ada siklus).
- Rute cerita alternatif (rute C/D) memakai sebagian peta yang sama dengan kondisi berbeda.

### 5.2 Awal game (urutan aktual)
1. **Judul → karakter-kreasi**: input nama, pilih kelas (3), pilih **hadiah awal** (4 opsi: kunci serba guna,
   cincin regen, item misterius, item heal penuh), opsi **lewati tutorial**.
2. **Penjara bawah tanah (tutorial, ±5 menit)**: bangun di sel, ±10 kilau loot (item soul, bubuk siluman,
   item stat), adegan pendek NPC ksatria vs algojo, **satu musuh terlihat** sebagai battle pertama.
3. **Peta luar (70×70)**: 4 musuh terlihat dari daftar encounter, beberapa herba, jalan ke benteng.
4. **Benteng**: dialog dengan pilihan (berjanji membantu / diam), lalu **battle melawan boss yang dirancang
   untuk kalah** → cerita membawa pemain ke **hub**.
5. Di hub: bonfire, toko, pandai besi, NPC; dari sini empat wilayah terbuka.

Pelajaran: dalam ±10 menit pemain sudah melihat loot, satu battle mudah, pilihan dialog, boss dramatis,
dan hub. **Tidak ada layar tutorial teks panjang**; sistem diajarkan lewat situasi.

---

## 6. Layout UI

### 6.1 Battle (640×480)
```
┌──────────────────────────────────────────────────────────┐
│ [log 1 baris: "X menyerang!" / nama skill]               │
│                                                          │
│          (battleback penuh layar)                        │
│        [musuh1]      [musuh2]       [musuh3]             │
│        ▬▬HP▬▬        ▬▬HP▬▬         ▬▬HP▬▬    ← bar HP   │
│        ░AP░░         ░AP░░          ░AP░░     ← gauge AP │
│                                                (semi-transparan)
├────────────┬─────────────────────────────────────────────┤
│ Attack     │ [wajah 96×24] Nama  ▣▣ ikon status          │
│ Skill      │   HP ▬▬▬▬▬ 386/386   MP ▬▬▬ 100   AP ▬▬ 75%  │
│ Guard      │ [wajah] Pendamping …                        │
│ Item       │                                             │
│ Escape     │                                             │
└────────────┴─────────────────────────────────────────────┘
  kiri bawah: perintah (lebar 128, 4–8 baris)   kanan: status party
```
- Perintah aktor di kiri bawah, status party di kanan; wajah dipotong jadi strip 96×24 di baris status.
- HP musuh ditampilkan sebagai bar (tanpa angka) di bawah sprite; gauge AP musuh tipis di atasnya.
- Angka damage dilaporkan lewat log battle bawaan VX Ace (tidak ditemukan skrip damage pop-up),
  layar bergetar saat kena pukul, SE khusus saat kritikal, kabut tipis bergerak di battleback.
  → Peluang v2: damage pop-up dan animasi serangan sudah langsung terasa lebih "hidup" dari BS1.

### 6.2 Peta & menu
- Peta: tidak ada HUD permanen (bersih), nama area muncul sebentar saat masuk, pop-up item kecil.
- Menu: daftar perintah standar (Item, Skill, Equip, Status, Save, dll.) dengan variabel soul ditampilkan.
- Dialog: kotak pesan bawah dengan **jendela nama terpisah** dan potret wajah.

---

## 7. Apa yang membuat BS1 terasa "penuh" (dan v1 kita terasa kosong)

1. **Kepadatan**: setiap layar peta punya sesuatu (kilau, musuh, NPC, pintu terkunci, mayat dengan item).
2. **Kehilangan yang bermakna**: soul bisa hilang → setiap langkah setelah bonfire terasa berisiko.
3. **Battle cepat**: 20–30 detik per battle biasa, jadi 40–60 battle dalam satu jam tidak melelahkan.
4. **Satu mata uang, banyak tujuan**: level, toko, pendamping, upgrade → keputusan terus-menerus.
5. **Atmosfer per area**: musik, cuaca, fog, warna battleback berbeda per wilayah.
6. **Boss sebagai tonggak**: tiap area diakhiri boss dengan mekanik (multi-bagian, telegraf, fase).
7. **Cerita lewat lingkungan dan NPC**, bukan teks pembuka panjang. Dongeng yang dikenal pemain dipelintir.
8. **Hub yang tumbuh**: NPC yang diselamatkan pindah ke hub, toko bertambah barang.

---

## 8. Implikasi untuk v2 (draf, bahan brainstorming)

| Sistem BS1 | Usulan adaptasi v2 | Struktur data kandidat |
|---|---|---|
| ATB wait/CTB + chant + stun | **Antrean giliran terlihat** (timeline) yang bisa dimanipulasi: skill cepat, rapal lambat, stun menunda | DLL (sisip/hapus/geser node berdasar waktu) atau Priority Queue manual |
| Dunia hub + wilayah + rantai peta | Graph area (node = peta, edge = pintu/jalan pintas, terkunci/terbuka); warp antar bonfire = BFS/jalur | **Graph** (adjacency list manual) |
| Bonfire, respawn, soul jatuh | Daftar bonfire yang sudah ditemukan, titik respawn terakhir, penanda soul | Linked list bonfire + Stack/riwayat |
| Event & dialog | Antrean perintah event/cutscene (teks, pilihan, battle, transfer) | **Queue** |
| Menu bertingkat | Navigasi menu (buka/tutup submenu) | **Stack** |
| Inventaris | Hindari mirip contoh "Gear Inventory SLL" yang dilarang | — |

Catatan kepatuhan tugas: mekanik yang direplikasi bisa dirumuskan sebagai **(M1) giliran berbasis kecepatan
dengan rapal/stun**, **(M2) dunia saling terhubung dengan bonfire & warp**, **(M3) loop mati–ambil soul**.
Semua itu benar-benar memakai struktur datanya saat game berjalan, bukan pajangan.
