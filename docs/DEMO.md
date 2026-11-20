# Panduan Demo TUMBAL

Dokumen teknis untuk uji coba dan presentasi: rute permainan, titik untuk memperlihatkan tiap struktur data,
dan cara cepat mengecek game. Nama di bawah memakai istilah Indonesia (sumber); di game versi English (bawaan)
istilahnya: sukma = Souls, Tungku = Hearth, tumbal = Offering. Bahasa diganti di **Settings**.

## Rute lengkap (±45–60 menit)

| # | Wilayah | Isi | Struktur data yang terlihat |
|---|---|---|---|
| 1 | **Sel Tumbal** | Bangun di sel. Lelaki tua, Tungku pertama, satu Tumbal Gagal. | M3 dialog (Stack frame), M1 battle pertama |
| 2 | **Lorong Lumbung** | Musuh yang mengejar, Abu Pulang, boss kecil **Penjaga Lumbung** di gerbang. | M1 antrean giliran, rapal & buyar rapal |
| 3 | **Desa Karangtumbal** (hub) | Mbok Srini, warga lesung, Tungku desa, **Empu Karta** (tempa senjata +1..+3 dengan sukma + Bijih Besi), **pedagang keliling** (Kendi Jamu menambah wadah jamu). Pintu selatan ke hutan bambu terkunci. | M2 edge terkunci (Graph) |
| 4 | **Ladang Timun Busuk** | Belalang & Timun Busuk. Boss **Memedi Sawah**: hentikan rapal "Ritual Panen". Hadiah **Biji Timun**. Kemenangan membuka jalan ke timur. | M3 `battle` + `unlock` dari event; M2 `Graph.setOpen` |
| 5 | **Hutan Bambu Jarum** | Arwah, gagak, ular. Makam Jarum (**Jarum**). Tebas bambu = jalan pintas balik ke desa. | M2 jalan pintas → BFS warp memendek |
| 6 | **Laut Garam** | Hantu laut & tengkorak. **Timun Mas** di ujung dermaga bergabung (**Garam**); party jadi dua. Rumah nelayan menyimpan sukma. | M1 party 2 orang di Timeline DLL |
| 7 | **Rawa Terasi** | Akar & buaya. Pemuda desa penjaga (memberi jamu). **Terasi**. Turun ke sarang = **jalan satu arah**. | M2 `Graph.addArc` (satu arah) |
| 8 | **Sarang Buto** | Lereng bertiang tengkorak, tumpukan tulang. Boss akhir **Buto Ijo** (kepala + dua tangan; kepala tumbang = tangan runtuh). Telegraf: "Lapar Buto" dan "Hantam Gada" dirapal dulu, nama jurus muncul di atas musuh dan baris party yang diincar berkedip; tunda/buyarkan rapalnya. Tidak bisa kabur. | M1 boss multi-bagian |
| — | Tamat | Layar tamat → kembali ke judul. | |

Kurva kesulitan diukur dengan `bin/tumbal.exe --balance` (simulasi battle otomatis per kelas × troop × level,
40 battle per sel, BattleSystem yang sama dengan game). Pemain yang melawan semua musuh sekali tiba di
Hutan Bambu sekitar level 5–6, di Laut level 7, dan di Buto level 8–9. Musuh muncul lagi setelah istirahat
di Tungku, jadi pemain bisa mengumpulkan sukma lebih dulu.

Hasil `--balance` terakhir (fase 12): musuh biasa dan Memedi sesuai kurva (Memedi menang ≥90% di L5–6).
**Buto Ijo lebih berat dari kurva:** bot (hanya jamu, jurus serang, tunda rapal; tanpa Elak/Tangkis, bom,
dan bekal Timun Mas) baru menang ±50% di L10–11 (Maling/Pawang) dan L12 (Pendekar); senjata +3 menurunkannya
±1 level. Pemain sungguhan memakai lebih banyak alat daripada bot, jadi angka ini batas atas. **Perlu dicek
lewat playtest** sebelum Buto dilemahkan; Pendekar paling terdampak.

## Loop Souls yang perlu ditunjukkan

1. Kumpulkan sukma, lalu **mati** dengan sengaja. Sukma tercecer (`~` di console, nyala merah di peta).
2. Bangkit di Tungku terakhir; semua musuh hidup lagi (traversal Graph).
3. Ambil kembali sukma di tempat mati. Mati lagi sebelum mengambilnya → sukma lama hilang.
4. Trik BS1: **simpan kapan saja** (menu X → Simpan), lalu muat ulang dari judul untuk membatalkan kematian.

## Titik presentasi struktur data

- **F1 Inspector** (GUI): isi Timeline DLL (saat battle), kedalaman Stack interpreter, Graph dunia dengan status
  edge terbuka/TERKUNCI, dan jejak operasi `dsOp` terbaru secara langsung. Queue terlihat lewat notifikasi yang
  muncul satu per satu dan animasi battle yang diputar berurutan.
- **Battle**: strip "Giliran" / "Turns" di atas = `Timeline::predictTurns` (simulasi pada salinan DLL).
  Telegraf: `BattleSystem::threatOn` menandai anggota party yang diincar jurus rapalan musuh.
- **Tungku → Warp**: daftar tujuan = BFS dari peta sekarang, hanya Tungku menyala yang terjangkau lewat edge terbuka.
- **Console** (`bin/tumbal.exe --console`): perintah `graph` mencetak adjacency list; setiap operasi ADT tercetak
  sebagai baris `[DS]`. Perintah `simpan 1` / `muat 1` memakai format save yang sama dengan GUI.

## Kontrol

Panah/WASD jalan (8 arah), Shift lari, Z/Enter aksi/konfirmasi, X/Esc menu/batal, tahan Z di battle untuk
mempercepat. Di menu Tungku halaman Timun Mas: C = panggil/lepas pendamping. F1 Inspector, F3 collision.
Bahasa: **English** bawaan; ganti ke Bahasa Indonesia di **Settings** (layar judul atau menu X), tersimpan di
`saves/settings.json`. Settings juga berisi mode layar (Windowed/Borderless/Fullscreen eksklusif; cepat: Alt+Enter
atau F11), ukuran jendela, skala gambar (pixel-perfect/penuhi layar), VSync, batas FPS, volume musik & efek, dan
tampilkan FPS. Menu X → **Keluar** (ke layar judul / keluar game) dan **Cheat** (juga F9; console: `cheat`): sukma +1000, naik level
gratis, tambah stat, pulih penuh, bekal barang, senjata +3, kebal, sekali pukul, buka semua jalan & Tungku, panggil
Timun Mas. Pemakaian cheat tersimpan di berkas simpan dan disebut di layar tamat.

## Cek cepat

```bash
mingw32-make test            # 377 unit test (termasuk cakupan terjemahan)
bin/tumbal.exe --shots       # screenshot otomatis: judul, 8 peta, dialog + potret, battle, menu, cheat, toko, tempa, Buto
/c/msys64/ucrt64/bin/python.exe tools/lang_keys.py     # cek semua teks punya terjemahan Inggris
/c/msys64/ucrt64/bin/python.exe tools/render_maps.py   # pratinjau seluruh peta -> docs/map_preview/*.png
bin/tumbal.exe --console     # mode teks
bin/tumbal.exe --balance     # simulasi kesulitan: level menang 50%/90% per troop & kurva Buto (senjata +0/+3)
```

Audio: musik per peta (property peta `musik`) dengan crossfade, musik battle/boss, efek suara menu & battle
(`src/gui/Audio.cpp`). Musik pihak ketiga CC0; musik Desa = gending gamelan orisinal (`tools/make_gamelan.py`).
Lihat `CREDITS.md`.

Screenshot versi English untuk README ada di `docs/screenshots/` (salinan hasil `--shots`).
