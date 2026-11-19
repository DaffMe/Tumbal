# CLAUDE.md — aturan proyek Cinderwake

Dibaca otomatis oleh AI agent di setiap sesi. Proyek ini adalah **Tugas Besar Struktur Data** (kelompok 3 orang):
game turn-based dark fantasy dalam C++17 + raylib. Ketentuan resmi: [Konteks-Tugas/](Konteks-Tugas/).
Peta kode: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). Rencana & justifikasi: [docs/PLAN.md](docs/PLAN.md).

## Aturan yang TIDAK boleh dilanggar (ketentuan dosen)

1. **Struktur data yang dinilai dibuat manual** di `src/core/adt/` dengan node + pointer sendiri.
   Jangan pakai `std::list`, `std::queue`, `std::stack`, `std::vector`, `std::map`, `std::priority_queue`,
   `std::deque`, atau `std::sort` di `src/core/`. Yang boleh: `std::string`, `<cmath>`, `<cstdint>`, `std::unique_ptr`.
2. **Seluruh logika mekanik ada di `src/core/` dan tidak boleh `#include "raylib.h"`.** GUI (`src/gui/`) hanya
   membaca state dan memanggil API core. Jangan memindahkan aturan game ke GUI.
3. Setiap ADT harus **benar-benar dipakai** mekanik yang berjalan. Jangan buat ADT pajangan.
4. Setiap mekanik punya ≥2 **subprogram** di luar operasi primitif ADT, ditandai komentar `// M1/M2/M3 subprogram`.
5. Minimal 1 struktur linear + 1 non-linear (saat ini: Timeline DLL, Queue, Stack + Graph).
6. Mode console (`--console`) harus tetap jalan. Ini bukti logika tidak bergantung pada GUI.
7. Yang direplikasi hanya **mekanik** game sumber. Dunia, nama, karakter, dan art harus orisinal.
8. Laporan final ditulis tim sendiri. Jangan menulis laporan atas nama mereka; dokumentasi teknis boleh.

## Perintah

```bash
mingw32-make            # build game → bin/cinderwake.exe (+ salin DLL raylib)
mingw32-make test       # build + jalankan semua unit test (wajib lulus sebelum commit)
mingw32-make run        # jalankan GUI
bin/cinderwake.exe --console   # mode console
bin/cinderwake.exe --shots     # screenshot otomatis beberapa scene (shot_*.png)
bin/cinderwake.exe --gallery   # screenshot semua aset (gallery_*.png)
bin/cinderwake.exe --balance 200   # simulasi balancing (jalankan ulang setelah mengubah stat/skill)
/c/msys64/ucrt64/bin/python.exe tools/process_assets.py   # assets/raw → assets/final
```

Toolchain: MSYS2 UCRT64 (g++, raylib dari pacman). Makefile jalan dari bash maupun PowerShell.

## Konvensi kode

- Identifier bahasa Inggris, **komentar & teks UI bahasa Indonesia**.
- Header file diawali blok komentar yang menjelaskan peran file dan struktur data yang dipakai.
- Logika battle menghasilkan `BattleEvent` ke Queue; operasi struktur data dicatat sebagai event `DsOp`
  (`dsOp("Timeline.updateAV(...)")`) agar terlihat di trace console & Inspector GUI. Tambahkan `dsOp` setiap kali
  menambah operasi ADT baru di mekanik.
- Fitur mekanik baru wajib punya unit test di `tests/` (framework `CHECK` di `tests/TestFramework.hpp`).
- Kompilasi tanpa warning (`-Wall -Wextra -Wpedantic`).
- raylib `DrawTriangle` butuh urutan counter-clockwise. Di `Art.cpp` pakai helper `tri()`.

## Aset

- Visual dibuat oleh **Codex** (`codex exec ... "<prompt>" -i <referensi.png>`; **prompt sebelum `-i`**, karena `-i` menelan
  argumen berikutnya) mengikuti [docs/ART_BIBLE.md](docs/ART_BIBLE.md), disimpan di `assets/raw/`.
- Claude mengerjakan detail: post-process (`tools/process_assets.py`), integrasi, shader, animasi, audio prosedural.
- Game otomatis memakai `assets/final/...` bila ada, dan placeholder prosedural bila tidak.

## Git

- Repo: https://github.com/DaffMe/Cinderwake (public, branch `main`).
- Commit **bertahap per bagian logis** (bukan satu commit besar), pesan bahasa Indonesia gaya `feat(x): ...`,
  diakhiri baris `Co-Authored-By` AI. Commit/push hanya bila diminta.
