# Skrip Demo Cinderwake (± 12 menit)

Urutan demo yang menunjukkan setiap struktur data **benar-benar bekerja di dalam mekanik**, sesuai poin penilaian
"hubungan aksi pengguna, logika C++, dan perubahan struktur data harus dapat diamati".

**Persiapan sebelum demo**
- `mingw32-make` dan `mingw32-make test` sudah dijalankan; folder `bin/` siap.
- Buka dua jendela: terminal (untuk console & test) dan game GUI.
- Tombol bantuan demo: **`F3`** di peta = ungkap seluruh peta, **`F2`** di battle = isi Resolve semua hero.
- Siapkan [ARCHITECTURE.md](ARCHITECTURE.md) untuk menjawab "fungsinya di file mana?".

---

## 1. Pembuka (1 menit)

- Game: dungeon crawler turn-based dark fantasy, **dunia & karakter orisinal**.
- Mekanik yang direplikasi:
  - **M1 Action Timeline** dari Honkai: Star Rail / Final Fantasy X → **Doubly Linked List**
  - **M2 Dungeon Exploration dengan cahaya** dari Darkest Dungeon → **Graph**
  - **M3 Rewind giliran** dari Into the Breach → **Stack**
  - Pendukung: **Battle Event Queue** → **Queue**
- Semua ADT dibuat manual di `src/core/adt/`, tanpa container STL. GUI hanya tampilan.

## 2. Bukti logika di C++, bukan di GUI (1,5 menit)

```bash
bin/cinderwake.exe --console
```

- Pilih `1` (Hollow Thralls). Tunjukkan:
  - baris **`Timeline (DLL): HEAD ⇄ ... ⇄ TAIL`** beserta nilai AV
  - baris **`[DS] Timeline.subtractAll / popFront / insertSorted`** setelah setiap aksi
- Kalimat kunci: *"Ini program yang sama dengan GUI, hanya tanpa raylib. Semua mekanik ada di `src/core`."*
- Keluar (`q`).

## 3. M2 — Peta Graph (3 menit)

Jalankan `mingw32-make run` → **Ekspedisi Baru**.

1. Tunjuk peta: ruangan = **vertex**, koridor = **edge berbobot** (titik kecil = bobot = biaya cahaya).
2. Tekan **`Tab`** → Inspector: **adjacency list** (`[1] E : ->4(w2) ->5(w2)`).
3. Klik ruang tetangga → cahaya turun sebesar bobot × 4. Log: **`Graph.bfs(start=.., radius=2)`**. Jelaskan
   bahwa radius BFS bergantung level cahaya (terang 2, redup 1, gelap 0), dan itulah fog of war.
4. Arahkan kursor ke ruang jauh yang sudah terungkap → jalur tersorot. Log: **`Graph.dijkstra(a -> b)`**. Jelaskan
   bahwa Dijkstra hanya lewat ruang terungkap, lalu jalurnya direkonstruksi lewat **Stack**.
5. Tekan **`S`** → auto-travel ke Ember Shrine terdekat (Dijkstra ke setiap shrine).
6. (Opsional) Ruang **Peristiwa → "Langit-langit Berderak"** → koridor runtuh: log
   **`Graph.removeEdge` lalu `Graph.isConnected()`** (koridor dipulihkan bila memutus lantai).

## 4. M1 — Timeline Doubly Linked List (3 menit)

Masuk ruang pertarungan. Inspector tetap terbuka.

1. Kolom **GILIRAN** di kiri = isi node DLL dari HEAD ke TAIL. Inspector menampilkan **pointer prev/next** dan AV.
2. Giliran Sable → **Unravel** ke musuh terdepan: node musuh **pindah ke belakang**. Log:
   **`Timeline.updateAV(X, 20.0 -> 60.0) - node direlink`**. Jelaskan: lepas node (unlink), lalu sambungkan ulang
   lewat `next` (delay) atau `prev` (advance).
3. Ione → **Kindle** ke sekutu: node sekutu **maju ke depan** (advance memakai pointer `prev`).
4. Tekan **`F2`**, lalu klik **SELA** pada hero lain: **`Timeline.remove` + `Timeline.insertFront`** (Interject).
5. Kalahkan musuh: **`Timeline.remove(X) - unit tumbang`**.
6. Tunjuk **`predictTurnOrder(8)`** di Inspector: simulasi pada **salinan** DLL (deep copy), DLL asli tidak berubah.

## 5. M3 — Rewind dengan Stack (1 menit)

1. Di Inspector bagian **REWIND · Stack**, kotak `g1 g2 ...`: satu snapshot di-push setiap giliran hero.
2. Lakukan aksi, lalu tekan **Rewind** / **`R`**: HP, Ember, dan Timeline kembali. Log **`Stack.pop` + `Stack.peek`**.

## 6. Queue — Event battle (0,5 menit)

- Jelaskan: logika battle meng-**enqueue** event (damage, delay, death); GUI men-**dequeue** satu per satu menjadi
  animasi (FIFO). Tahan **Space** untuk mempercepat; urutan tetap sama.

## 7. Siklus souls-like & penutup (1 menit)

- Saat party tumbang: bangkit di **Ember Shrine** (checkpoint), **Ash tertinggal** di ruang kematian (penanda merah).
- Tunjukkan hubungan M2 → M1: cahaya **Gelap** → peluang **penyergapan**, musuh dimajukan 30% di Timeline.

## 8. Pengujian (1 menit)

```bash
mingw32-make test          # 182 pengecekan otomatis
bin/cinderwake.exe --balance 200   # simulasi ratusan battle & run penuh
```

---

## Persiapan tanya-jawab

| Pertanyaan yang mungkin | Jawaban singkat | Rujukan |
|---|---|---|
| Kenapa DLL, bukan array/queue biasa? | Urutan berubah di tengah (delay/advance, mati, summon); DLL cukup relink pointer O(1), `prev` untuk geser maju | `Timeline.hpp: reposition()` |
| Kenapa Graph, bukan Tree? | Ruangan punya banyak tetangga dan **siklus**; koridor berbobot untuk Dijkstra | `Floor.cpp: generateFloor()` |
| Representasi Graph? | Adjacency **multi linked list**: list vertex, tiap vertex punya list edge yang menunjuk pointer vertex tujuan | `Graph.hpp` |
| Dijkstra tanpa priority queue? | Pemilihan minimum linear O(V²), cukup untuk ±20 ruangan | `Graph.hpp: dijkstra()` |
| Delete di mana? | `Timeline::remove` (unit mati), `Graph::removeEdge` (koridor runtuh), `Stack::pop` (rewind), `Queue::dequeue` | ARCHITECTURE.md §3 |
| Mana subprogram selain operasi primitif? | `advanceTimeline`, `shiftActionValue`, `predictTurnOrder`, `generateFloor`, `revealByLight`, `findNearestShrine`, `saveTurnSnapshot`, `rewindTurn`, dll. | ARCHITECTURE.md §4 |
| Apakah GUI menjalankan logika? | Tidak; GUI memanggil API `Run`/`Battle` dan memutar Event Queue. Bukti: `--console` | `src/gui/BattleView.cpp` |
| Ada library yang menggantikan ADT? | Tidak; core hanya memakai `std::string`, `<cmath>`, `std::unique_ptr`. raylib hanya di `src/gui` | CLAUDE.md |
| Data apa yang disimpan? | Unit (HP, ATK, DEF, SPD, status), node Timeline (unit + AV), Room (jenis, terungkap, selesai), edge (bobot) | `Unit.hpp`, `Floor.hpp` |
