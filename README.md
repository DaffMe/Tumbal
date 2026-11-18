# Cinderwake

Dungeon crawler turn-based bertema dark fantasy, dibuat untuk **Tugas Besar Struktur Data — Semester Ganjil 2026/2027**.
Seluruh struktur data dan logika mekanik ditulis manual dalam **C++17**. GUI memakai **raylib** sebagai antarmuka saja.

> Status: **Fase 0 — fondasi** (ADT + unit test). Rencana lengkap ada di [docs/PLAN.md](docs/PLAN.md).

## Mekanik yang direplikasi

| Mekanik | Game sumber | Struktur data |
|---|---|---|
| M1 Action Timeline (urutan giliran) | Honkai: Star Rail, Final Fantasy X | Doubly Linked List terurut |
| M2 Dungeon Exploration dengan cahaya | Darkest Dungeon | Graph (adjacency multi linked list) |
| M3 Rewind giliran | Into the Breach | Stack |
| Battle Event Queue (pendukung) | — | Queue |

## Struktur data (semua manual, tanpa container STL)

| File | ADT | Operasi |
|---|---|---|
| `src/core/adt/Timeline.hpp` | Doubly Linked List terurut | insertSorted, insertFront, remove, popFront, search, updateAV (relink), subtractAll, traversal maju/mundur |
| `src/core/adt/Graph.hpp` | Graph tak berarah berbobot | addVertex, removeVertex, addEdge, removeEdge, findVertex, neighbors, BFS, Dijkstra, isConnected |
| `src/core/adt/Queue.hpp` | Queue (linked) | enqueue, dequeue, peek, traversal |
| `src/core/adt/Stack.hpp` | Stack (linked) | push, pop, peek, traversal |

## Cara menjalankan

Kebutuhan: [MSYS2](https://www.msys2.org/) dengan toolchain UCRT64.

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-make
```

Pastikan `C:\msys64\ucrt64\bin` ada di `PATH`, lalu dari folder proyek:

```bash
mingw32-make test   # build + jalankan unit test ADT
mingw32-make run    # build + jalankan game
```

## Struktur folder

```
src/core/      ADT + logika mekanik (C++ murni, tidak bergantung raylib)
src/gui/       tampilan raylib
src/console/   mode console (logika yang sama tanpa GUI)
tests/         unit test
assets/        aset visual & audio
docs/          rencana, art bible, skrip demo
```

## Anggota kelompok

| Nama | NIM | Peran |
|---|---|---|
| _(isi)_ | _(isi)_ | Programmer game |
| _(isi)_ | _(isi)_ | Perencanaan |
| _(isi)_ | _(isi)_ | Laporan |

## Penggunaan AI

Pengembangan kode dan aset dibantu AI (Claude Code untuk kode, Codex untuk aset visual). Rincian penggunaan dicantumkan di laporan sesuai ketentuan tugas.
