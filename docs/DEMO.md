# Panduan Demo TUMBAL

Dokumen teknis untuk uji coba dan presentasi: rute permainan, titik untuk memperlihatkan tiap struktur data,
dan cara cepat mengecek game.

## Rute lengkap (±45–60 menit)

| # | Wilayah | Isi | Struktur data yang terlihat |
|---|---|---|---|
| 1 | **Sel Tumbal** | Bangun di sel. Lelaki tua, Tungku pertama, satu Tumbal Gagal. | M3 dialog (Stack frame), M1 battle pertama |
| 2 | **Lorong Lumbung** | Musuh yang mengejar, Abu Pulang, boss kecil **Penjaga Lumbung** di gerbang. | M1 antrean giliran, rapal & buyar rapal |
| 3 | **Desa Karangtumbal** (hub) | Mbok Srini, warga lesung, Tungku desa. Pintu selatan ke hutan bambu terkunci. | M2 edge terkunci (Graph) |
| 4 | **Ladang Timun Busuk** | Belalang & Timun Busuk. Boss **Memedi Sawah**: hentikan rapal "Ritual Panen". Hadiah **Biji Timun**. Kemenangan membuka jalan ke timur. | M3 `battle` + `unlock` dari event; M2 `Graph.setOpen` |
| 5 | **Hutan Bambu Jarum** | Arwah, gagak, ular. Makam Jarum (**Jarum**). Tebas bambu = jalan pintas balik ke desa. | M2 jalan pintas → BFS warp memendek |
| 6 | **Laut Garam** | Hantu laut & tengkorak. **Timun Mas** bergabung (**Garam**); party jadi dua. | M1 party 2 orang di Timeline DLL |
| 7 | **Rawa Terasi** | Akar & buaya. **Terasi**. Turun ke sarang = **jalan satu arah**. | M2 `Graph.addArc` (satu arah) |
| 8 | **Sarang Buto** | Boss akhir **Buto Ijo** (kepala + dua tangan; kepala tumbang = tangan runtuh). Tidak bisa kabur. | M1 boss multi-bagian |
| — | Tamat | Layar tamat → kembali ke judul. | |

Kurva kesulitan (diukur dengan simulasi battle otomatis per troop × level × kelas): pemain yang melawan semua
musuh sekali tiba di Hutan Bambu sekitar level 5–6, di Laut level 7, dan di Buto level 8–9.
Buto bisa dikalahkan di level 9 dengan hati-hati, dan aman di level 11. Musuh muncul lagi setelah
istirahat di Tungku, jadi pemain bisa mengumpulkan sukma lebih dulu.

## Loop Souls yang perlu ditunjukkan

1. Kumpulkan sukma, lalu **mati** dengan sengaja. Sukma tercecer (`~` di console, nyala merah di peta).
2. Bangkit di Tungku terakhir; semua musuh hidup lagi (traversal Graph).
3. Ambil kembali sukma di tempat mati. Mati lagi sebelum mengambilnya → sukma lama hilang.
4. Trik BS1: **simpan kapan saja** (menu X → Simpan), lalu muat ulang dari judul untuk membatalkan kematian.

## Titik presentasi struktur data

- **F1 Inspector** (GUI): isi Timeline DLL (saat battle), kedalaman Stack interpreter, Graph dunia dengan status
  edge terbuka/TERKUNCI, dan jejak operasi `dsOp` terbaru secara langsung. Queue terlihat lewat notifikasi yang
  muncul satu per satu dan animasi battle yang diputar berurutan.
- **Battle**: strip "Giliran" di atas = `Timeline::predictTurns` (simulasi pada salinan DLL).
- **Tungku → Warp**: daftar tujuan = BFS dari peta sekarang, hanya Tungku menyala yang terjangkau lewat edge terbuka.
- **Console** (`bin/tumbal.exe --console`): perintah `graph` mencetak adjacency list; setiap operasi ADT tercetak
  sebagai baris `[DS]`. Perintah `simpan 1` / `muat 1` memakai format save yang sama dengan GUI.

## Kontrol

Panah/WASD jalan (8 arah), Shift lari, Z/Enter aksi/konfirmasi, X/Esc menu/batal, tahan Z di battle untuk
mempercepat. Di menu Tungku halaman Timun Mas: C = panggil/lepas pendamping. F1 Inspector, F3 collision.

## Cek cepat

```bash
mingw32-make test            # 283 unit test
bin/tumbal.exe --shots       # screenshot otomatis: judul, bambu, laut, rawa, sarang, battle Buto
bin/tumbal.exe --console     # mode teks
```

Audio: musik per peta (property peta `musik`) dengan crossfade, musik battle/boss, efek suara menu & battle
(`src/gui/Audio.cpp`). Semua berkas CC0, lihat `CREDITS.md`.
