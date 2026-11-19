# TUMBAL

JRPG turn-based bernuansa **cerita rakyat Nusantara versi kelam**, dibuat untuk
**Tugas Besar Struktur Data — Semester Ganjil 2026/2027**. Mekanik diadaptasi dari *Black Souls 1*
(RPG Maker VX Ace): battle front-view ATB, loop Tungku/Sukma ala Souls, dan dunia top-down yang saling terhubung.
Seluruh struktur data dan logika mekanik ditulis manual dalam **C++17**; GUI memakai **raylib** sebagai antarmuka saja.

> Status: **demo lengkap dapat dimainkan dari awal sampai tamat (±45–60 menit).** Delapan peta: Sel Tumbal →
> Lorong Lumbung → Desa Karangtumbal (hub) → Ladang Timun Busuk (boss Memedi Sawah) → Hutan Bambu Jarum →
> Laut Garam (Timun Mas bergabung) → Rawa Terasi → Sarang Buto (boss akhir Buto Ijo). Battle ATB dengan antrean
> giliran, Tungku (naik level, warp BFS), sukma tercecer, pendamping, simpan/muat 3 slot, layar judul & tamat,
> musik & efek suara, Inspector (F1), mode console.
> Panduan uji & presentasi: [docs/DEMO.md](docs/DEMO.md) · Struktur data: [docs/STRUKTUR_DATA.md](docs/STRUKTUR_DATA.md) ·
> Rencana: [docs/PLAN_V2.md](docs/PLAN_V2.md) ·
> Bedah sistem acuan: [docs/research/black-souls-1-analysis.md](docs/research/black-souls-1-analysis.md)

## Mekanik yang direplikasi

| Mekanik | Dari Black Souls 1 | Struktur data | Status |
|---|---|---|---|
| M1 Battle ATB mode *wait* + antrean giliran | skrip ATB (gauge AP, rapal, stun) | Doubly Linked List terurut (`Timeline`) + Queue `BattleEvent` | **berjalan** (gauge AP dari AGI, rapal & buyar rapal, tunda giliran, Elak/Tangkis, AI musuh, prediksi antrean) |
| M2 Dunia saling terhubung & Tungku | hub + rantai peta, bonfire, warp, jalan pintas, musuh terlihat | Graph berarah (adjacency multi linked list) | **berjalan** (pindah peta, kunci jalan pintas, warp hanya ke Tungku yang terjangkau lewat BFS, respawn musuh lewat traversal) |
| M3 Interpreter event & dialog | perintah event RPG Maker (teks, pilihan, if, panggil, battle, barang) | Stack (frame eksekusi) + Queue (notifikasi) | **berjalan** (cabang bersarang, panggil skrip lain, flag cerita, battle dari event) |

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

Kontrol: panah/WASD jalan (8 arah), Shift lari, Z/Enter aksi & konfirmasi (istirahat di Tungku), X/Esc batal,
tahan Z saat battle untuk mempercepat animasi, F1 Inspector struktur data, F3 tampilkan collision.
Simpan lewat menu (X → Simpan); kematian tidak menyimpan otomatis, jadi memuat ulang bisa membatalkannya (seperti BS1).

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
