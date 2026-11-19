# Kredit Aset — TUMBAL

Kode game ditulis oleh tim. Aset visual/audio pihak ketiga di bawah ini dipakai sesuai lisensinya.
Lisensi diperiksa di halaman aslinya pada 2026-11-19.

## Grafis

| Aset | Pembuat | Sumber | Lisensi | Dipakai di |
|---|---|---|---|---|
| Evil Dungeon Asset Pack | Sevarihk (Aurora-Sprites) | https://opengameart.org/content/evil-dungeon-asset-pack | CC-BY 4.0 | tileset lumbung, autotile tembok |
| Dead Swamp Tileset | Sevarihk | https://opengameart.org/content/dead-swamp-tileset | CC-BY 4.0 | tileset rawa |
| Bamboo Tiles | Sevarihk | https://opengameart.org/content/bamboo-tiles | CC-BY 4.0 | hutan bambu |
| Batak House | Sevarihk | https://opengameart.org/content/batak-house-pixel-tileset | CC-BY 4.0 | rumah desa (sementara) |
| Ground and Foliage Autotiles | Sevarihk | https://opengameart.org/content/ground-and-foliage-autotiles | CC-BY 4.0 | rumput, semak |
| Animated Water Tiles | Sevarihk | https://opengameart.org/content/animated-water-tiles-0 | CC-BY 4.0 | air |
| Fisher's Hut / Tree Trunk / Palm | Sevarihk | https://opengameart.org/users/sevarihk | CC-BY 4.0 | rumah panggung desa (dipotong, digelapkan), properti peta |
| NPC Faceset | Sevarihk | https://opengameart.org/content/npc-faceset | CC-BY 4.0 | potret NPC |
| Tribal Hide and Wood UI | Sevarihk | https://opengameart.org/content/tribal-hide-and-wood-ui | CC-BY 4.0 | bingkai UI |
| Aurora Character Generator (potongan sprite) | Sevarihk; generator oleh RedoxGames | https://redoxgames.itch.io/aurora-character-generator | CC-BY 4.0 | sprite karakter (dirakit & diwarnai ulang oleh `tools/build_assets.py`) |
| 10 Basic RPG Enemies, 6 More RPG Enemies, More RPG Enemies, Fossil Undead | Stephen "Redshrike" Challener (beberapa sprite hasil edit dari karya Blarumyrran & LordNeo) | https://opengameart.org/users/redshrike | CC-BY 3.0 | battler musuh (dipotong & diperbesar) |
| Backgrounds (battleback) | Nidhoggn | https://opengameart.org/content/backgrounds-3 | CC0 | battleback (digelapkan & diberi vinyet) |

## Aset buatan AI untuk proyek ini

Dibuat dengan Codex (model gpt-6.2-astra, alat pembuat gambar OpenAI) khusus untuk TUMBAL karena tidak ada
padanan gratisnya; prompt tersimpan di `assets/raw/codex/PROMPT_*.md`. Gambar mentah di `assets/raw/codex/`,
diproses (latar dibuang, grid piksel dikembalikan, diskalakan) oleh `tools/build_assets.py`.

| Aset | Dipakai di |
|---|---|
| Memedi Sawah (battler + sprite peta) | boss Ladang Timun Busuk |
| Timun Busuk (battler) | musuh ladang |
| Props ladang: ajir timun, ajir roboh, gubuk sawah, tumpukan timun, ember, cangkul, pagar bambu | peta ladang |

## Font

| Font | Pembuat | Sumber | Lisensi |
|---|---|---|---|
| Jersey 10 | The Soft Type Project Authors | https://github.com/google/fonts/tree/main/ofl/jersey10 | SIL OFL 1.1 (`assets/fonts/OFL_Jersey10.txt`) |

Perubahan: autotile RPG Maker XP dibakar ulang menjadi lembar 47 tile (`tools/autotile.py`); sprite karakter
dirakit dari beberapa lapisan dan diwarnai ulang.

## Audio

Musik (semua CC0, lisensi dicek di halaman OpenGameArt masing-masing):

| Berkas | Judul asli | Pembuat | Sumber | Lisensi |
|---|---|---|---|---|
| `music/judul.ogg` | Dark Intro | Nikke | https://opengameart.org/content/dark-intro | CC0 |
| `music/lumbung.ogg` | Dungeon Ambience | yd | https://opengameart.org/content/dungeon-ambience | CC0 |
| `music/desa.ogg` | Lost in a Bad Place (horror ambience loop) | congusbongus | https://opengameart.org/content/lost-in-a-bad-place-horror-ambience-loop | CC0 |
| `music/ladang.mp3` | Dark Forest Theme | cynicmusic | https://opengameart.org/content/dark-forest-theme | CC0 |
| `music/laut.ogg` | Whispers in the Fog | Ruhinre | https://opengameart.org/content/whispers-in-the-fog | CC0 |
| `music/rawa.ogg` | A Lurking Evil (horror ambience) | congusbongus | https://opengameart.org/content/a-lurking-evil-horror-ambience | CC0 |
| `music/battle.mp3` | Battle Theme B (for RPG) | cynicmusic | https://opengameart.org/content/battle-theme-b-for-rpg | CC0 |
| `music/boss.ogg` | Battle RPG Theme (Var) | CleytonKauffman | https://opengameart.org/content/boss-battle-theme | CC0 |

Efek suara (`assets/audio/sfx/`, CC0, lisensi di `LICENSE_Kenney.txt`): Kenney — Interface Sounds
(https://kenney.nl/assets/interface-sounds), Impact Sounds (https://kenney.nl/assets/impact-sounds),
RPG Audio (https://kenney.nl/assets/rpg-audio). Berkas diganti nama sesuai fungsinya (kursor, pilih, tebas, ...).
Hutan Bambu memakai ulang musik lumbung; Sarang Buto memakai ulang musik rawa.
