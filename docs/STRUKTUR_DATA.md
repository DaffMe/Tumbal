# Dokumentasi Teknis Struktur Data — TUMBAL

Dokumen teknis untuk anggota tim dan bahan Pendampingan: struktur data apa yang dibuat manual, di mana dipakai
mekanik, operasi beserta kompleksitasnya, dan cara memperlihatkannya saat demo. Ini **bukan** laporan tugas besar;
laporan ditulis tim sendiri (lihat `Konteks-Tugas/`).

Semua ADT ada di [`src/core/adt/`](../src/core/adt/), ditulis dengan node + pointer sendiri, tanpa container STL.
Logika mekanik ada di `src/core/` dan tidak bergantung pada raylib; mode `--console` menjalankan logika yang sama.

## Ringkasan

| Mekanik | Struktur data utama | Jenis | Pendukung |
|---|---|---|---|
| **M1** Battle ATB mode wait + antrean giliran | `Timeline<Battler>` — Doubly Linked List terurut | Linear | `Queue<BattleEvent>` |
| **M2** Dunia saling terhubung & Tungku | `Graph<MapInfo>` — adjacency multi linked list | Non-linear | `Queue<Vertex*>` di BFS |
| **M3** Interpreter event & dialog | `Stack<Frame>` + `Queue<std::string>`; skrip = linked list `Command` bercabang (pohon) | Linear (+ pohon skrip) | — |

Syarat tugas: ≥1 linear (DLL, Stack, Queue) dan ≥1 non-linear (Graph) terpenuhi; setiap mekanik punya ≥2 subprogram
di luar operasi primitif ADT (ditandai komentar `// M1/M2/M3 subprogram` di kode).

---

## M1 — `Timeline` (Doubly Linked List) · [Timeline.hpp](../src/core/adt/Timeline.hpp)

**Ide.** Setiap node = satu battler + `av` (sisa tick sampai gilirannya). List selalu terurut menaik, jadi
**head = yang bertindak berikutnya**. Laju AP = `AGI + 10` per tick, satu giliran = 4000 AP, sehingga
`av = (4000 − AP) / laju`. Gauge AP di layar dihitung balik dari `av`.

**Kenapa DLL, bukan array.** Efek battle mengubah posisi battler di tengah antrean: rapal (chant) memasang waktu
tunggu baru, jurus tunda mendorong lawan mundur, status Cepat/Lambat mengubah laju, battler mati dihapus dari mana
saja. Di DLL cukup lepas (unlink) lalu sambung ulang pointer; pointer `prev` membuat node bisa bergeser maju tanpa
menelusuri ulang dari head.

| Operasi | Kompleksitas | Dipakai untuk |
|---|---|---|
| `insertSorted(item, av)` | O(n) | battler masuk battle (urut berdasarkan kecepatan + inisiatif) |
| `popFront` / `front()` | O(1) | giliran berikutnya |
| `remove(item)` | O(n) cari + O(1) lepas | battler tumbang / kabur |
| `updateAV(item, av)` + `reposition` | O(n) | rapal, tunda (`applyDelay`), giliran penuh baru |
| `subtractAll(delta)` | O(n) | waktu berjalan (mode wait: berhenti saat pemain memilih) |
| salin list (`Timeline(const Timeline&)`) | O(n) | `predictTurns`: simulasi giliran ke depan pada salinan |

**Subprogram M1** ([BattleSystem.cpp](../src/core/battle/BattleSystem.cpp)): `advanceTime` / `advanceToNextActor`
(menjalankan waktu ATB), `startChant` (rapal), `applyDelay` (tunda/buyarkan rapal), `resolveAction` (jalankan jurus),
`predictTurns` (isi strip "Giliran"), `threatOn` (telegraf: jurus besar yang sedang dirapal mengincar siapa).

**Pendukung linear:** `Queue<BattleEvent>` — logika battle meng-enqueue kejadian (damage, rapal, buyar, menang),
GUI men-dequeue satu per satu untuk animasi; console mencetaknya sebagai teks. Logika tidak menunggu animasi.

**Di demo:** strip "Giliran" di atas layar battle = hasil `predictTurns`. Pakai jurus tunda/Serangan Guncang pada
musuh yang merapal: namanya mundur di strip dan muncul "BUYAR!". F1 menampilkan isi DLL (`HEAD <-> ...`) dengan
`av` tiap node.

---

## M2 — `Graph` (adjacency multi linked list) · [Graph.hpp](../src/core/adt/Graph.hpp)

**Ide.** Vertex = satu peta, edge = jalan antarpeta (pintu, tepi peta). Daftar vertex adalah linked list; setiap
vertex punya linked list edge sendiri yang menunjuk langsung ke vertex tujuan (pointer, bukan indeks). Setiap edge
menyimpan status `open`.

**Kenapa Graph, bukan Tree.** Peta punya banyak tetangga dan ada siklus: jalan pintas Hutan Bambu → Desa membuat
lebih dari satu rute ke peta yang sama.

| Operasi | Kompleksitas | Dipakai untuk |
|---|---|---|
| `addVertex`, `addEdge` (dua arah), `addArc` (satu arah) | O(V) cari vertex | memuat `world.json`; Rawa → Sarang Buto = `addArc` |
| `findVertex`, `findEdge` | O(V), O(deg) | pindah peta lewat pintu (`travel`) |
| `setOpen(a, b, true)` | O(V + deg) | membuka jalan pintas / gerbang boss (perintah event `unlock`) |
| `bfs(start, maxDepth, fn)` | O(V + E) | Tungku terdekat & daftar tujuan warp (hanya lewat edge `open`) |
| `forEachVertex` | O(V) | istirahat di Tungku: musuh di **semua** peta hidup lagi |
| `dijkstra` (pakai `Stack` untuk jalur) | O(V² + E) | tersedia & diuji; mekanik saat ini cukup BFS |

**Subprogram M2** ([World.cpp](../src/core/world/World.cpp), [Game.cpp](../src/core/game/Game.cpp)): `travel`
(lewat pintu, tolak bila edge terkunci), `unlockShortcut`, `nearestTungku` (BFS), `restAtTungku` (traversal semua
vertex), `warpTargets` (BFS, hanya Tungku menyala), `warpTo`.

**Di demo:** F1 → daftar vertex & edge dengan status `terbuka`/`TERKUNCI`. Sebelum Memedi kalah, edge Ladang ↔ Bambu
terkunci; setelah menang terbuka. Tebas bambu di Hutan Bambu → edge Bambu ↔ Desa terbuka dan daftar warp di Tungku
berubah (BFS memendek). Console: perintah `graph` mencetak adjacency list.

---

## M3 — Interpreter event: `Stack<Frame>` + `Queue` · [Interpreter.hpp](../src/core/event/Interpreter.hpp)

**Ide.** Skrip event (dialog, pilihan, loot, battle, buka jalan, toko) dimuat dari `assets/events/*.json` menjadi
linked list `Command`; perintah bercabang (`choice`, `if_flag`, `if_item`) menyimpan daftar anak, sehingga satu
skrip membentuk pohon. Interpreter menelusurinya seperti `Game_Interpreter` RPG Maker:

- **Stack<Frame>**: setiap frame = pointer ke perintah berikutnya di satu daftar. Masuk cabang / `call` skrip lain
  = **push**; daftar habis = **pop** dan lanjut di frame bawahnya (LIFO). Karena itu cabang bersarang dan skrip yang
  memanggil skrip lain kembali ke baris pemanggil dengan benar.
- **Queue<std::string>**: notifikasi ("Didapat: Bijih Besi") ditampilkan GUI satu per satu tanpa menghentikan
  permainan.

| Operasi | Kompleksitas | Dipakai untuk |
|---|---|---|
| `push` / `pop` / `peek` | O(1) | masuk/keluar cabang dan skrip panggilan |
| `enqueue` / `dequeue` | O(1) | notifikasi & kejadian |

**Subprogram M3**: `runEvent`, `update` (jalankan sampai harus menunggu), `choose` (masuk cabang pilihan = push),
`execute` (satu perintah).

**Di demo:** F1 di luar battle menampilkan kedalaman Stack interpreter. Contoh bagus: bicara dengan Mbok Srini
setelah Memedi kalah dan Timun Mas bergabung — `if_flag` bersarang di dalam `if_flag`, lalu `choice` di dalamnya
(kedalaman 3–4), kemudian kembali ke daftar utama.

---

## Yang sengaja tidak memakai ADT

- **Inventaris** = array jumlah per id barang ([GameState.hpp](../src/core/game/GameState.hpp)). Ketentuan PDF
  melarang linked list "gear".
- **Progresi** = tukar sukma menjadi level di Tungku dan tempa senjata +0..+3, **bukan pohon skill** (contoh terlarang
  kedua di PDF).
- Flag cerita, stok pedagang = array berukuran tetap.

## Bukti & pengujian

- `mingw32-make test`: unit test ADT (`tests/test_adt.cpp`), battle (`test_battle.cpp`), event (`test_event.cpp`),
  dunia/game (`test_world.cpp`).
- Setiap operasi ADT di mekanik dicatat `dsOp(...)` ([DsTrace.hpp](../src/core/util/DsTrace.hpp)): baris `[DS]` di
  console dan panel "Operasi struktur data terbaru" di Inspector F1.
- `bin/tumbal.exe --console`: permainan yang sama tanpa GUI (bukti logika lepas dari raylib).

## Pertanyaan yang mungkin muncul saat Pendampingan

| Pertanyaan | Jawaban singkat (tunjuk kode) |
|---|---|
| Kenapa antrean giliran tidak pakai priority queue / heap? | Butuh menggeser battler di tengah (rapal, tunda) dan menampilkan urutan lengkap ke depan; DLL terurut memberi keduanya, `predictTurns` cukup menyalin list. |
| Di mana bedanya ATB dengan giliran biasa? | Laju AP bergantung AGI dan status; rapal/tunda mengubah `av` → posisi di DLL berubah. Lihat `updateAV` + `reposition`. |
| Graph-nya berbobot? Dipakai untuk apa? | Edge punya bobot & status `open`. BFS lewat edge terbuka untuk warp & Tungku terdekat; traversal penuh untuk respawn musuh. |
| Kenapa ada satu arah? | Turun ke Sarang Buto = `addArc(7, 8)`: tidak ada edge balik, jadi `travel` dan BFS tidak bisa kembali. |
| Stack di interpreter bisa diganti rekursi? | Bisa, tetapi interpreter harus bisa **berhenti di tengah** (menunggu pemain menutup dialog/memilih) lalu lanjut frame berikutnya; Stack eksplisit menyimpan posisi itu di antara frame game. |
| Apa bukti logika tidak bergantung GUI? | `src/core` tanpa `#include "raylib.h"`; `--console` dan unit test menjalankan logika yang sama. |
