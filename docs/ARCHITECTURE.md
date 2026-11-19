# Arsitektur Cinderwake

Peta kode untuk tim (laporan, pendampingan, demo) dan untuk AI agent. Jika kode berubah, perbarui dokumen ini.

## 1. Lapisan

```
┌──────────────────────────── src/gui (raylib) ────────────────────────────┐   ┌── src/console ──┐
│ App ─ MapView ─ BattleView ─ Inspector ─ Art/Gfx/Sfx                     │   │ ConsoleGame     │
│ (tampilan + input saja, membaca state & memanggil API core)              │   │ (menu teks)     │
└──────────────┬────────────────────────────────────▲──────────────────────┘   └──┬───────▲──────┘
     aksi pemain│  travelTo / playerUseSkill / ...   │ dequeue BattleEvent           │       │
               ▼                                     │ (Queue)                       ▼       │
┌──────────────────────────── src/core (C++ murni, tanpa raylib) ──────────────────────────────────┐
│ run/Run ──────── dungeon/Floor ──────── battle/Battle ── Unit / Skill / Status / Encounters     │
│   │                  │                     │                                                     │
│   │                  ▼                     ▼                                                     │
│   │            adt/Graph<Room>     adt/Timeline<Unit>  adt/Queue<BattleEvent>  adt/Stack<Snap*> │
└───┴───────────────────────────────────────────────────────────────────────────────────────────────┘
```

GUI dan console memanggil **API core yang sama**. Itu sebabnya `--console` bisa memainkan game penuh tanpa GUI.

## 2. Peta file

| File | Tanggung jawab |
|---|---|
| `src/core/adt/Timeline.hpp` | **Doubly Linked List** terurut berdasarkan Action Value (M1) |
| `src/core/adt/Queue.hpp` | **Queue** linked (Event Queue battle, BFS) |
| `src/core/adt/Stack.hpp` | **Stack** linked (snapshot rewind M3, rekonstruksi jalur Dijkstra) |
| `src/core/adt/Graph.hpp` | **Graph** tak berarah berbobot, adjacency multi linked list (M2) |
| `src/core/battle/Battle.*` | Mesin battle: alur giliran, eksekusi skill, AI musuh, rewind |
| `src/core/battle/Unit.*` | Data petarung + roster hero/musuh/boss |
| `src/core/battle/Skill.*` | Tabel skill (efek, target, biaya Ember) |
| `src/core/battle/Status.hpp` | Status effect (Burn, DEF+, SPD+, Taunt, Vulnerable) |
| `src/core/battle/BattleEvent.hpp` | Jenis event yang di-enqueue ke Event Queue |
| `src/core/battle/Encounters.*` | Komposisi musuh per encounter + party awal |
| `src/core/battle/Kinds.hpp` | Enum jenis hero/musuh (juga dipakai GUI memilih sprite) |
| `src/core/dungeon/Floor.*` | Lantai dungeon: generator, cahaya, BFS reveal, Dijkstra, koridor runtuh |
| `src/core/run/Run.*` | Satu ekspedisi: perjalanan, battle, shrine/checkpoint, toko, event, relic, kematian |
| `src/core/util/Rng.hpp` | RNG xorshift deterministik (ikut disimpan di snapshot) |
| `src/core/util/Format.hpp` | Helper format string |
| `src/console/ConsoleGame.*` | Mode console: battle cepat & ekspedisi, trace operasi struktur data |
| `src/console/Balance.*` | Simulator balancing `--balance` (pemain heuristik, laporan win rate) |
| `src/gui/App.*` | Loop utama & perpindahan scene (Title, Map, Battle, Death, Won) |
| `src/gui/MapView.*` | Gambar Graph lantai, input perjalanan, modal shrine/toko/event/relic |
| `src/gui/BattleView.*` | Gambar battle, memutar Event Queue jadi animasi, input skill/target |
| `src/gui/Inspector.*` | Panel `Tab`: isi node Timeline, Stack, Queue, adjacency list Graph |
| `src/gui/Gfx.*` | Resolusi virtual, font, widget, partikel, shader post-process |
| `src/gui/Art.*` | Gambar aset final atau placeholder prosedural |
| `src/gui/Sfx.*` | Efek suara & ambience prosedural |
| `src/gui/Gallery.*` | Mode `--gallery` untuk memeriksa aset |
| `tests/*` | Unit test ADT, battle, dungeon/run |
| `tools/process_assets.py` | Post-process aset Codex `assets/raw` → `assets/final` |

## 3. Struktur data → operasi → pemakai

### Timeline (Doubly Linked List) — M1 Action Timeline

| Operasi | Dipanggil oleh | Kegunaan di mekanik |
|---|---|---|
| `insertSorted` | `Battle::start`, `Battle::endTurn`, `Battle::spawnUnit`, `predictTurnOrder` | Unit masuk antrean sesuai AV |
| `insertFront` | `Battle::interject`, `Battle::endTurn` (extra turn) | Interject / bertindak lagi segera |
| `remove` | `Battle::interject`, `Battle::handleUnitDeath` | Pindah ke head / unit tumbang |
| `popFront` | `Battle::advanceTimeline` | Ambil unit yang bertindak |
| `search` | `shiftActionValue`, `rescaleAV` | Cari node target delay/advance |
| `updateAV` (relink prev/next) | `shiftActionValue`, `rescaleAV` | Delay/advance, perubahan SPD |
| `subtractAll` (traversal) | `advanceTimeline`, `predictTurnOrder` | Waktu berjalan |
| copy constructor | `saveTurnSnapshot`, `restore`, `predictTurnOrder` | Snapshot & simulasi |
| `forEach` | BattleView, Inspector, console | Menampilkan urutan giliran |

### Graph (adjacency multi linked list) — M2 Dungeon Exploration

| Operasi | Dipanggil oleh | Kegunaan di mekanik |
|---|---|---|
| `addVertex`, `addEdge` | `Floor::generateFloor` | Membangun ruangan & koridor |
| `removeEdge` + `isConnected` (+ `addEdge` pemulihan) | `Floor::collapseCorridor` | Event koridor runtuh |
| `bfs` (pakai Queue) | `Floor::revealByLight`, `isConnected` | Fog of war sesuai cahaya |
| `dijkstra` (filter ruang terungkap, jalur di Stack) | `Floor::findPath`, `Floor::findNearestShrine` | Auto-travel, jalur ke shrine |
| `hasEdge`, `edgeWeight` | `Floor::moveTo`, `Floor::travelCost` | Validasi gerak, biaya cahaya |
| `forEachNeighbor`, `forEachVertex`, `findVertex` | Floor, MapView, Inspector, console | Traversal & tampilan |
| `removeVertex` | (unit test) | Tersedia di ADT; belum dibutuhkan mekanik |

### Queue — Battle Event Queue

| Operasi | Dipanggil oleh | Kegunaan |
|---|---|---|
| `enqueue` | `Battle::emit`, `Run::info`, `Floor::dsOp`, `Graph::bfs` | Catat kejadian / antre BFS |
| `dequeue` | `Battle::pollEvent` → BattleView/console, `Run::pollEvent`, `Graph::bfs` | Putar animasi berurutan (FIFO) |

### Stack — M3 Cracked Hourglass

| Operasi | Dipanggil oleh | Kegunaan |
|---|---|---|
| `push` | `Battle::saveTurnSnapshot`, `Graph::dijkstra` | Simpan state tiap giliran hero / jalur |
| `pop`, `peek` | `Battle::rewindTurn`, `Run::autoTravel` | Kembali ke giliran sebelumnya / ikuti jalur |
| `forEach` | `Battle::snapshotTurns` → Inspector | Tampilkan isi stack |

## 4. Subprogram mekanik (di luar operasi primitif ADT)

| Mekanik | Subprogram | Lokasi |
|---|---|---|
| M1 Action Timeline | `advanceTimeline`, `shiftActionValue`, `predictTurnOrder`, `handleUnitDeath`, `interject`, `spawnUnit` | `src/core/battle/Battle.cpp` |
| M2 Dungeon Exploration | `generateFloor`, `revealByLight`, `findPath`, `findNearestShrine`, `collapseCorridor` | `src/core/dungeon/Floor.cpp` |
| M2 (run) | `travelTo`, `autoTravel`, `travelToNearestShrine`, `handlePartyWipe` | `src/core/run/Run.cpp` |
| M3 Rewind | `saveTurnSnapshot`, `rewindTurn` | `src/core/battle/Battle.cpp` |

Hubungan M2 → M1: di `Run::startBattle`, cahaya gelap memberi peluang ambush, lalu `Battle::start` memanggil
`shiftActionValue(musuh, -30%)`.

## 5. Alur satu giliran battle

```
Battle::advanceUntilPlayer()
 └─ beginNextTurn()
     ├─ advanceTimeline()  : subtractAll(av head) → popFront()        [DLL]
     ├─ tickStartOfTurn()  : Burn
     ├─ musuh → runEnemyTurn() → executeSkill() → endTurn()
     └─ hero  → saveTurnSnapshot()                                    [Stack.push]
                tunggu input ─► playerUseSkill(slot, target)
                                 ├─ executeSkill() → applyEffect()
                                 │     Damage/Break/Delay → shiftActionValue → updateAV  [DLL relink]
                                 └─ endTurn() → insertSorted(aktor, AV dasar)             [DLL]
Setiap langkah → emit(BattleEvent) / dsOp(...)                                            [Queue.enqueue]
GUI: BattleView::update → pollEvent() satu per satu → animasi                            [Queue.dequeue]
```

## 6. Pengujian

| File | Cakupan |
|---|---|
| `tests/test_adt.cpp` | Semua operasi ADT termasuk kasus batas (tie AV, list kosong, graph terputus) |
| `tests/test_battle.cpp` | Urutan giliran vs prediksi, delay/advance, kematian, rewind, interject, break, summon, ambush |
| `tests/test_dungeon.cpp` | Generator (200 seed), boss per lantai, BFS/Dijkstra, koridor runtuh, kematian & Ash, run penuh |
