# CLAUDE.md — aturan proyek TUMBAL

Dibaca otomatis oleh AI agent di setiap sesi. Proyek ini adalah **Tugas Besar Struktur Data** (kelompok 3 orang):
JRPG turn-based bernuansa cerita rakyat Nusantara versi kelam, dalam C++17 + raylib. Acuan mekanik: Black Souls 1.
Ketentuan resmi: [Konteks-Tugas/](Konteks-Tugas/). Rencana v2: [docs/PLAN_V2.md](docs/PLAN_V2.md).
Bedah sistem Black Souls: [docs/research/black-souls-1-analysis.md](docs/research/black-souls-1-analysis.md).
Prototipe pertama (TUMBAL v1) ada di branch `legacy/tumbal-v1` / tag `tumbal-v1`; jangan diubah.
Jangan memakai nama lama "Cinderwake" di commit, dokumen, atau nama branch/tag baru.

## Aturan yang TIDAK boleh dilanggar (ketentuan dosen)

1. **Struktur data yang dinilai dibuat manual** di `src/core/adt/` dengan node + pointer sendiri.
   Jangan pakai `std::list`, `std::queue`, `std::stack`, `std::vector`, `std::map`, `std::unordered_map`,
   `std::priority_queue`, `std::deque`, atau `std::sort` di `src/core/`. Yang boleh: `std::string`, `<cmath>`,
   `<cstdint>`, `std::unique_ptr`, array mentah. (GUI di `src/gui/` boleh memakai container std.)
2. **Seluruh logika mekanik ada di `src/core/` dan tidak boleh `#include "raylib.h"`.** GUI hanya membaca state dan
   memanggil API core. Jangan memindahkan aturan game ke GUI.
3. Setiap ADT harus **benar-benar dipakai** mekanik yang berjalan. Jangan buat ADT pajangan.
4. Setiap mekanik punya ≥2 **subprogram** di luar operasi primitif ADT, ditandai komentar `// M1/M2/M3 subprogram`.
   M1 = battle ATB + antrean giliran (Timeline DLL), M2 = dunia & Tungku (Graph), M3 = interpreter event (Stack + Queue).
5. Minimal 1 struktur linear + 1 non-linear.
6. Mode console (`--console`) harus tetap jalan. Ini bukti logika tidak bergantung pada GUI.
7. Yang direplikasi hanya **mekanik** Black Souls. Dunia, nama, karakter, cerita orisinal; jangan salin aset/teks/peta BS.
8. Laporan final ditulis tim sendiri. Jangan menulis laporan atas nama mereka; dokumentasi teknis boleh.
9. Inventaris jangan dibuat sebagai linked list "gear" dan progresi jangan sebagai pohon skill (dua contoh terlarang di PDF).

## Perintah

```bash
mingw32-make            # build game → bin/tumbal.exe (+ salin DLL raylib)
mingw32-make test       # build + jalankan semua unit test (wajib lulus sebelum commit)
mingw32-make run        # jalankan GUI
mingw32-make dist       # paket rilis Windows → release/TUMBAL-v<VERSION>-windows.zip (VERSION=1.0.2 bawaan)
bin/tumbal.exe --console   # mode console
bin/tumbal.exe --shots     # screenshot otomatis beberapa scene (shot_*.png)
bin/tumbal.exe --balance   # simulasi kesulitan (kelas x troop x level), lihat docs/DEMO.md
/c/msys64/ucrt64/bin/python.exe tools/build_assets.py   # assets/src → assets/tiles, assets/chars
/c/msys64/ucrt64/bin/python.exe tools/make_maps.py      # tata letak ASCII → assets/maps/*.tmj + world.json
/c/msys64/ucrt64/bin/python.exe tools/render_maps.py    # pratinjau seluruh peta → docs/map_preview/*.png
/c/msys64/ucrt64/bin/python.exe tools/lang_en.py        # kamus Inggris → assets/lang/en.json
/c/msys64/ucrt64/bin/python.exe tools/lang_keys.py      # cek cakupan terjemahan (harus 0 belum)
/c/msys64/ucrt64/bin/python.exe tools/make_gamelan.py   # musik Desa (butuh numpy + ffmpeg)
```

Toolchain: MSYS2 UCRT64 (g++, raylib 6 dari pacman, Python + Pillow). Makefile jalan dari bash maupun PowerShell.
GUI: F1 = Inspector struktur data, F3 = tampilkan collision, F9 = menu cheat, Alt+Enter/F11 = layar penuh. Shift = lari, Z = aksi/konfirmasi,
X = batal/menu (Barang, Status, Simpan, Cheat, Pengaturan, Keluar; pengaturan di `src/gui/Settings` + `SettingsView`). Console: perintah `cheat`.

## Konvensi kode

- Identifier bahasa Inggris, **komentar & teks UI sumber bahasa Indonesia**.
- **Bahasa game**: English bawaan, Bahasa Indonesia di Pengaturan (`saves/settings.json`). Teks Indonesia di
  kode/data/skrip = kunci kamus `assets/lang/en.json` (`tr("...")`, `trf("format %d", n)` di `src/core/util/Lang`;
  `ui::text` dkk. menerjemahkan otomatis). Setiap teks baru yang tampil ke pemain: tambahkan padanannya di
  `tools/lang_en.py` (gaya muram ala Black Souls; sukma=Souls, Tungku=Hearth, tumbal=Offering), jalankan
  `tools/lang_en.py` lalu `tools/lang_keys.py` (harus 0 belum). Unit test `test_lang.cpp` memeriksa cakupannya.
- Header file diawali blok komentar yang menjelaskan peran file dan struktur data yang dipakai.
- Operasi struktur data di mekanik dicatat dengan `dsOp("Graph.setOpen(...)")` (`src/core/util/DsTrace.hpp`) agar
  terlihat di trace console & Inspector GUI. Tambahkan `dsOp` setiap kali menambah operasi ADT baru di mekanik.
- Fitur mekanik baru wajib punya unit test di `tests/` (framework `CHECK` di `tests/TestFramework.hpp`).
- Kompilasi tanpa warning (`-Wall -Wextra -Wpedantic`).
- raylib `DrawTriangle` butuh urutan counter-clockwise di layar (atas, kiri-bawah, kanan-bawah).
- raylib tidak mendukung `BeginTextureMode` bersarang: siapkan render-texture lain (mis. lightmap) sebelum layar utama.
- Resolusi logis 640x360, tile 32px, sprite karakter = 4 frame x 4 arah (bawah, kiri, kanan, atas); ukuran frame
  dihitung dari ukuran gambar / 4.
- Font UI Jersey 10 (`src/gui/Ui.hpp`), selalu digambar di 10/20/30/40 px agar tajam. Jangan pakai nama warna yang
  bentrok dengan makro raylib (GOLD, RED, ...); palet ada di namespace `ui`.
- Data game (jurus, status, musuh, troop, kelas) ada di `src/core/data/GameData.cpp`; id troop juga dipakai
  `tools/make_maps.py` (objek `enemy` dengan property `troop`, `walker`).

## Peta & aset

- Peta = berkas Tiled JSON (`assets/maps/*.tmj`, tileset embedded) + `assets/maps/world.json` (daftar peta).
  Konvensi: layer `collision` (≠0 = terhalang), property layer `above` = digambar di atas karakter, objek bertipe
  `spawn`, `door` (`to`, `spawn`, `locked`, `oneway`), `tungku`, `loot`. Property peta `nama`, `gelap` (0..1), `kabut` (0..1).
- Objek `prop` = gambar besar (rumah, pohon) dari `assets/props/` (dibuat `build_props()`), digambar terurut-y
  bersama karakter; generator memblokir tapak bawahnya (`MapBuilder.prop`). Property `flat` = selalu di bawah
  karakter (dermaga). Hiasan kecil acak: `MapBuilder.scatter` (tileset `dekor.png`, layer `hiasan`) dan
  `scatter_props`. Objek event/npc boleh lebih lebar dari 1 tile (`w`); diperiksa dari ubin mana pun di dalamnya.
- Objek peta lain: `npc` (`event`, `walker`, `dir`, `once`, `after`), `event` (`event`, `trigger` = action/touch/auto,
  `once` = flag yang menandai sudah terjadi), `loot` (`item` + `n`, atau `event`), `enemy` (`troop`, `walker`, `dir`).
- Skrip event = JSON di `assets/events/*.json` (perintah: text, choice, item, sukma, flag, if_flag, if_item, battle,
  call, heal, unlock, transfer, notice, end, join, shop `kind`=dagang/tempa). Teks cerita ditulis orisinal; jangan menyalin teks Black Souls.
- Logika permainan dirangkai di `src/core/game/Game` (dipakai GUI dan console). App/console hanya meneruskan input.
- Peta saat ini dibuat oleh `tools/make_maps.py`. Peta yang disunting manual di Tiled harus dikeluarkan dari generator.
- Aset utama: CC-BY 4.0 (Sevarihk/Aurora) & CC0 dari internet. **Setiap aset baru wajib dicatat di `CREDITS.md`**
  (pembuat, URL, lisensi). Hindari lisensi share-alike dan "no redistribution".
- Codex hanya untuk celah yang tidak ada aset gratisnya. Pakai model terbaru yang tersedia (cek
  `~/.codex/models_cache.json`; saat ini `gpt-6.2-astra`, reasoning medium):
  `codex exec -m gpt-6.2-astra -c model_reasoning_effort="medium" -s workspace-write -C <repo> "<prompt>" -i <ref.png>`
  (prompt SEBELUM `-i`). Prompt disimpan di `assets/raw/codex/PROMPT_*.md`; minta latar magenta polos #FF00FF dan
  gaya sesuai gambar referensi. Hasil diproses `build_codex()` (deteksi grid piksel, buang magenta, skala) dan
  dicatat di bagian "Aset buatan AI" di `CREDITS.md`.
- Unduhan mentah disimpan di luar repo; yang dipakai disalin ke `assets/src/` lalu diproses `tools/build_assets.py`.

## Dokumentasi

- `README.md` = halaman publik (Bahasa Indonesia, format ala template open source); `docs/*.md` = dokumen tim (Indonesia).
- Setelah fitur/fase selesai, perbarui `docs/PLAN_V2.md` (status fase), `docs/DEMO.md` (rute & cek), README
  (angka test, fitur), dan screenshot `docs/screenshots/` bila tampilan berubah.

## Git

- Repo: https://github.com/DaffMe/Tumbal (public, branch `main`).
- Commit **bertahap per bagian logis** (bukan satu commit besar), diakhiri baris `Co-Authored-By` AI.
  Commit/push hanya bila diminta.
- Format pesan: `tipe(lingkup): deskripsi`. Tipe Conventional Commits saja (feat, fix, perf, refactor, docs, test,
  build, chore). Lingkup = modul (adt, battle, core, event, gui, peta, aset, audio, bahasa, tools, rilis, readme, ...),
  bukan nomor fase. Deskripsi bahasa Indonesia baku (EYD V), diawali verba dasar (tambah, perbaiki, ganti, perbarui,
  hapus), huruf kecil di awal, tanpa titik di akhir, tanpa `&` atau tanda pisah. Contoh:
  `feat(battle): tambah telegraf serangan besar Buto Ijo`. Rincian panjang ditaruh di badan commit.
- Riwayat pesan commit lengkap (sebelum dan sesudah dirapikan) ada di `Laporan/riwayat-commit.md` (tidak di-commit).
