# CINDERWAKE — Rencana Proyek (draft v1)

> Tugas Besar Struktur Data · Semester Ganjil 2026/2027 · Kelompok 3 orang
> Working title, boleh diganti. Dokumen ini adalah bahan perencanaan; laporan final tetap ditulis sendiri oleh tim (lihat §10).

---

## 1. Ringkasan

**Cinderwake** adalah dungeon crawler turn-based bertema dark fantasy. Party berisi 3 pahlawan menuruni katedral terkutuk sambil membawa lentera yang cahayanya terus menipis. Pemain menjelajahi peta ruangan yang saling terhubung, bertarung dengan sistem **timeline giliran**, dan kembali ke **Ember Shrine** (checkpoint) setiap kali party tumbang.

- **Genre:** turn-based RPG, pertarungan *front-view* (gaya Black Souls / RPG Maker), visual 2D lukisan gothic.
- **Teknologi:** C++17 + raylib. Satu `.exe`. Tidak memakai game engine.
- **Prinsip:** seluruh ADT dan logika mekanik ada di `src/core/` (C++ murni, tanpa raylib). GUI di `src/gui/` hanya membaca state dan mengirim aksi.

Dunia, karakter, nama, dan art dibuat **orisinal**. Yang direplikasi dari game sumber hanya **mekaniknya**.

---

## 2. Mekanik yang direplikasi

### M1 — Action Timeline (urutan giliran)
**Game sumber:** *Honkai: Star Rail* (sistem Action Value), *Final Fantasy X* (Conditional Turn-Based).

**Cara kerja di game sumber:** setiap unit memiliki jarak aksi `AV = 10000 / SPD`. Unit dengan AV terkecil bertindak lebih dulu. Setelah bertindak, AV unit itu di-reset dan unit masuk kembali ke antrean. Skill tertentu bisa memajukan (*advance*) atau menunda (*delay*) giliran, sehingga urutan berubah secara dinamis. Urutan giliran berikutnya ditampilkan di layar.

**Yang dipertahankan:** rumus AV, advance/delay persentase, extra turn, preview urutan giliran, unit mati keluar dari timeline, dan summon masuk ke timeline.
**Yang disederhanakan:** tanpa sistem energi/ultimate yang kompleks. Diganti dengan *Resolve* (gauge) untuk **Interject**, yaitu menyisip ke posisi paling depan.

**Struktur data:** **Doubly Linked List terurut** (`Timeline`), node = `TurnNode { Unit* unit; float av; TurnNode* prev; TurnNode* next; }`.

**Justifikasi:**
- Urutan selalu berubah di tengah list (delay/advance memindahkan node ke posisi baru). Pada linked list ini cukup *relink* pointer dengan O(1) setelah posisi ditemukan, tanpa menggeser elemen seperti array.
- `prev` dibutuhkan untuk memindahkan node **mundur** (advance) tanpa traversal ulang dari head, dan untuk animasi geser kiri/kanan di GUI.
- Queue biasa tidak cukup karena elemen tengah perlu dihapus (unit mati) dan disisipkan secara terurut, bukan hanya di ujung.

| Operasi ADT | Dipakai saat |
|---|---|
| `insertSorted(unit, av)` | awal battle, summon, unit selesai bertindak |
| `insertFront(unit)` | Interject / extra turn |
| `remove(unit)` | unit mati |
| `search(unit)` | target skill delay/advance |
| `moveNode(node)` (relink prev/next) | setelah AV berubah |
| `traverse()` | render timeline, kurangi AV semua unit |

**Subprogram mekanik (≥2):**
1. `advanceTimeline()`: ambil head, kurangi AV semua node sebesar AV head, lalu kembalikan unit yang bertindak.
2. `shiftActionValue(unit, persen)`: delay/advance lalu reposisi node.
3. `predictTurnOrder(n)`: simulasi n giliran ke depan tanpa mengubah list asli, untuk preview.
4. `handleUnitDeath(unit)`: hapus node, cek kondisi menang/kalah.

### M2 — Dungeon Exploration dengan Cahaya
**Game sumber:** *Darkest Dungeon* (peta ruangan + koridor, mekanik torch/light level).

**Cara kerja di game sumber:** dungeon terdiri dari ruangan yang dihubungkan koridor. Pemain berpindah antarruangan, dan perjalanan menghabiskan cahaya obor. Cahaya rendah membuat musuh lebih kuat dan meningkatkan risiko disergap, tapi hadiahnya lebih besar. Peta terungkap bertahap.

**Yang dipertahankan:** ruangan dan koridor, cahaya habis saat bergerak, efek cahaya terhadap pertarungan, peta terungkap bertahap, kembali ke ruangan yang sudah dikunjungi.
**Yang disederhanakan:** koridor tidak berisi petak/curio. Bobot koridor langsung menjadi biaya cahaya.

**Struktur data:** **Graph tak berarah berbobot** dengan representasi **adjacency multi-linked list** (list vertex, dan setiap vertex punya list edge sendiri).

**Justifikasi:**
- Ruangan memiliki banyak tetangga dan **siklus** (lebih dari satu jalan ke ruangan yang sama), sehingga tidak bisa dimodelkan dengan Tree.
- Koridor punya bobot (biaya cahaya), sehingga pencarian jalur termurah (Dijkstra) bermakna.
- Adjacency list hemat memori untuk graph jarang (~16–20 vertex, ~25 edge) dan memudahkan hapus edge (koridor runtuh).

| Operasi ADT | Dipakai saat |
|---|---|
| `addVertex(room)` / `addEdge(a, b, w)` | generate floor |
| `removeEdge(a, b)` | event "koridor runtuh" |
| `findVertex(id)` | klik ruangan |
| `neighbors(v)` (traversal edge list) | gerak, render peta |
| `bfs(start, depth)` | reveal fog of war sesuai cahaya |
| `dijkstra(start, target)` | auto-travel / jalur termurah ke shrine |

**Subprogram mekanik (≥2):**
1. `generateFloor(seed, floorNo)`: tempatkan ruangan, buat spanning tree supaya pasti terhubung, lalu tambah edge ekstra untuk siklus.
2. `revealByLight()`: radius BFS ditentukan level cahaya (terang = 2, redup = 1, gelap = 0).
3. `travelTo(room)`: kurangi cahaya sebesar bobot, lalu picu isi ruangan (battle, shrine, event, toko).
4. `findPathToShrine()`: Dijkstra ke shrine terdekat yang sudah terungkap.

**Hubungan M2 → M1:** saat cahaya gelap, musuh dapat *ambush*, yaitu AV musuh dimajukan 30% di awal battle (memanggil `shiftActionValue`). Peta dan pertarungan saling memengaruhi, bukan dua fitur terpisah.

### M3 — Cracked Hourglass (rewind giliran) · struktur pendukung
**Game sumber:** *Into the Breach* (Reset Turn).
**Struktur data:** **Stack** berisi snapshot state battle (HP, buff, salinan timeline). Push di awal setiap giliran pemain. Pop saat item Hourglass dipakai (terbatas 3 per run).
**Subprogram:** `saveTurnSnapshot()`, `rewindTurn()`.

### Struktur pendukung — Battle Event Queue
**Queue** (linked) berisi event hasil logika C++ (`DAMAGE`, `DELAY`, `DEATH`, `SUMMON`, …). Core mengisi queue, GUI mengambilnya satu per satu untuk dianimasikan. Queue yang sama dipakai BFS di M2.
Peran Queue ini membuat **hubungan aksi pemain → logika C++ → perubahan struktur data → tampilan** bisa diamati secara langsung (syarat klarifikasi GUI).

### Ringkasan ADT (semua manual, tanpa `std::list/queue/stack/vector/map/priority_queue`)

| ADT | Kategori | Dipakai oleh |
|---|---|---|
| Doubly Linked List (Timeline) | Linear | M1 |
| Queue | Linear | Event battle, BFS (M2) |
| Stack | Linear | M3 Rewind |
| Graph (adjacency multi-linked list) | Non-linear | M2 |

Library yang tetap dipakai karena tidak menggantikan ADT: `std::string`, `<cmath>`, `<random>`, raylib (rendering/audio/input).

---

## 3. Game loop

```
Title → Pilih "New Descent"
  └─ Floor 1..3 (graph baru tiap floor)
       Map: pilih ruangan → travelTo → isi ruangan:
         ⚔ Battle / ☠ Elite  → Battle Scene (M1 + M3) → loot Ash
         🔥 Ember Shrine     → heal, isi cahaya, set checkpoint
         ? Event             → pilihan teks (koridor runtuh, altar, dll.)
         ⚖ Merchant          → beli oil lentera, potion, hourglass
         👑 Boss             → menang → pilih 1 dari 3 relic → floor berikutnya
  Party tumbang → kembali ke shrine terakhir, Ash tertinggal di ruangan tempat mati
                  (bisa diambil kembali jika sampai ke sana, sentuhan "souls-like")
Menang Floor 3 → ending
```

**Party (orisinal):**
| Hero | Peran | SPD | Skill khas (hubungan timeline) |
|---|---|---|---|
| Varga, the Oathbound | Tank | 95 | *Shield Bash*: damage + delay target 25% |
| Sable, the Ash Witch | Hexer | 110 | *Unravel*: delay target 40% · *Cinder Curse*: DoT |
| Ione, the Lamplighter | Support | 120 | *Kindle*: advance ally 50% · *Mend*: heal |

Sumber daya bersama: **Ember Points** (maks 5). Basic attack menambah 1, skill memakai 1–2. Interject memakai gauge *Resolve*.

**Musuh (contoh):** Hollow Thrall (dasar), Bell-Ringer (memajukan semua musuh 20%), Candle Wraith (summon → `insertSorted`), Gargoyle (lambat, tahan). **Boss:** The Choir Warden (F1), Sister of Ash (F2), The Last Flame (F3).

---

## 4. Arsitektur

```
cinderwake/
├─ src/
│  ├─ core/                 ← C++ murni, TIDAK include raylib
│  │  ├─ adt/               Timeline(DLL), Queue, Stack, Graph
│  │  ├─ battle/            Unit, Skill, Battle, StatusEffect, AI musuh sederhana
│  │  ├─ dungeon/           Room, Floor, generator, light
│  │  └─ run/               RunState (party, ash, relic, checkpoint)
│  ├─ gui/                  raylib: scene Title/Map/Battle/Shrine/Death, UI, VFX, Inspector
│  ├─ console/              mode console: menjalankan core yang sama lewat menu teks
│  └─ main.cpp
├─ tests/                   unit test ADT + mekanik (assert sederhana)
├─ assets/  raw/ (output Codex) · final/ (siap pakai) · shaders/ · audio/
├─ docs/   PLAN.md, ART_BIBLE.md, DEMO_SCRIPT.md
├─ Makefile
└─ README.md
```

- **Mode console (`cinderwake --console`)** menjalankan logika yang sama tanpa GUI. Ini bukti kuat untuk dosen bahwa GUI bukan tempat logika.
- **Inspector struktur data (tombol `Tab`)** menampilkan node Timeline dengan panah `prev/next` dan nilai AV (beranimasi saat relink), graph peta beserta adjacency list, isi Stack snapshot, isi Event Queue, dan log operasi (`insertSorted(Sable, av=72.7)`). Fitur ini yang paling menolong saat demo.
- **Build:** `mingw32-make` → `cinderwake.exe`, `cinderwake_tests.exe`. Raylib dipasang via `pacman -S mingw-w64-ucrt-x86_64-raylib`.

---

## 5. Target visual (≥ Black Souls)

- Resolusi dasar 1280×720 (render target, di-scale ke layar). Gaya **lukisan gothic gelap**: battler musuh besar di tengah, background katedral, UI ornamen besi/emas pudar.
- **Shader:** vignette, film grain, color grading per floor, *dissolve* saat musuh mati, *hit flash*.
- **Juice:** cahaya lilin berkedip, partikel bara/debu, screen shake, angka damage melayang, musuh "bernapas" (idle sine scale), token timeline meluncur saat urutan berubah (sekaligus memvisualisasikan relink DLL).
- **Peta:** ruangan berupa ikon di atas peta perkamen/batu, koridor bercahaya sesuai level lentera, dan fog of war.
- **Audio (opsional):** SFX dan ambience CC0.

---

## 6. Pembagian kerja

| Pihak | Tugas |
|---|---|
| **Codex** | Membuat dan menyusun aset visual dengan image generation mengikuti `ART_BIBLE.md`: battler musuh & boss, potret hero, background, peta, frame UI, ikon skill/ruangan/status, key art title. Output ke `assets/raw/`, disusun menjadi sheet/atlas. |
| **Claude** | Semua kode C++ (ADT, mekanik, GUI, shader, VFX). Detail aset: pembersihan alpha, crop, harmonisasi palet, tekstur partikel, animasi, integrasi. Art bible dan prompt untuk Codex. Draft README, komentar kode, dokumentasi teknis untuk tim. |
| **Kamu (programmer)** | Mengarahkan, mereview, menguji, memahami kode untuk demo, commit bertahap. |
| **Anggota perencanaan** | Analisis mekanik game sumber, justifikasi struktur data, bahan Pendampingan 1 & 2. Bisa memakai §2 sebagai titik awal. |
| **Anggota laporan** | Menulis laporan final **sendiri**. Jika AI dipakai untuk editing, cantumkan versi asli, versi hasil editing, dan perbedaannya (aturan §8.1). |

---

## 7. Fase pengerjaan

| Fase | Isi | Hasil yang bisa didemokan |
|---|---|---|
| **0 · Fondasi** | Repo, Makefile, raylib, 4 ADT + unit test | `tests.exe` lulus |
| **1 · Battle core** | Unit, skill, Timeline M1, Event Queue, AI musuh, mode console | Battle lengkap di console → **bahan Pendampingan 1** |
| **2 · Dungeon core** | Graph M2, generator, cahaya, BFS/Dijkstra, run loop, checkpoint, Stack M3 | Run penuh di console |
| **3 · GUI** | Scene Map & Battle, UI, Inspector | Bisa dimainkan dengan placeholder art → **bahan Pendampingan 2** |
| **4 · Art & juice** | Aset Codex masuk, shader, partikel, animasi, audio | Visual final |
| **5 · Polish** | Balancing, bug, README, `DEMO_SCRIPT.md` | Siap dikumpulkan dan didemokan |

Paralel: Codex mulai membuat aset sejak Fase 1 setelah art bible jadi.

---

## 8. Risiko & mitigasi

| Risiko | Mitigasi |
|---|---|
| Gaya aset image gen tidak konsisten | Art bible ketat (palet, arah cahaya, template prompt), post-process palet seragam |
| Background aset tidak transparan | Pipeline penghapusan background + pembersihan alpha manual |
| Scope membengkak | MVP = 1 floor + 1 boss dan harus playable dulu. Floor 2–3 baru dikerjakan setelahnya |
| Mekanik dianggap mirip kelompok lain | Ajukan M1+M2 ke dosen/asisten di Pendampingan 1. Belum dianggap disetujui sebelum ada konfirmasi |
| Tim tidak bisa menjelaskan kode saat demo | Kode modular, komentar Bahasa Indonesia, Inspector, `DEMO_SCRIPT.md` |

---

## 9. Pertanyaan terbuka

1. Sekarang **pekan ke berapa**, dan kapan tanggal Pendampingan 1?
2. Nama game final dan nama repo GitHub.
3. Nama & NIM anggota untuk README.

## 10. Catatan kejujuran akademik

Kode game ini dibuat dengan bantuan AI (Claude + Codex). Aturan tugas tidak melarang AI untuk kode, tetapi laporan wajib dibuat sendiri dan editing AI wajib dicantumkan. Sebaiknya penggunaan AI dalam pengembangan juga disebutkan secara terbuka di laporan/README, dan seluruh anggota memahami kode sebelum demo.
