#include "World.hpp"

#include "../util/DsTrace.hpp"
#include "../util/Json.hpp"

World::~World() {
    graph_.forEachVertex([](MapGraph::Vertex* v) {
        delete v->data.map;
        v->data.map = nullptr;
    });
}

MapInfo* World::info(int mapId) {
    MapGraph::Vertex* v = graph_.findVertex(mapId);
    return v ? &v->data : nullptr;
}

const MapInfo* World::info(int mapId) const {
    MapGraph::Vertex* v = graph_.findVertex(mapId);
    return v ? &v->data : nullptr;
}

bool World::load(const std::string& worldJsonPath, std::string* error) {
    JsonValue* root = JsonValue::parseFile(worldJsonPath, error);
    if (root == nullptr) return false;

    std::string dir;
    std::size_t k = worldJsonPath.find_last_of("/\\");
    if (k != std::string::npos) dir = worldJsonPath.substr(0, k + 1);

    if (const JsonValue* s = root->get("start")) {
        startMap_ = s->getInt("map");
        startSpawn_ = s->getString("spawn", "awal");
    }

    // 1) vertex: setiap peta
    bool ok = true;
    if (const JsonValue* maps = root->get("maps")) {
        for (const JsonValue* m = maps->first(); m != nullptr; m = m->next()) {
            MapInfo mi;
            mi.name = m->getString("name");
            mi.file = m->getString("file");
            mi.map = new TileMap();
            if (!mi.map->load(dir + mi.file, error)) {
                delete mi.map;
                ok = false;
                break;
            }
            mi.hasTungku = false;
            for (const MapObject* o = mi.map->objects(); o != nullptr; o = o->next)
                if (o->type == "tungku") mi.hasTungku = true;
            int id = m->getInt("id");
            if (graph_.addVertex(id, mi) == nullptr) {
                delete mi.map;
                if (error) *error = "id peta ganda: " + std::to_string(id);
                ok = false;
                break;
            }
            dsOp("Graph.addVertex(" + std::to_string(id) + ", \"" + mi.name + "\")");
        }
    }
    delete root;
    if (!ok) return false;

    // 2) edge: dari objek pintu di setiap peta
    graph_.forEachVertex([this](MapGraph::Vertex* v) {
        for (const MapObject* o = v->data.map->objects(); o != nullptr; o = o->next) {
            if (o->type != "door") continue;
            int to = o->props.getInt("to");
            if (graph_.findVertex(to) == nullptr) continue;
            bool locked = o->props.getBool("locked", false);
            bool oneway = o->props.getBool("oneway", false);
            if (graph_.hasEdge(v->id, to)) {
                // pintu kedua untuk pasangan yang sama: kunci ikut pintu terkunci
                if (locked) graph_.setOpen(v->id, to, false);
                continue;
            }
            if (oneway) graph_.addArc(v->id, to, 1, !locked);
            else        graph_.addEdge(v->id, to, 1, !locked);
            dsOp(std::string(oneway ? "Graph.addArc(" : "Graph.addEdge(") + std::to_string(v->id) + ", " +
                 std::to_string(to) + (locked ? ", terkunci)" : ")"));
        }
    });
    return true;
}

TravelResult World::travel(int fromMap, const MapObject& door) {
    TravelResult r;
    int to = door.props.getInt("to");
    dsOp("Graph.isOpen(" + std::to_string(fromMap) + " -> " + std::to_string(to) + ")");
    if (!graph_.hasEdge(fromMap, to)) return r;   // tidak ada jalan a->b
    if (!graph_.isOpen(fromMap, to)) {
        r.locked = true;
        return r;
    }
    MapInfo* dest = info(to);
    if (dest == nullptr) return r;

    // titik tiba: objek spawn bernama `spawn` di peta tujuan, atau tx/ty
    std::string spawn = door.props.getString("spawn");
    const MapObject* sp = spawn.empty() ? nullptr : dest->map->findObject(spawn);
    if (sp != nullptr) {
        r.x = sp->tileX(dest->map->tileW());
        r.y = sp->tileY(dest->map->tileH());
        r.dir = sp->props.getInt("dir", door.props.getInt("dir", 0));
    } else {
        r.x = door.props.getInt("tx");
        r.y = door.props.getInt("ty");
        r.dir = door.props.getInt("dir", 0);
    }
    r.toMap = to;
    r.ok = true;
    markVisited(to);
    return r;
}

bool World::unlockShortcut(int a, int b) {
    bool ok = graph_.setOpen(a, b, true);
    dsOp("Graph.setOpen(" + std::to_string(a) + ", " + std::to_string(b) + ", true) -> " + (ok ? "jalan pintas terbuka" : "gagal"));
    return ok;
}

int World::nearestTungku(int fromMap) {
    int best = 0;
    int bestDepth = -1;
    graph_.bfs(fromMap, -1, [&](MapGraph::Vertex* v, int depth) {
        if (v->data.hasTungku && v->data.visited && (bestDepth < 0 || depth < bestDepth)) {
            best = v->id;
            bestDepth = depth;
        }
    });
    dsOp("Graph.bfs(" + std::to_string(fromMap) + ") -> Tungku terdekat di peta " + std::to_string(best));
    return best;
}

void World::markVisited(int mapId) {
    if (MapInfo* i = info(mapId)) i->visited = true;
}
