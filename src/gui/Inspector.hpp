#pragma once
// ============================================================================
// Inspector (tombol Tab) — memvisualisasikan isi struktur data core secara
// langsung saat game berjalan:
//   Battle: node Timeline (Doubly Linked List) + pointer prev/next,
//           Stack snapshot rewind, Event Queue, prediksi giliran
//   Peta  : adjacency list Graph, radius BFS, jalur Dijkstra
//   Keduanya: log operasi struktur data terakhir
// Hanya MEMBACA state; tidak mengubah apa pun.
// ============================================================================

#include "core/battle/Battle.hpp"
#include "core/run/Run.hpp"
#include "DsLog.hpp"

namespace inspector {

void drawBattle(Battle& battle, const DsLog& log, float slide);
void drawMap(const Run& run, const DsLog& log, float slide);

}  // namespace inspector
