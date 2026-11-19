# TUMBAL

JRPG turn-based bernuansa **cerita rakyat Nusantara versi kelam**, dibuat untuk
**Tugas Besar Struktur Data — Semester Ganjil 2026/2027**. Mekanik diadaptasi dari *Black Souls 1*
(RPG Maker VX Ace): battle front-view ATB, loop Tungku/Sukma ala Souls, dan dunia top-down yang saling terhubung.
Seluruh struktur data dan logika mekanik ditulis manual dalam **C++17**; GUI memakai **raylib** sebagai antarmuka saja.

> Status: **v2 hari 1 — kerangka eksplorasi.** Peta Tiled, jalan 8 arah, perpindahan peta lewat Graph dunia,
> kegelapan & cahaya Tungku, Inspector struktur data (F1), mode console.
> Rencana lengkap: [docs/PLAN_V2.md](docs/PLAN_V2.md) · Bedah sistem acuan: [docs/research/black-souls-1-analysis.md](docs/research/black-souls-1-analysis.md)

## Mekanik yang direplikasi (rencana)

| Mekanik | Dari Black Souls 1 | Struktur data | Status |
|---|---|---|---|
| M1 Battle ATB mode *wait* + antrean giliran | skrip ATB (gauge AP, rapal, stun) | Doubly Linked List terurut (`Timeline`) | ADT siap, mekanik menyusul |
| M2 Dunia saling terhubung & Tungku | hub + rantai peta, bonfire, warp, jalan pintas | Graph berarah (adjacency multi linked list) | **berjalan** (pindah peta, kunci jalan pintas, BFS Tungku) |
| M3 Interpreter event & dialog | perintah event RPG Maker | Stack (frame) + Queue (pesan) | menyusul |

## Struktur data (semua manual, tanpa container STL di `src/core`)

| File | ADT | Operasi |
|---|---|---|
| `src/core/adt/Graph.hpp` | Graph berarah berbobot, edge buka/kunci | addVertex, removeVertex, addEdge, addArc, setOpen, isOpen, removeEdge, BFS, Dijkstra, isConnected |
| `src/core/adt/Timeline.hpp` | Doubly Linked List terurut | insertSorted, remove, popFront, search, updateAV (relink), subtractAll, traversal maju/mundur |
| `src/core/adt/Queue.hpp` | Queue (linked) | enqueue, dequeue, peek, traversal |
| `src/core/adt/Stack.hpp` | Stack (linked) | push, pop, peek, traversal |
| `src/core/util/Json.hpp` | pohon first-child/next-sibling | parser JSON untuk berkas Tiled |

## Cara menjalankan

Kebutuhan: [MSYS2](https://www.msys2.org/) dengan toolchain UCRT64.

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-make
```

Pastikan `C:\msys64\ucrt64\bin` ada di `PATH`, lalu dari folder proyek:

```bash
mingw32-make test          # build + jalankan unit test
mingw32-make run           # build + jalankan game (GUI)
bin/tumbal.exe --console   # mode console (logika yang sama, tanpa GUI)
```

Kontrol: panah/WASD jalan (8 arah), Shift lari, F1 Inspector struktur data, F3 tampilkan collision.

Membangun ulang aset dan peta (butuh Python + Pillow dari MSYS2):

```bash
/c/msys64/ucrt64/bin/python.exe tools/build_assets.py
/c/msys64/ucrt64/bin/python.exe tools/make_maps.py
```

## Anggota kelompok

| Nama | NIM | Peran |
|---|---|---|
| _(isi)_ | _(isi)_ | Programmer game |
| _(isi)_ | _(isi)_ | Perencanaan |
| _(isi)_ | _(isi)_ | Laporan |

## Penggunaan AI

Pengembangan kode dibantu AI (Claude Code). Rincian penggunaan dicantumkan di laporan sesuai ketentuan tugas.

## Kredit

Aset pihak ketiga (CC-BY 4.0 / CC0) tercantum di [CREDITS.md](CREDITS.md).
