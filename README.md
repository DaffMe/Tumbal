# Cinderwake

Dungeon crawler turn-based bertema dark fantasy, dibuat untuk **Tugas Besar Struktur Data — Semester Ganjil 2026/2027**.
Seluruh struktur data dan logika mekanik ditulis manual dalam **C++17**. GUI memakai **raylib** sebagai antarmuka saja.

> Status: **Fase 3 — GUI** (ekspedisi bisa dimainkan penuh dengan GUI raylib, aset masih placeholder prosedural). Rencana lengkap ada di [docs/PLAN.md](docs/PLAN.md).

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
| `src/core/adt/Graph.hpp` | Graph tak berarah berbobot | addVertex, removeVertex, addEdge, removeEdge, findVertex, neighbors, BFS, Dijkstra (dengan filter ruang terungkap), isConnected |
| `src/core/adt/Queue.hpp` | Queue (linked) | enqueue, dequeue, peek, traversal |
| `src/core/adt/Stack.hpp` | Stack (linked) | push, pop, peek, traversal |

## Cara menjalankan

Kebutuhan: [MSYS2](https://www.msys2.org/) dengan toolchain UCRT64.

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-make
```

Pastikan `C:\msys64\ucrt64\bin` ada di `PATH`, lalu dari folder proyek:

```bash
mingw32-make test   # build + jalankan unit test ADT dan mekanik
mingw32-make run    # build + jalankan game (GUI)
bin/cinderwake.exe --console   # mode console
```

### Kontrol GUI

| Tombol | Fungsi |
|---|---|
| Klik ruang | Berjalan ke ruang bertetangga / auto-travel (Dijkstra) ke ruang terungkap |
| `S` | Auto-travel ke Ember Shrine terdekat |
| Klik skill / `1`-`3` / `U` | Pilih skill; klik musuh atau sekutu sebagai target |
| `SELA` di kartu hero | Interject (Resolve 100%) |
| `R` / tombol Rewind | Kembali ke giliran hero sebelumnya (Stack snapshot) |
| Tahan `Space` | Percepat animasi |
| **`Tab`** | **Inspector struktur data**: node Timeline + pointer prev/next, Stack rewind, Event Queue, adjacency list Graph, log operasi |
| `F11` | Layar penuh |

Logika di GUI dan console sama persis: GUI hanya memanggil `Run`/`Battle` di `src/core` lalu menganimasikan isi Battle Event Queue.

### Mode console

**Ekspedisi (`e`)** — run penuh 3 lantai:
- Peta grid dengan fog of war; ruang terungkap lewat **BFS** dengan radius sesuai cahaya lentera (terang 2, redup 1, gelap 0)
- Setiap koridor menghabiskan cahaya sesuai **bobot edge**; makin gelap, makin besar peluang musuh menyergap (giliran musuh dimajukan di Timeline)
- `g <id>` auto-travel lewat jalur **Dijkstra**, `s` ke Ember Shrine terdekat, `m` cetak **adjacency list** graph
- Ember Shrine = checkpoint. Party tumbang → bangkit di checkpoint, Ash tertinggal di ruang kematian dan bisa dipungut kembali
- Pedagang, peristiwa (termasuk koridor runtuh = `removeEdge` + cek `isConnected`), boss, dan relic

**Pertarungan cepat (`1-5`)** — langsung ke satu encounter. Setiap giliran hero menampilkan:
- HP, Resolve, Toughness/Break, dan status setiap unit
- **Isi node Timeline (Doubly Linked List)** dari HEAD ke TAIL beserta Action Value
- **Prediksi 8 giliran** berikutnya (simulasi pada salinan timeline)
- Trace operasi struktur data (`[DS] Timeline.updateAV(...)`, `Stack.push(...)`), bisa dimatikan dengan `d`

Aksi: `1-3` skill, `i` Interject (Resolve 100%), `r` Rewind (Cracked Hourglass), `a` di menu untuk mode lentera gelap (ambush).

## Struktur folder

```
src/core/      ADT + logika mekanik (C++ murni, tidak bergantung raylib)
src/gui/       tampilan raylib (scene peta & battle, Inspector, shader, placeholder art)
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
