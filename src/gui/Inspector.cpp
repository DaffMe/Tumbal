#include "Inspector.hpp"

#include <string>
#include "Art.hpp"
#include "Gfx.hpp"

using namespace gfx;

namespace {

constexpr float kW = 470;

const Color kMono  {170, 210, 170, 255};
const Color kPtr   {120, 170, 230, 255};
const Color kTitle {230, 200, 130, 255};

Rectangle frame(float slide) {
    float x = VW - kW * slide;
    Rectangle r = {x, 0, kW, static_cast<float>(VH)};
    DrawRectangleRec(r, Color{8, 10, 12, 238});
    DrawLineEx({x, 0}, {x, static_cast<float>(VH)}, 2, kTitle);
    text(head(), "INSPECTOR STRUKTUR DATA", x + 18, 12, 18, kTitle);
    text(body(), "Tab untuk menutup", x + kW - 18, 15, 13, col::inkFaint, Right);
    return r;
}

float section(float x, float y, const std::string& title, const std::string& sub) {
    DrawLine(static_cast<int>(x), static_cast<int>(y), static_cast<int>(x + kW - 36), static_cast<int>(y), Color{60, 60, 50, 255});
    text(head(), title, x, y + 6, 16, kTitle);
    text(body(), sub, x + kW - 36, y + 8, 13, col::inkFaint, Right);
    return y + 30;
}

float drawLog(const DsLog& log, float x, float y, float bottom) {
    y = section(x, y, "LOG OPERASI", "total " + std::to_string(log.total));
    int maxLines = static_cast<int>((bottom - y) / 15);
    int start = static_cast<int>(log.lines.size()) - maxLines;
    if (start < 0) start = 0;
    for (size_t i = start; i < log.lines.size(); ++i) {
        float a = 0.45f + 0.55f * (i - start + 1) / static_cast<float>(log.lines.size() - start);
        std::string s = log.lines[i];
        if (s.size() > 62) s = s.substr(0, 61) + "~";
        text(body(), s, x, y, 13, fade(kMono, a));
        y += 15;
    }
    return y;
}

}  // namespace

namespace inspector {

void drawBattle(Battle& b, const DsLog& log, float slide) {
    Rectangle r = frame(slide);
    float x = r.x + 18;
    float y = 44;

    // ---------------------------------------------------- Timeline (DLL)
    y = section(x, y, "TIMELINE  ·  Doubly Linked List", std::to_string(b.timeline().size()) + " node");
    text(body(), "HEAD", x, y, 13, kPtr);
    y += 15;
    int i = 0;
    b.timeline().forEach([&](const Timeline<Unit>::Node& n) {
        if (i >= 9) return;
        Rectangle box = {x + 30, y, 250, 22};
        Color sc = n.item->side == Side::Hero ? col::hero : col::enemy;
        DrawRectangleRec(box, Color{18, 22, 26, 255});
        DrawRectangleLinesEx(box, 1, fade(sc, 0.8f));
        text(body(), n.item->name, box.x + 8, box.y + 3, 14, sc);
        text(body(), "av " + std::to_string(static_cast<int>(n.av * 10) / 10) + "." + std::to_string(static_cast<int>(n.av * 10) % 10),
             box.x + box.width - 8, box.y + 3, 14, kMono, Right);
        // pointer prev/next
        std::string pv = n.prev ? art::shortName(n.prev->item->name) : "null";
        std::string nx = n.next ? art::shortName(n.next->item->name) : "null";
        text(body(), "prev:" + pv, box.x + box.width + 10, box.y - 1, 11, fade(kPtr, 0.8f));
        text(body(), "next:" + nx, box.x + box.width + 10, box.y + 10, 11, fade(kPtr, 0.8f));
        if (n.next) {
            DrawLineEx({x + 18, y + 22}, {x + 18, y + 30}, 1.5f, kPtr);
            DrawTriangle({x + 14, y + 26}, {x + 18, y + 31}, {x + 22, y + 26}, kPtr);
        }
        y += 30;
        ++i;
    });
    text(body(), "TAIL", x, y - 6, 13, kPtr);
    y += 14;

    // prediksi
    int order[8];
    int n = b.predictTurnOrder(8, order);
    std::string pred;
    for (int k = 0; k < n; ++k) {
        if (k) pred += " > ";
        pred += art::shortName(b.unit(order[k]).name);
    }
    text(body(), "predictTurnOrder(8) [salinan DLL]:", x, y, 13, col::inkDim);
    y += 15;
    textWrap(body(), pred, x, y, kW - 36, 13, kMono);
    y += 34;

    // ---------------------------------------------------- Stack
    y = section(x, y, "REWIND  ·  Stack<Snapshot*>", std::to_string(b.historySize()) + " snapshot");
    int turns[10];
    int ns = b.snapshotTurns(turns, 10);
    for (int k = 0; k < ns; ++k) {
        Rectangle box = {x + k * 42.0f, y, 38, 26};
        DrawRectangleRec(box, k == 0 ? Color{50, 40, 20, 255} : Color{18, 22, 26, 255});
        DrawRectangleLinesEx(box, 1, k == 0 ? col::gold : col::inkFaint);
        text(body(), "g" + std::to_string(turns[k]), box.x + 19, box.y + 5, 13, kMono, Center);
    }
    text(body(), "<- top", x + ns * 42.0f + 4, y + 6, 12, col::inkFaint);
    y += 40;

    // ---------------------------------------------------- Queue
    y = section(x, y, "EVENT  ·  Queue<BattleEvent>", "menunggu: " + std::to_string(b.events().size()));
    y += 2;
    drawLog(log, x, y, VH - 14);
}

void drawMap(const Run& run, const DsLog& log, float slide) {
    Rectangle r = frame(slide);
    float x = r.x + 18;
    float y = 44;
    const Floor& f = run.floor();

    y = section(x, y, "DUNGEON  ·  Graph (adjacency multi linked list)",
                std::to_string(f.graph().vertexCount()) + "V / " + std::to_string(f.graph().edgeCount()) + "E");
    text(body(), "Hanya ruang terungkap. w = bobot (biaya cahaya / " + std::to_string(LIGHT_PER_WEIGHT) + ")", x, y, 13,
         col::inkFaint);
    y += 18;
    int shown = 0;
    f.graph().forEachVertex([&](Floor::Vertex* v) {
        if (!v->data.revealed || y > 430) return;
        bool here = v->id == f.currentRoom();
        std::string line = (here ? "@" : " ") + std::string("[") + (v->id < 10 ? " " : "") + std::to_string(v->id) + "] " +
                           std::string(1, roomSymbol(v->data.type)) + " :";
        for (auto* e = v->edges; e != nullptr; e = e->next)
            line += " ->" + std::to_string(e->to->id) + "(w" + std::to_string(e->weight) + ")";
        if (line.size() > 60) line = line.substr(0, 59) + "~";
        text(body(), line, x, y, 13, here ? col::emberHot : kMono);
        y += 15;
        ++shown;
    });
    y += 6;
    y = section(x, y, "CAHAYA  ·  BFS reveal", "");
    text(body(), "cahaya " + std::to_string(run.light()) + " -> radius BFS " + std::to_string(revealRadius(run.lightLevel())),
         x, y, 14, kMono);
    y += 18;
    text(body(), "checkpoint (shrine): ruang " + std::to_string(run.checkpoint()), x, y, 14, kMono);
    y += 18;
    if (run.droppedAsh() > 0)
        text(body(), "Ash tertinggal: " + std::to_string(run.droppedAsh()) + " di ruang " + std::to_string(run.droppedAshRoom()),
             x, y, 14, col::blood);
    y += 24;
    drawLog(log, x, y, VH - 14);
    (void)shown;
}

}  // namespace inspector
