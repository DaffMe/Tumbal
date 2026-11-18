# KONTEKS PENUGASAN --- TUGAS BESAR STRUKTUR DATA

## Replikasi Mekanik Game \| Semester Ganjil 2026/2027

> **Dokumen konteks untuk AI Agent**
>
> Dokumen ini menggabungkan isi instruksi resmi **Tugas Besar Struktur
> Data Semester Ganjil 2026/2027** (PDF, 8 halaman) dengan **klarifikasi
> tambahan dari dosen mengenai penggunaan GUI**.
>
> Gunakan dokumen ini sebagai acuan utama saat merancang,
> mengimplementasikan, menguji, mendokumentasikan, dan mengevaluasi
> proyek. Jangan mengabaikan ketentuan struktur data hanya demi membuat
> game terlihat menarik.

------------------------------------------------------------------------

## 1. Ringkasan dan tujuan tugas

Tugas besar ini meminta mahasiswa **mereplikasi satu atau lebih mekanik
dari sebuah game** ke dalam program/simulasi yang memanfaatkan struktur
data linear dan non-linear.

Fokus penilaian bukan membuat game lengkap, melainkan:

1.  Mendefinisikan, merancang, dan mengimplementasikan struktur data
    linear maupun non-linear.
2.  Memilih struktur data yang sesuai dengan kebutuhan mekanik game dan
    memberikan justifikasi pemilihannya.
3.  Mengelola data, hubungan antar-data, dan operasi yang diperlukan
    oleh mekanik.
4.  Mengimplementasikan mekanik game dengan logika program yang benar.
5.  Mendokumentasikan proses pengembangan, termasuk penggunaan GitHub.

Mekanik boleh diadaptasi atau disederhanakan dari game sumber.
Penyederhanaan diperbolehkan selama konsep utama mekanik aslinya masih
dapat dikenali dan dijelaskan.

**Prinsip utama:** struktur data harus benar-benar digunakan oleh
mekanik yang berjalan. Jangan membuat ADT hanya sebagai formalitas atau
pajangan.

## 2. Ketentuan inti yang wajib dipenuhi

### 2.1 Jumlah anggota

-   Tugas dapat dikerjakan sendiri atau berkelompok.
-   Jumlah anggota kelompok maksimal **3 orang**.
-   Kelompok wajib melaporkan anggota kelompoknya.

### 2.2 Struktur data wajib

Setiap mahasiswa/kelompok wajib mengimplementasikan minimal **dua
struktur data**, yang terdiri atas:

  Kategori     Struktur data yang diperbolehkan
  ------------ ------------------------------------------------------
  Linear       Singly Linked List, Doubly Linked List, Stack, Queue
  Non-linear   Multi Linked List, Tree, Graph

Ketentuan: - Minimal satu struktur data linear. - Minimal satu struktur
data non-linear. - Boleh menggunakan lebih banyak struktur data apabila
memang dibutuhkan. - Satu mekanik boleh menggunakan lebih dari satu
struktur data. - Struktur data linear dan non-linear boleh bekerja
bersama untuk mendukung satu mekanik.

### 2.3 Implementasi wajib menggunakan C++ dan ADT manual

Berdasarkan klarifikasi tambahan dosen:

-   Implementasi utama seluruh struktur data **wajib menggunakan C++**.
-   Seluruh logika mekanik game **tetap menggunakan C++**.
-   Struktur data wajib dibuat/diimplementasikan sendiri, bukan
    digantikan oleh library atau container siap pakai.
-   Implementasi harus memperlihatkan cara kerja struktur data, termasuk
    manipulasi data dan relasi antar-node jika relevan.

Konsekuensi untuk implementasi: - Buat representasi data yang sesuai,
misalnya `struct` atau `class`. - Untuk struktur berbasis linked node,
implementasikan node dan pointer sendiri. - Buat operasi yang dibutuhkan
mekanik, misalnya insert, delete, search, update, dan traversal. -
Pastikan operasi tersebut benar-benar dipanggil oleh fitur/mekanik game.

**Jangan menggunakan container siap pakai sebagai pengganti ADT yang
sedang dinilai.** Contohnya, jangan mengimplementasikan Stack wajib
hanya dengan `std::stack`, Queue wajib hanya dengan `std::queue`, atau
Linked List wajib hanya dengan `std::list`. Hal yang sama berlaku untuk
container/fungsi siap pakai yang menggantikan proses utama yang dinilai.

Instruksi tugas tidak menyatakan bahwa seluruh library C++ dilarang.
Batas larangannya adalah penggunaan library/container/fungsi siap pakai
**yang menggantikan implementasi ADT, operasi primitif, atau proses
utama yang sedang dinilai**. Library pendukung untuk hal lain tidak
otomatis dilarang, tetapi penggunaannya jangan sampai mengambil alih
implementasi yang harus dibuat sendiri.

### 2.4 Mekanik game wajib

-   Minimal satu mekanik dari sebuah game.
-   Mekanik dapat dibuat menyerupai mekanik asli atau disederhanakan.
-   Konsep utama mekanik sumber harus tetap dapat dikenali.
-   Boleh mereplikasi lebih dari satu mekanik.
-   Setiap mekanik harus memiliki peran yang jelas dan menggunakan
    struktur data sesuai kebutuhannya.
-   Mekanik yang dipilih harus dijelaskan pada Pendampingan Tubes 1,
    termasuk struktur data, data yang digunakan, dan logika program.

### 2.5 Keunikan pilihan mekanik

Kelompok **tidak diperbolehkan** mereplikasi mekanik yang sama dari game
yang sama dengan konsep implementasi yang sama atau sangat mirip dengan
kelompok lain.

Mekanik yang sama masih dapat digunakan apabila terdapat perbedaan
substansial pada konsep, pengelolaan data, atau cara kerjanya, dan telah
disetujui dosen pengampu.

Karena itu: - Jangan menganggap sebuah mekanik otomatis aman dipilih
hanya karena tercantum sebagai contoh. - Periksa potensi kemiripan
dengan proyek kelompok lain. - Jelaskan perbedaan konsep implementasi
saat mengusulkan mekanik. - Minta persetujuan dosen jika menggunakan
mekanik yang sama dengan pendekatan yang berbeda secara substansial.

### 2.6 Operasi manipulasi data

Operasi manipulasi data dilakukan secara wajar sesuai kebutuhan mekanik,
misalnya: - Insert / menambahkan data. - Delete / menghapus data. -
Search / mencari data. - Update / memperbarui data. - Traversal /
menelusuri data. - Operasi lain yang relevan.

Tidak semua operasi wajib ada pada setiap struktur data. Jika mekanik
memang tidak membutuhkan operasi tertentu, operasi tersebut tidak perlu
dipaksakan.

------------------------------------------------------------------------

## 3. Klarifikasi tambahan dosen: penggunaan GUI

### 3.1 Ketentuan dari dosen

Dosen memberikan klarifikasi bahwa mahasiswa **boleh menambahkan GUI**
sebagai visualisasi agar mekanik game yang direplikasi lebih menarik dan
mudah dipahami.

Ketentuan yang menyertainya:

1.  Implementasi utama struktur data dan seluruh logika mekanik game
    tetap menggunakan **C++**.
2.  GUI hanya berperan sebagai antarmuka atau visualisasi tambahan,
    bukan pengganti proses implementasi struktur data.
3.  Semua struktur data wajib (minimal satu linear dan satu non-linear)
    tetap diimplementasikan sendiri menggunakan C++, bukan library siap
    pakai yang menggantikan ADT.
4.  Fitur utama harus tetap dapat didemonstrasikan dan dijelaskan,
    termasuk proses manipulasi data dan cara kerja struktur data.

Dosen mempersilakan mahasiswa mengembangkan proyek sekreatif mungkin
selama fokus penilaian, yaitu penerapan struktur data dan logika mekanik
game, tetap terpenuhi.

### 3.2 Cara membaca ketentuan PDF dan klarifikasi GUI

Instruksi PDF awal menyebut program berbasis console dan menyatakan GUI,
animasi, visualisasi game, audio, serta game engine tidak diwajibkan.
Klarifikasi dosen kemudian secara eksplisit memperbolehkan penambahan
GUI.

Untuk proyek ini, gunakan pemahaman berikut:

-   **GUI diperbolehkan**, sebagai antarmuka atau visualisasi tambahan.
-   GUI tidak menghapus kewajiban implementasi C++.
-   GUI tidak boleh menjadi tempat utama implementasi ADT atau logika
    mekanik yang dinilai.
-   Jangan memindahkan logika inti ke bahasa atau framework GUI sehingga
    C++ hanya menjadi pelengkap.
-   Proses struktur data harus dapat diperlihatkan dan diterangkan saat
    demo.
-   Jika menggunakan GUI, pastikan hubungan antara aksi pengguna, logika
    C++, dan perubahan struktur data dapat diamati.

**Arah implementasi yang aman:** GUI menangani tampilan dan interaksi
pengguna, sedangkan C++ menjadi sumber utama implementasi ADT serta
logika mekanik game. Pastikan mekanisme integrasi yang dipilih tidak
membuat GUI menggantikan tanggung jawab C++.

------------------------------------------------------------------------

## 4. Batasan scope pengerjaan

Tugas ini berfokus pada replikasi mekanik game dan penerapan struktur
data, **bukan pengembangan game lengkap**.

### 4.1 Hal yang tidak diwajibkan

Berdasarkan instruksi PDF, hal-hal berikut tidak diwajibkan: - GUI
(namun kini diperbolehkan berdasarkan klarifikasi dosen). - Animasi. -
Visualisasi game. - Audio atau musik. - Penggunaan game engine. -
Implementasi aspek game yang tidak berkaitan dengan mekanik yang
direplikasi, misalnya otomatisasi NPC. - Database atau penyimpanan
permanen.

### 4.2 Penyimpanan data

-   Data cukup disimpan selama program berjalan.
-   Tidak wajib menggunakan database.
-   Penyimpanan permanen bukan fokus tugas.

### 4.3 Interaksi dan output

PDF awal menjelaskan interaksi melalui menu aplikasi console. Dengan
adanya klarifikasi GUI, interaksi dapat disajikan melalui GUI sebagai
antarmuka tambahan, tetapi mekanik dan proses C++ tetap harus berjalan.

Apa pun antarmukanya: - Pengguna harus dapat menjalankan fitur
mekanik. - Operasi seperti menambah, mencari, mengubah, menghapus,
memilih objek, atau menjalankan aksi harus dapat dilakukan. - Hasil
suatu aksi harus terlihat pada output. - Perubahan data harus dapat
diamati. Contoh pada instruksi: setelah damage gear di-upgrade, nilai
damage hasil upgrade harus ditampilkan.

### 4.4 Penyederhanaan

Mekanik boleh disederhanakan. Namun, konsep utama dari mekanik sumber
harus tetap dikenali dan dapat dijelaskan berdasarkan game sumber.

Jangan memperluas scope ke fitur game yang tidak membantu demonstrasi
mekanik atau struktur data wajib.

### 4.5 Subprogram tambahan

Setiap mekanik yang direplikasi wajib memiliki minimal **dua subprogram
tambahan** berupa function atau procedure, di luar operasi primitif ADT.

Subprogram tambahan tersebut harus merealisasikan fitur atau proses
utama dari mekanik. Operasi dasar seperti insert atau delete saja tidak
dihitung sebagai pengganti dua subprogram mekanik ini.

------------------------------------------------------------------------

## 5. Referensi mekanik game dari instruksi dosen

Daftar berikut merupakan contoh, bukan daftar tertutup. Mahasiswa boleh
mencari mekanik lain selama memenuhi ketentuan tugas.

  -----------------------------------------------------------------------
  Game                    Contoh mekanik          Gambaran mekanik /
                                                  implementasi
  ----------------------- ----------------------- -----------------------
  Assassin's Creed        Papyrus Puzzle &        Petunjuk pada papyrus
  Origins                 Treasure Hunt           mengarahkan pemain
                                                  menuju lokasi harta
                                                  tertentu. Papyrus dapat
                                                  menjadi data induk yang
                                                  menyimpan clue untuk
                                                  lokasi lain.

  DOTA 2                  Courier Delivery        Courier mengantarkan
                                                  item yang dibeli kepada
                                                  hero. Permintaan dan
                                                  proses pengiriman
                                                  dilakukan secara
                                                  berurutan.

  Dying Light / Dying     Day/Night Cycle &       Kondisi dunia dan
  Light 2                 Scheduled Events        tingkat ancaman berubah
                                                  mengikuti fase siang
                                                  dan malam. Fase waktu
                                                  diproses berurutan
                                                  ketika waktunya tiba.

  Pokemon FireRed /       Pokemon Party           Pemain membawa sejumlah
  LeafGreen               Management              Pokemon dan mengatur
                                                  susunan party. Data
                                                  party dapat ditambah,
                                                  dihapus, ditukar, dan
                                                  ditelusuri.

  Valorant                Map & Site Navigation   Map memiliki site,
                                                  spawn area, dan jalur
                                                  yang saling terhubung.
                                                  Area map
                                                  direpresentasikan
                                                  sebagai titik-titik
                                                  yang terhubung.

  PUBG: Battlegrounds     Loot & Inventory        Pemain mengambil,
                          Management              menyimpan, menggunakan,
                                                  atau membuang item.
                                                  Item dapat ditambah,
                                                  dihapus, dicari, dan
                                                  ditelusuri.

  Alan Wake               Safe Haven & Checkpoint Area bercahaya
                                                  berfungsi sebagai
                                                  tempat aman dan
                                                  checkpoint. Checkpoint
                                                  terakhir dapat menjadi
                                                  tempat spawn kembali
                                                  ketika karakter game
                                                  over.

  Two Point Hospital      Patient Queue           Pasien menunggu giliran
                                                  untuk memperoleh
                                                  layanan suatu room.
                                                  Pasien yang datang
                                                  lebih dahulu diperiksa
                                                  lebih dahulu.

  Mobile Legends: Bang    Lanes & Turrets         Lane, turret, jungle,
  Bang                                            dan base membentuk
                                                  titik penting map yang
                                                  saling terhubung.
                                                  Titik-titik yang dapat
                                                  dikunjungi dihubungkan.

  Pokemon UNITE           Held Item & Battle Item Pokemon membawa
                          Loadout                 kombinasi held item dan
                                                  battle item. Item dapat
                                                  ditambah, dihapus, atau
                                                  diganti.
  -----------------------------------------------------------------------

**Penting:** daftar ini hanya referensi ide. Ketentuan larangan
kemiripan implementasi dengan kelompok lain tetap berlaku.

------------------------------------------------------------------------

## 6. Contoh dari PDF yang tidak boleh direplikasi dengan konsep sama atau sangat mirip

PDF memberikan dua contoh implementasi untuk membantu memahami
penerjemahan mekanik menjadi data, operasi, dan struktur data. Kedua
contoh ini **tidak boleh direplikasi kembali dengan konsep yang sama
atau sangat mirip**.

### 6.1 Contoh 1 --- Gear Inventory

**Game sumber:** Assassin's Creed Origins\
**Struktur data pada contoh:** Singly Linked List

Mekanik: - Pemain memperoleh berbagai gear dan weapon. - Gear yang
dimiliki dapat dilihat melalui menu. - Pemain dapat menentukan gear yang
digunakan. - Gear dapat ditambahkan atau dihapus secara dinamis.

Contoh tipe data `Gear`: - `idGear`: integer - `namaGear`: string -
`jenisGear`: string - `damage`: integer - `requiredLevel`: integer -
`equipStatus`: boolean

Contoh fitur: - Menambah gear. - Melihat seluruh gear. -
Menghapus/menjual gear. - Memperbarui nilai gear, misalnya damage atau
required level. - Equip gear dengan mengubah `equipStatus`.

**Larangan:** jangan membuat replikasi Gear Inventory dengan konsep,
fungsi utama, pengelolaan data, dan penggunaan struktur data yang sama
atau sangat mirip.

### 6.2 Contoh 2 --- Skill Progression

**Game sumber:** Assassin's Creed Origins\
**Struktur data pada contoh:** Tree

Mekanik: - Pemain memiliki Ability Points untuk membuka kemampuan. -
Skill tertentu hanya dapat dibuka jika skill prasyarat sudah terbuka. -
Relasi parent-child pada Tree merepresentasikan jalur perkembangan dan
prerequisite.

Contoh tipe data `Skill`: - `idSkill`: integer - `namaSkill`: string -
`skillPoint`: integer - `unlocked`: boolean

Contoh fitur: - Melihat seluruh skill dan relasi prerequisite. - Mencari
skill berdasarkan ID atau nama. - Membuka skill jika prerequisite sudah
terpenuhi dan skill point mencukupi. - Menampilkan skill yang telah
terbuka. - Menampilkan skill yang dapat dibuka berikutnya.

**Larangan:** jangan membuat replikasi Skill Progression dengan konsep,
fungsi utama, pengelolaan data, dan penggunaan struktur data yang sama
atau sangat mirip.

------------------------------------------------------------------------

## 7. Pendampingan tugas besar

Pendampingan dilakukan sebanyak **dua kali**. Mahasiswa/kelompok tidak
diwajibkan mengumpulkan dokumen progres terpisah, tetapi wajib
menyampaikan dan menjelaskan progres secara lisan kepada asisten
praktikum.

### 7.1 Pendampingan Tubes 1 --- Perencanaan

Hal yang harus disampaikan: - Latar belakang pemilihan game dan
mekanik. - Analisis mekanik yang akan direplikasi. - Data yang akan
dikelola. - Perencanaan solusi. - Struktur data yang akan digunakan dan
justifikasi pemilihannya. - Tipe data bentukan. - Operasi yang
direncanakan. - Gambaran awal fitur dan alur program.

Pastikan dapat menjelaskan: 1. Apa mekanik yang direplikasi? 2.
Bagaimana mekanik tersebut bekerja pada game sumber? 3. Data apa yang
perlu disimpan? 4. Mengapa struktur data linear yang dipilih sesuai? 5.
Mengapa struktur data non-linear yang dipilih sesuai? 6. Bagaimana kedua
struktur data tersebut digunakan oleh mekanik? 7. Bagaimana alur program
dan fitur utamanya?

### 7.2 Pendampingan Tubes 2 --- Perkembangan implementasi

Hal yang harus disampaikan: - Mekanik dan fitur yang telah berhasil
diimplementasikan. - Struktur data dan subprogram yang telah dibuat. -
Kendala yang ditemukan dan solusi yang dilakukan. - Bagian yang masih
perlu diselesaikan.

Hasil diskusi pendampingan digunakan sebagai masukan untuk memperbaiki
dan menyempurnakan tugas sebelum pengumpulan laporan final.

------------------------------------------------------------------------

## 8. Laporan tugas besar final

Setiap mahasiswa/kelompok wajib mengumpulkan satu Laporan Tugas Besar
Final melalui LMS sesuai jadwal.

Isi laporan minimal mencakup:

1.  Latar belakang pemilihan game dan mekanik yang direplikasi.
2.  Analisis mekanik yang direplikasi, meliputi:
    -   Game sumber.
    -   Cara kerja mekanik pada game sumber.
    -   Data yang dikelola.
    -   Alasan memilih mekanik tersebut.
3.  Deskripsi proyek dan mekanik yang berhasil direplikasi.
4.  Seluruh fitur yang berhasil diimplementasikan.
5.  Seluruh struktur data yang digunakan, beserta penjelasan dan
    justifikasi pemilihannya.
6.  Tipe data bentukan, ADT, primitive, procedure, dan function yang
    digunakan.
7.  Penjelasan operasi yang dilakukan terhadap data dan struktur data.
8.  Tantangan atau kendala selama pengembangan beserta solusi yang
    dilakukan.
9.  Kesimpulan dan rekomendasi pengembangan.
10. Tautan repository GitHub.
11. Referensi yang digunakan selama pengerjaan.

### 8.1 Ketentuan penggunaan AI pada laporan

Instruksi penilaian laporan menyebutkan: - Laporan asli yang dibuat
mahasiswa/kelompok tanpa bantuan AI. - Jika AI digunakan untuk membantu
proses editing guna memperbaiki kejelasan/kualitas laporan, cantumkan
hasil laporan yang telah dibantu editing AI. - Cantumkan perbedaannya.

Perhatikan ketentuan ini secara khusus saat menyiapkan laporan. Jangan
menghilangkan atau menyamarkan penggunaan AI pada proses editing
laporan.

### 8.2 Ketentuan repository GitHub

Repository GitHub wajib bersifat **public**.

Minimal harus memiliki:

**README** - Deskripsi aplikasi/proyek. - Fitur-fitur. - Struktur data
yang digunakan. - Cara menjalankan program, termasuk instruksi instalasi
dan penggunaan. - Identitas mahasiswa/daftar anggota kelompok.

**Commit Log** - Menunjukkan perkembangan program secara bertahap. -
Riwayat commit sebaiknya mencerminkan tahapan pengerjaan yang nyata,
bukan hanya satu commit besar di akhir.

------------------------------------------------------------------------

## 9. Penilaian

  ------------------------------------------------------------------------
  Komponen                                     Bobot Aspek penilaian
  --------------------- ---------------------------- ---------------------
  Laporan                                    **50%** Kualitas analisis
                                                     mekanik game,
                                                     kedalaman justifikasi
                                                     struktur data,
                                                     penjelasan program
                                                     dan subprogram,
                                                     kelengkapan serta
                                                     kerapihan laporan,
                                                     dan ketentuan
                                                     pelaporan penggunaan
                                                     AI.

  Implementasi program                       **30%** Keberhasilan
                                                     replikasi mekanik dan
                                                     fitur utama,
                                                     manipulasi data,
                                                     kejelasan output,
                                                     efektivitas struktur
                                                     data, dokumentasi
                                                     GitHub (README dan
                                                     Commit Log).

  Presentasi dan demo                        **20%** Kemampuan
                                                     mendemonstrasikan dan
                                                     mempresentasikan
                                                     hasil serta
                                                     kesesuaian program
                                                     dengan laporan.
  ------------------------------------------------------------------------

**Implikasi:** kualitas visual game bukan pengganti keberhasilan
implementasi. GUI dapat meningkatkan keterbacaan demo, tetapi struktur
data, logika mekanik, dan penjelasan teknis tetap menjadi fokus utama.

------------------------------------------------------------------------

## 10. Timeline

  -----------------------------------------------------------------------
  Tahap                               Waktu
  ----------------------------------- -----------------------------------
  Pengumuman tugas besar dan          Pekan ke-9
  penentuan bentuk pengerjaan         

  Pendampingan Tubes 1 --- progres    Pekan ke-11
  awal dan perencanaan teknis         

  Pendampingan Tubes 2 --- progres    Pekan ke-13
  implementasi                        

  Pengumpulan Laporan Tugas Besar     Pekan ke-14
  Final melalui LMS                   

  Presentasi dan demo normal          Pekan ke-15

  Kesempatan presentasi lebih awal    Pekan ke-14 atau Flexible Week,
                                      **21--26 Desember 2026**, jika
                                      program dan laporan final sudah
                                      selesai
  -----------------------------------------------------------------------

Teknis pelaksanaan presentasi dan demo diserahkan kepada dosen pengampu
masing-masing.

------------------------------------------------------------------------

## 11. Panduan kepatuhan untuk AI Agent

Bagian ini merangkum konsekuensi praktis dari ketentuan tugas agar AI
Agent tidak menghasilkan solusi yang melanggar fokus penilaian.

### 11.1 Hal yang wajib dijaga

1.  **C++ adalah implementasi inti.** ADT dan seluruh logika mekanik
    game harus tetap berada pada implementasi C++.
2.  **Implementasikan ADT sendiri.** Jangan mengganti struktur data
    wajib dengan container siap pakai.
3.  **Gunakan minimal dua kategori struktur data.** Pastikan ada minimal
    satu linear dan satu non-linear.
4.  **Struktur data harus berfungsi nyata.** Hubungkan operasi ADT
    dengan fitur mekanik yang dapat dijalankan.
5.  **Setiap mekanik memiliki minimal dua subprogram tambahan.**
    Subprogram harus berada di luar operasi primitif ADT dan
    merealisasikan fitur/proses mekanik.
6.  **Buat mekanik yang dapat didemonstrasikan.** Input, aksi, perubahan
    data, dan hasilnya harus dapat diamati.
7.  **GUI boleh digunakan.** Tempatkan GUI sebagai antarmuka/visualisasi
    tambahan; jangan pindahkan tanggung jawab inti dari C++.
8.  **Jaga scope.** Implementasikan mekanik yang dipilih, bukan
    keseluruhan game sumber.
9.  **Jaga keunikan.** Hindari konsep implementasi yang sama atau sangat
    mirip dengan kelompok lain; konsultasikan kepada dosen jika
    diperlukan.
10. **Dokumentasikan proses.** Siapkan README, Commit Log, laporan, dan
    materi demo sesuai ketentuan.

### 11.2 Sebelum implementasi

AI Agent sebaiknya memastikan rancangan menjawab pertanyaan berikut:

-   Apa nama game sumber dan mekanik yang akan direplikasi?
-   Bagaimana mekanik bekerja pada game sumber?
-   Bagian mana yang dipertahankan dan disederhanakan?
-   Apa saja entitas/data yang perlu disimpan?
-   Apa relasi antar-data?
-   Struktur data linear apa yang digunakan dan mengapa?
-   Struktur data non-linear apa yang digunakan dan mengapa?
-   Operasi ADT apa yang diperlukan?
-   Apa minimal dua subprogram tambahan untuk setiap mekanik?
-   Bagaimana pengguna menjalankan dan mengamati mekanik?
-   Bagaimana struktur data dapat diperlihatkan saat demo?
-   Apakah konsep implementasi berpotensi sama atau sangat mirip dengan
    kelompok lain?

Jika keputusan penting belum jelas, tanyakan kepada pengguna. Jangan
mengarang persetujuan dosen atau menganggap pilihan mekanik sudah
disetujui.

### 11.3 Saat implementasi

-   Pisahkan dengan jelas kode ADT, logika mekanik, dan lapisan
    antarmuka.
-   Buat struktur data manual dengan operasi yang relevan.
-   Pastikan fitur memanggil operasi ADT yang benar-benar dibuat.
-   Berikan cara untuk mengamati perubahan data dan relasi node.
-   Uji kondisi normal dan kondisi batas yang relevan.
-   Hindari fitur tambahan yang tidak mendukung mekanik, demonstrasi,
    atau tujuan tugas.
-   Jangan menyatakan struktur data sudah diterapkan hanya karena
    tipe/class-nya dibuat; buktikan pemakaiannya dalam alur program.

### 11.4 Saat evaluasi dan demo

Pastikan kelompok mampu menunjukkan: - Mekanik yang direplikasi dan
kaitannya dengan game sumber. - Struktur data linear yang dibuat
manual. - Struktur data non-linear yang dibuat manual. - Operasi
manipulasi data yang benar-benar berjalan. - Minimal dua subprogram
tambahan untuk setiap mekanik. - Hubungan antara aksi pengguna, logika
C++, dan perubahan struktur data. - Output atau visualisasi yang
membuktikan mekanik bekerja. - Kesesuaian implementasi dengan isi
laporan.

------------------------------------------------------------------------

## 12. Hierarki acuan dan penanganan ketidakjelasan

Gunakan urutan acuan berikut:

1.  **Instruksi resmi PDF Tugas Besar Struktur Data Semester Ganjil
    2026/2027** sebagai dasar ketentuan tugas.
2.  **Klarifikasi tambahan dosen tentang GUI** sebagai penjelasan
    terbaru untuk penggunaan GUI.
3.  Keputusan atau persetujuan langsung dosen/asisten praktikum untuk
    hal yang memerlukan persetujuan, seperti penggunaan mekanik yang
    berpotensi mirip dengan kelompok lain.

Jika ada hal yang tidak dijelaskan secara eksplisit: - Jangan
mengubahnya menjadi kewajiban tanpa dasar. - Bedakan antara ketentuan
tertulis, interpretasi teknis, dan rekomendasi pengembangan. - Ajukan
pertanyaan kepada pengguna jika keputusan tersebut memengaruhi kepatuhan
tugas. - Untuk keputusan yang memerlukan persetujuan dosen, jangan
mengklaim sudah disetujui sebelum ada konfirmasi.

------------------------------------------------------------------------

## 13. Sumber dokumen

1.  **Instruksi Tugas Besar Struktur Data --- Semester Ganjil
    2026/2027**, dokumen PDF 8 halaman yang diberikan pengguna.
2.  **Klarifikasi tambahan dosen mengenai penggunaan GUI**, teks yang
    diberikan pengguna dalam percakapan.

> Dokumen Markdown ini merupakan penggabungan dan penataan ulang isi
> kedua sumber untuk memudahkan AI Agent membaca konteks penugasan. Jika
> terdapat keraguan terhadap redaksi atau keputusan resmi, rujuk kembali
> dokumen PDF dan klarifikasi dosen yang asli.
