# Rencana v2 — TUMBAL

> Status: **disetujui 2026-11-19; fase 1–14 selesai; tersisa playtest penuh oleh pemilik proyek, keputusan kesulitan Buto, & tabel tim README** (demo lengkap 8 peta sampai tamat sudah dapat dimainkan; lihat docs/DEMO.md dan bagian 8). Disusun 2026-11-19 dari brainstorming dan
> [bedah sistem Black Souls 1](research/black-souls-1-analysis.md).
> Judul game: **TUMBAL** (disepakati 2026-11-19).

---

## 1. Keputusan yang sudah disepakati

| Topik | Keputusan |
|---|---|
| Acuan | Black Souls 1 (RPG Maker VX Ace): JRPG sederhana + loop Souls. Hanya **mekanik** yang ditiru; dunia, cerita, nama, art orisinal. |
| Tema | Dongeng Nusantara versi kelam. Wilayah pertama: **Timun Mas & Buto Ijo**. Bukan gereja/agama. |
| Battle | Front-view. **ATB mode wait persis BS1** (gauge AP terisi sesuai kecepatan, berhenti total saat memilih perintah) + **panel "giliran berikutnya"** berbasis Doubly Linked List. |
| Perintah battle | Serang / Jurus / Bertahan / Barang / Kabur. Semua kelas punya **Elak** dan **Tangkis** sejak awal (versi Evade/Parry BS1). |
| Eksplorasi | Dunia top-down berbasis tile, jalan 8 arah, musuh **terlihat** di peta (symbol encounter), bukan peta node. |
| Party | Mulai **sendirian**, lalu **memanggil pendamping** (seperti BS1). |
| Mata uang | **Sukma** = mata uang + EXP (seperti soul BS1). |
| Checkpoint | **Tungku**: pulih penuh, isi ulang ramuan, musuh hidup lagi, titik bangkit, tujuan warp. |
| Mati | Sukma jatuh di lokasi kematian, bisa diambil **sekali**; langsung bangkit di Tungku terakhir (seperti BS1). |
| Save | **Save di mana saja** lewat menu, beberapa slot, mati tidak pernah menyimpan otomatis → trik "load save" BS1 tetap ada. |
| Tokoh utama | Tanpa nama, dinamai pemain (seperti BS1). |
| Aset | CC0/gratis dari internet (lisensi dicek); Codex hanya untuk celah yang tidak ada aset gratisnya. |
| Target | Demo ±45–60 menit, 1–2 minggu. |

---

## 2. Konsep cerita (draf)

Desa **Karangtumbal** subur karena perjanjian lama dengan **Buto Ijo**: setiap musim, satu anak "ditanam"
dan dipanen untuk sang raksasa. Dalam dongeng, Timun Mas lolos dengan empat bekal ajaib (biji timun,
jarum, garam, terasi) yang mengubah tanah menjadi ladang, hutan bambu, laut, dan rawa. Versi kita:
**sihir itu tidak pernah hilang dan membusuk**, dan Buto tidak pernah benar-benar mati.

Pemain adalah **Tumbal** tanpa nama yang seharusnya sudah dimakan, tetapi bangun lagi di lumbung Buto.

- **Mbok Srini**: penjaga Tungku di desa; ibu angkat Timun Mas; berpihak ke mana tidak jelas.
- **Timun Mas**: selamat, tetapi tidak utuh lagi; pendamping yang bisa dipanggil.
- **Buto Ijo**: boss akhir wilayah, multi-bagian (kepala + dua lengan) seperti boss BS1.
- Cerita disampaikan lewat lingkungan, NPC, dan deskripsi barang; tanpa teks pembuka panjang.

Prinsip: pelintir dengan hormat, tidak mengejek budaya; tidak memakai figur yang disakralkan.

---

## 3. Alur demo (±45–60 menit)

```
Judul → Nama → Pilih kelas (Pendekar / Maling / Pawang) → Hadiah awal (1 dari 4)
  │
  ▼
[1] Lumbung Buto (tutorial, ±5 mnt)    loot, 1 musuh terlihat, pintu sel, cutscene
  ▼
[2] Desa Karangtumbal (HUB)  ●Tungku    Mbok Srini, pandai besi, pedagang, NPC
  ├──▶ [3] Ladang Timun Busuk           mini-boss: Memedi Sawah
  │      ▼
  │   [4] Hutan Bambu Jarum  ●Tungku    jalan pintas balik ke desa (edge terkunci → terbuka)
  │      ▼
  │   [5] Laut Garam                    Timun Mas ditemukan → bisa dipanggil
  │      ▼
  └──▶ [6] Rawa Terasi  ●Tungku  →  [7] Sarang Buto: BOSS Buto Ijo
                                       ▼
                                    Ending demo + teaser wilayah berikutnya
```
Target kepadatan per peta: 10–20 kilau loot, 5–10 musuh terlihat, 1–3 NPC/rahasia, 1 jalan pintas.

**Realisasi (dipakai di game sekarang):** 8 peta — Sel Tumbal → Lorong Lumbung (●Tungku, boss kecil Penjaga
Lumbung) → Desa Karangtumbal (●Tungku, hub: Mbok Srini, penumbuk lesung, Empu Karta, pedagang) → Ladang Timun
Busuk (●Tungku, Memedi Sawah membuka jalan ke timur) → Hutan Bambu Jarum (●Tungku, tebas bambu = jalan pintas ke
desa) → Laut Garam (Timun Mas di dermaga bergabung) → Rawa Terasi (●Tungku, pemuda desa) → Sarang Buto (satu arah,
boss Buto Ijo) → layar tamat. Rincian rute: [DEMO.md](DEMO.md).

---

## 4. Mekanik yang dinilai & struktur data

Aturan tugas: ≥1 linear + ≥1 non-linear, ADT manual (node + pointer), ≥2 subprogram per mekanik
di luar operasi primitif, semua di `src/core/` tanpa raylib.

### M1 — Battle ATB "wait" + antrean giliran *(BS1: skrip ATB C Winter)*
- **Struktur data:** `Timeline<Battler>` = **Doubly Linked List** terurut menurut "sisa waktu sampai giliran"
  (dipakai ulang dari v1). Head = yang bertindak berikutnya.
- **Cara kerja:** AP 0→4000, laju `AGI + 10` per tick. Sisa waktu = `(4000 − AP) / (AGI + 10)`.
  Menjalankan waktu = `subtractAll(Δ)`; gauge AP di GUI dihitung balik dari sisa waktu → visual persis ATB.
- **Operasi:** `insertSorted` (masuk battle, summon), `remove` (mati/kabur), `updateAV` +
  `reposition` (stun menunda, haste mempercepat, rapal memasang waktu tunggu), `popFront` (giliran),
  salinan list untuk prediksi.
- **Subprogram M1:** `advanceTime()` / `advanceToNextActor()`, `startChant()`, `applyDelay()` (stun/sapuan),
  `predictTurns(n)` (isi panel antrean), `resolveAction()`, `threatOn()` (telegraf jurus besar).
- **Pendukung linear:** `Queue<BattleEvent>` untuk urutan animasi/log (sudah ada di v1).

### M2 — Dunia saling terhubung & Tungku *(BS1: hub + rantai peta + bonfire/warp)*
- **Struktur data:** **Graph** adjacency multi linked list (dipakai ulang dari v1).
  Vertex = peta; edge = pintu/tepi peta, dengan status **terbuka/terkunci/satu arah** (jalan pintas).
- **Operasi:** `addVertex/addEdge` (muat dunia), `findEdge` (pindah peta), update status edge (buka jalan
  pintas), BFS (atlas/peta dunia, cari Tungku terdekat), traversal penuh (respawn musuh semua peta).
- **Subprogram M2:** `travel(exit)`, `unlockShortcut()`, `restAtTungku()` (traversal: pulihkan + hidupkan
  musuh di setiap vertex), `nearestTungku()` (BFS), `warpTargets()` (BFS), `warpTo(tungku)`.

### M3 — Interpreter event & dialog *(BS1: daftar perintah event RPG Maker)*
- **Struktur data:** **Stack** frame eksekusi (event memanggil event lain / cabang pilihan, seperti
  interpreter RPG Maker) + **Queue** pesan dan pop-up "barang didapat".
- **Perintah event:** teks, pilihan, beri/ambil barang, sukma, battle, pindah peta, set flag, if flag/barang,
  panggil event, pulihkan, buka jalan, notifikasi, gabung pendamping (`join`), buka toko/tempa (`shop`).
- **Subprogram M3 (realisasi):** `runEvent()`, `update()` (jalankan sampai menunggu), `choose()` (masuk cabang =
  push), `execute()` (satu perintah).

> Inventaris sengaja **bukan** linked list "gear" (mirip contoh terlarang di PDF): cukup array jumlah per
> ID barang. Pohon skill juga tidak dipakai (contoh terlarang kedua).

---

## 5. Sistem game (non-ADT, tetap di core)

| Sistem | Rencana | Realisasi |
|---|---|---|
| Stat | HP, MP, ATK, DEF, MAG, RES, AGI, LCK; rumus BS1 | Sesuai rencana (fisik `atk*4 − def*2`, ilmu `dasar + mag*5 − res*2`, variance 20%) |
| Kelas | Pendekar, Maling, Pawang | Sesuai rencana (+ Timun Mas sebagai kelas pendamping) |
| Jurus | ±12 jurus pemain + Elak, Tangkis; rapal; elemen | 7 jurus pemain + Serang/Bertahan/Elak/Tangkis, 2 jurus Timun Mas, 24 jurus musuh; rapal & tunda |
| Status | 10 status | Racun, Berdarah, Terbakar, Lumpuh, Tidur, Bisu, Takut, Rapuh, Cepat, Lambat (+ Bertahan/Elak/Tangkis) |
| Barang | Jamu, penawar, bom, 4 bekal sebagai barang battle | Jamu, penawar, bom, sukma, Abu Tungku, Bijih Besi, Kendi Jamu; **4 bekal jadi barang cerita** (bukan barang battle) |
| Senjata | 4 tipe × 3 tingkat | **1 senjata per kelas**, ditempa +0..+3 (sukma + Bijih Besi) di Empu Karta |
| Level | Tukar sukma di Tungku | Sesuai rencana |
| Pendamping | Timun Mas, dipanggil di Tungku | Sesuai rencana (naik level dengan sukma, panggil/lepas) |
| Musuh | ±10 jenis + 2 mini-boss + Buto | 13 musuh biasa + Penjaga Lumbung + Memedi Sawah + Buto Ijo (kepala + 2 tangan, telegraf) |
| AI musuh | Pola aksi + kondisi; pendukung menyembuhkan | Pola aksi berbobot + syarat HP; **belum ada musuh penyembuh** |
| Save | Beberapa slot, simpan di mana saja | 3 slot, mati tidak menyimpan; tambahan: menu cheat, bahasa EN/ID |

---

## 6. Arsitektur

Struktur yang dipakai sekarang:

```
src/core/            (tanpa raylib & tanpa container STL — bisa dites & jalan di console)
  adt/               Timeline (DLL), Queue, Stack, Graph
  data/              GameData: kelas, jurus, status, barang, musuh, troop, senjata, dagangan
  battle/            Battler, BattleSystem (ATB wait, AI, rumus, telegraf, cheat), BattleEvent
  world/             World (Graph peta + Tungku), TileMap (Tiled), Field (gerak, musuh terlihat)
  event/             Script (perintah JSON), Interpreter (Stack frame + Queue notifikasi)
  game/              Game (perangkai), GameState (party, inventaris, sukma, flag, simpan/muat)
  util/              Json, Lang (kamus EN/ID), Rng, DsTrace
src/gui/             (raylib) Title, Map, Battle, Dialog, Menu, Tungku, Shop, Settings, Audio, Ui
src/console/         --console (versi teks) dan --balance (simulasi kesulitan)
assets/              maps/, events/, lang/, tiles/, props/, chars/, battlers/, faces/, audio/, fonts/
tools/               generator aset, peta, pratinjau, musik, terjemahan
tests/               unit test core (framework CHECK)
```
- **Peta dibuat di Tiled** (gratis). Objek di layer khusus: `tungku`, `loot`, `enemy(troop)`, `door(to,edge)`,
  `npc(event)`, `lever(shortcut)`. Anggota tim non-programmer bisa ikut membuat peta.
- Yang dipakai ulang dari v1: ADT + testnya, Makefile, framework test, Rng, pola `dsOp` untuk Inspector.
  Sisanya ditulis ulang.

---

## 7. Aset

Prioritas: satu paket top-down yang konsisten, lalu battler musuh front-view, battleback, UI, audio.
Hasil riset lisensi: lihat bagian 7.1 (diisi setelah riset selesai). Celah yang tidak ada aset gratisnya
(kemungkinan battler Buto Ijo & Memedi Sawah) dibuat dengan Codex atau digambar ulang.

### 7.1 Paket inti (lisensi dibaca di halaman aslinya, 2026-11-19)

Semua CC0 atau CC-BY → boleh disimpan di repo publik **asal ada berkas `CREDITS.md`** yang menyebut
setiap pembuat. Tile 32×32 gaya RPG Maker XP, cocok dengan nuansa BS1.

| Kebutuhan | Pilihan | Lisensi | Catatan |
|---|---|---|---|
| Tileset | **Sevarihk / Aurora-Sprites** (OpenGameArt): Dead Swamp, Evil Dungeon, Bamboo Tiles, Batak House, Ground & Foliage, Animated Water, Fisher's Hut | CC-BY 4.0 | Satu gaya, gelap, nuansa Asia Tenggara. Rawa, lumbung, hutan bambu, pantai, rumah adat. |
| Karakter jalan | **Aurora Character Generator** (redoxgames.itch.io) | CC-BY 4.0 | 4 arah, cocok dengan tileset Sevarihk. Kita pakai gerak 8 arah dengan sprite 4 arah (seperti BS1). |
| Potret wajah | Sevarihk NPC Faceset + Cardboard Style | CC-BY 4.0 | Gaya sketsa; tokoh utama dan Timun Mas perlu dibuat. |
| Battler musuh | **Redshrike** (10 basic + 6 more rpg enemies, fossil undead, dll.), **NettySvit** Plant & Mushroom Enemies | CC-BY 3.0 / CC0 | Gaya FF6, front-view. |
| Battleback | **Nidhoggn "Backgrounds"** (10 lukisan 1104×621), Hyptosis Simple Battles | CC0 / CC-BY 3.0 | Hutan dan ruang bawah tanah sudah dicek. |
| UI | Kenney Fantasy UI Borders, Flare Windows (dark fantasy), ikon 7Soul1 (496 ikon) | CC0 | — |
| Font | m5x7 atau Pixel Operator | CC0 | Alternatif OFL: Pixelify Sans. |
| Musik | cynicmusic Dark Forest, qubodup Dark Shrine Loop, SubspaceAudio Horror Atmosphere, Pro Sensory Boss Battle Loop, tema rawa/desa Ninja Adventure | CC0 | — |
| SFX | Kenney RPG Audio, artisticdude RPG Sound Pack (ada raungan raksasa), SubspaceAudio 512 SFX | CC0 | — |
| Gamelan | ~~Sampel CC0 Freesound~~ → disintesis sendiri (`tools/make_gamelan.py`) | orisinal | Musik Desa: gending pelog muram + lesung. |

**Dihindari:** paket share-alike (LPC, Nighthawking, dll.), paket "dilarang didistribusikan ulang"
(Pipoya, Mystic Woods, Aekashics Librarium untuk file satuan), dan aset bawaan RPG Maker.

### 7.2 Celah → dibuat sendiri / Codex
1. Battler **Buto Ijo** (boss 3 bagian) dan **Memedi Sawah** (mini-boss orang-orangan sawah).
2. Potret & sprite **tokoh utama, Timun Mas, Mbok Srini**, warga berpakaian Jawa. *(Potret dialog untuk 9 pembicara
   dibuat dari sprite peta masing-masing (diperbesar 4x) + battler Codex; tokoh utama tanpa potret, seperti BS1.)*
3. Tile **ladang timun busuk** dan **rumah desa Jawa**. *(Selesai: rumah limasan & rumah tua runtuh dibuat Codex,
   dipasang di Desa.)*
4. Battleback **pantai garam** dan **rawa** (bila Nidhoggn tidak cocok).
5. Semua dibuat mengikuti gaya Sevarihk/Redshrike agar tidak belang.

Yang belum terverifikasi: isi lengkap 8 battleback Nidhoggn lain, ukuran piksel sprite Redshrike/Aurora.
Diperiksa saat aset diunduh pada hari 1.

---

## 8. Fase pengerjaan

Semula ditulis sebagai jadwal 14 hari, tetapi pengerjaan berjalan per fase (fase 1–11 selesai dalam satu
hari kerja, 2026-11-19). Fase 14 dikerjakan 2026-11-20 dari catatan playtest pertama. Status dicocokkan dengan
kode terbaru (377 unit test lulus).

| Fase | Target | Hasil yang bisa dicoba | Status |
|---|---|---|---|
| 1 | Bersihkan kode v1 dari `main` (backup sudah ada), kerangka baru, impor aset, loader Tiled | Jendela + peta tampil | Selesai |
| 2 | Jalan 8 arah, kamera, collision, pindah peta lewat Graph | Bisa keliling 2 peta | Selesai |
| 3 | Core battle ATB wait + DLL + rumus + status dasar (+ test) | Battle di console | Selesai |
| 4 | Battle GUI front-view: gauge, panel antrean, menu perintah, damage pop-up | Battle di GUI | Selesai |
| 5 | Musuh terlihat di peta, transisi ke battle, Tungku, sukma, mati/ambil sukma | Loop Souls jalan | Selesai |
| 6 | Interpreter event (dialog, pilihan, loot), menu (Barang/Jurus/Status/Save), save/load | Tutorial bisa dimainkan | Selesai |
| 7 | **Vertical slice**: tutorial + desa + ladang + mini-boss. **Playtest pemilik proyek** | ±15 menit main | Konten selesai; **playtest belum** |
| 8 | Peta 4–5, pendamping Timun Mas, pandai besi, toko | ±35 menit | Selesai (Empu Karta menempa +0..+3 dengan sukma + Bijih Besi; pedagang dengan stok terbatas & Kendi Jamu) |
| 9 | Rawa + boss Buto Ijo (3 bagian, telegraf) | Demo tamat | Selesai (telegraf: jurus besar dirapal, namanya tampil di atas musuh dan baris party yang diincar berkedip) |
| 10 | Audio, cahaya/kabut, getar layar, transisi, judul & ending | Terasa "jadi" | Selesai (kabut: property peta `kabut`, dua lapis noise bergeser di Ladang, Bambu, Laut, Rawa, Sarang) |
| 11 | Polish visual tiap wilayah, kejelasan battle, teks cerita reaktif | — | Selesai |
| 12 | Balancing, playtest penuh, perbaikan | — | `--balance` selesai (Buto ternyata ±1–3 level lebih berat dari kurva untuk bot); **playtest penuh belum** |
| 13 | README, dokumentasi struktur data, skrip demo, bahan Pendampingan | — | `docs/DEMO.md` (rute & titik demo) dan `docs/STRUKTUR_DATA.md` (ADT, kompleksitas, subprogram, tanya-jawab Pendampingan) selesai; **tabel tim README diisi tim** |
| 14 | Perbaikan dari playtest & polish lanjutan | — | Selesai: gerak mulus & garis hitam (skala bulat, kamera piksel), pakaian karakter tertutup, menu Keluar & Cheat, Mulai Baru mereset dunia, pilih kelas kiri/kanan, potret dialog dari sprite, musik gamelan, rumah Jawa, **bahasa English (bawaan) + Indonesia**, font kecil terbaca, README publik |


---

## 9. Risiko

| Risiko | Mitigasi |
|---|---|
| Battler musuh front-view gratis yang gelap & bergaya sama sulit dicari | Pilih paket paling konsisten; celah via Codex dengan prompt satu gaya |
| Membuat peta padat memakan waktu | Tiled + 6–7 peta saja; tim planning ikut menyusun peta |
| Pendampingan 1 belum terjadi; mekanik bisa mirip kelompok lain | Siapkan penjelasan perbedaan; ATB+DLL+Graph dunia+interpreter event cukup khas |
| Scope melebar | Vertical slice fase 7 jadi patokan; fitur di luar tabel ini ditunda |

---

## 10. Yang masih perlu diputuskan
1. ~~Judul game~~ → **TUMBAL** (sudah dipilih).
2. **Repo**: tetap `DaffMe/Cinderwake` lalu ganti nama setelah judul dipilih, atau repo baru.
3. **Pembagian tim**: usul — kamu: kode; teman planning: peta Tiled + teks dialog/deskripsi barang;
   teman laporan: laporan + catat keputusan desain sejak sekarang.
