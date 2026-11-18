# Cinderwake — Art Bible

Panduan gaya untuk semua aset visual. Dibaca oleh Codex (pembuat aset) dan dipakai saat post-process.
Dunia, karakter, dan desain di sini **orisinal**: jangan meniru karakter, logo, atau gaya khas game/film/seniman tertentu.

## 1. Arah visual

- **Tema:** katedral gothic yang ditelan api yang sekarat. Lentera adalah satu-satunya harapan.
- **Gaya:** lukisan digital dark fantasy, painterly, sapuan kuas terlihat, detail tinggi pada fokus dan lebih longgar di tepi.
- **Cahaya:** satu sumber hangat yang kuat (lilin, bara, lentera) dari kiri atas atau bawah, dengan bayangan sangat pekat. Rim light oranye pada siluet.
- **Mood:** suram, sakral, dan membusuk, tapi tetap indah. Tidak gore berlebihan.

## 2. Palet

| Peran | Warna |
|---|---|
| Bayangan / latar | `#0C0A0E` `#1A1418` `#2A2024` |
| Batu & kain | `#4A3E38` `#6E5E50` |
| Tinta / teks | `#DECEAA` |
| Emas pudar | `#CEA65C` |
| Bara / api | `#FF8C32` `#FFC878` |
| Darah / bahaya | `#B22C28` |
| Arcane (sekunder, hemat) | `#9678DC` |

Lantai 1 bernuansa ungu-cokelat hangat, lantai 2 biru dingin, lantai 3 merah membara.

## 3. Aturan teknis

- Format **PNG**.
- **Battler musuh** dan **ikon**: latar **transparan** (alpha), subjek utuh tidak terpotong, sedikit ruang kosong di sekeliling. Tanpa bayangan lantai (bayangan digambar oleh game).
- Battler musuh menghadap ke depan atau sedikit menyamping (tampilan front-view, pemain melihat musuh dari depan), dengan kaki atau dasar tubuh di tepi bawah gambar.
- **Potret hero**: setengah badan (bust), latar gelap bervignette (tidak perlu transparan), wajah di sepertiga atas.
- **Latar**: 16:9, tanpa teks, tanpa karakter. Latar battle harus menyisakan area tengah-bawah (garis lantai sekitar 65% tinggi) yang lapang untuk menaruh musuh.
- Tidak ada teks, huruf, watermark, tanda tangan, atau bingkai di dalam gambar.

## 4. Daftar aset

Simpan hasil mentah di `assets/raw/` dengan path yang sama seperti kolom **File**. Versi final (setelah post-process) ada di `assets/final/`.

### Musuh (transparan, potret ±3:4)

| File | Deskripsi |
|---|---|
| `enemies/hollow_thrall.png` | Bekas jemaat yang hampa, tubuh kurus membungkuk, kulit abu-abu retak, jubah compang-camping, mata menyala oranye redup, lengan panjang bercakar |
| `enemies/bell_ringer.png` | Penjaga lonceng berjubah tinggi bertudung, wajah tertutup kain, membawa lonceng perunggu besar berkarat pada rantai |
| `enemies/candle_wraith.png` | Arwah melayang dari lilin meleleh, tubuh lilin pucat menetes, beberapa sumbu menyala di kepala, bagian bawah memudar menjadi asap |
| `enemies/wisp.png` | Bola api biru pucat kecil dengan ekor api, wajah samar di dalam nyala |
| `enemies/gargoyle.png` | Gargoyle batu besar berjongkok, sayap batu retak, lumut, mata hijau menyala, tanduk melengkung |
| `enemies/choir_warden.png` | BOSS lantai 1: sosok raksasa berjubah paduan suara merah tua dan emas, halo patah di belakang kepala, wajah topeng porselen retak dengan banyak mata kecil menyala, memegang tongkat pendeta |
| `enemies/sister_of_ash.png` | BOSS lantai 2: biarawati jangkung yang tubuhnya tersusun dari serpihan abu kelabu yang terus berguguran, kerudung panjang, memegang guci batu yang menuang abu, nuansa biru dingin |
| `enemies/last_flame.png` | BOSS lantai 3: sosok kolosal dari api hidup berbentuk santo bermahkota, rusuk dari logam cair, rambut berupa lidah api, kedua lengan terentang |

### Hero (potret bust, 3:4, latar gelap)

| File | Deskripsi |
|---|---|
| `heroes/varga.png` | Varga, the Oathbound: ksatria tua berarmor baja gelap penuh goresan, perisai besar, jenggot abu-abu, mata lelah tapi teguh |
| `heroes/sable.png` | Sable, the Ash Witch: penyihir muda berjubah hitam berabu, rambut putih pucat, tangan mengeluarkan bara, tatapan tajam |
| `heroes/ione.png` | Ione, the Lamplighter: penjaga lentera berjubah krem lusuh, membawa lentera kuningan menyala, wajah lembut diterangi cahaya hangat |

### Latar (16:9)

| File | Deskripsi |
|---|---|
| `bg/battle_floor1.png` | Lorong tengah katedral (nave) dengan bangku rusak, pilar gothic, kaca patri ungu redup, lilin di lantai |
| `bg/battle_floor2.png` | Krypta bawah tanah, peti batu, tulang di relung dinding, kabut biru dingin |
| `bg/battle_floor3.png` | Jantung katedral yang terbakar, altar raksasa dengan api, bara beterbangan, merah membara |
| `bg/map.png` | Permukaan meja batu atau perkamen tua sangat gelap dengan noda lilin, tekstur halus tanpa gambar peta |
| `bg/title.png` | Key art: siluet tiga pengembara kecil dengan satu lentera, berdiri di depan pintu katedral raksasa yang menyala dari dalam. Bagian atas tengah lebih kosong untuk judul |

### Ikon ruangan (transparan, 1:1, ukiran garis emas pudar)

`icons/room_entrance.png` (pintu lengkung), `icons/room_battle.png` (dua pedang bersilang),
`icons/room_elite.png` (tengkorak bertanduk), `icons/room_shrine.png` (api kecil di atas mangkuk batu),
`icons/room_event.png` (mata di dalam segitiga / tanda misteri), `icons/room_merchant.png` (kantong koin),
`icons/room_treasure.png` (peti kayu berbingkai besi), `icons/room_boss.png` (mahkota retak).

## 5. Template prompt

```
Dark fantasy painterly digital illustration, <SUBJEK>.
Gothic decaying cathedral mood, single warm candle-light source with deep black shadows,
orange rim light, muted desaturated palette (soot black, ash brown, faded gold, ember orange),
visible brush strokes, highly detailed focal area, original character design.
<ATURAN TEKNIS: transparent background / 16:9 background / bust portrait>.
No text, no watermark, no frame.
```
