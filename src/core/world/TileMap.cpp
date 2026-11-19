#include "TileMap.hpp"

#include <cstdlib>
#include "../util/Json.hpp"

// -------------------------------------------------------------- PropertyList

PropertyList::~PropertyList() {
    while (head != nullptr) {
        MapProperty* n = head->next;
        delete head;
        head = n;
    }
}

void PropertyList::add(const std::string& name, const std::string& value) {
    MapProperty* p = new MapProperty{name, value, nullptr};
    if (head == nullptr) { head = p; return; }
    MapProperty* last = head;
    while (last->next != nullptr) last = last->next;
    last->next = p;
}

const MapProperty* PropertyList::find(const std::string& name) const {
    for (MapProperty* p = head; p != nullptr; p = p->next)
        if (p->name == name) return p;
    return nullptr;
}

std::string PropertyList::getString(const std::string& name, const std::string& def) const {
    const MapProperty* p = find(name);
    return p ? p->value : def;
}
int PropertyList::getInt(const std::string& name, int def) const {
    const MapProperty* p = find(name);
    return p ? std::atoi(p->value.c_str()) : def;
}
bool PropertyList::getBool(const std::string& name, bool def) const {
    const MapProperty* p = find(name);
    if (p == nullptr) return def;
    return p->value == "true" || p->value == "1";
}

// --------------------------------------------------------------- Tileset

TileAnim::~TileAnim() {
    while (frames != nullptr) {
        TileAnimFrame* n = frames->next;
        delete frames;
        frames = n;
    }
}

Tileset::~Tileset() {
    while (anims != nullptr) {
        TileAnim* n = anims->next;
        delete anims;
        anims = n;
    }
}

int Tileset::animatedTile(int localId, long long ms) const {
    for (TileAnim* a = anims; a != nullptr; a = a->next) {
        if (a->tileId != localId || a->totalMs <= 0) continue;
        long long t = ms % a->totalMs;
        for (TileAnimFrame* f = a->frames; f != nullptr; f = f->next) {
            if (t < f->durationMs) return f->tileId;
            t -= f->durationMs;
        }
        return a->frames ? a->frames->tileId : localId;
    }
    return localId;
}

// --------------------------------------------------------------- helpers

static std::string jsonToText(const JsonValue* v) {
    if (v == nullptr) return "";
    switch (v->type()) {
        case JsonValue::String: return v->asString();
        case JsonValue::Bool:   return v->asBool() ? "true" : "false";
        case JsonValue::Number: {
            double d = v->asNumber();
            long long i = static_cast<long long>(d);
            if (static_cast<double>(i) == d) return std::to_string(i);
            return std::to_string(d);
        }
        default: return "";
    }
}

static void readProps(const JsonValue* arr, PropertyList& out) {
    if (arr == nullptr || !arr->isArray()) return;
    for (const JsonValue* p = arr->first(); p != nullptr; p = p->next())
        out.add(p->getString("name"), jsonToText(p->get("value")));
}

static std::string dirOf(const std::string& path) {
    std::size_t k = path.find_last_of("/\\");
    return k == std::string::npos ? std::string() : path.substr(0, k + 1);
}

// Gabung "assets/maps/" + "../tiles/x.png" → "assets/tiles/x.png".
static std::string joinPath(const std::string& dir, const std::string& rel) {
    if (rel.empty() || rel[0] == '/' || (rel.size() > 1 && rel[1] == ':')) return rel;
    std::string base = dir;
    std::string r = rel;
    while (r.compare(0, 3, "../") == 0) {
        r = r.substr(3);
        if (!base.empty() && (base.back() == '/' || base.back() == '\\')) base.pop_back();
        std::size_t k = base.find_last_of("/\\");
        base = (k == std::string::npos) ? std::string() : base.substr(0, k + 1);
    }
    return base + r;
}

// --------------------------------------------------------------- TileMap

TileMap::~TileMap() { clear(); }

void TileMap::clear() {
    while (layers_ != nullptr) { TileLayer* n = layers_->next; delete layers_; layers_ = n; }
    while (tilesets_ != nullptr) { Tileset* n = tilesets_->next; delete tilesets_; tilesets_ = n; }
    while (objects_ != nullptr) { MapObject* n = objects_->next; delete objects_; objects_ = n; }
    collision_ = nullptr;   // milik layer "collision", sudah dihapus
}

bool TileMap::load(const std::string& path, std::string* error) {
    clear();
    path_ = path;
    JsonValue* root = JsonValue::parseFile(path, error);
    if (root == nullptr) return false;

    width_ = root->getInt("width");
    height_ = root->getInt("height");
    tileW_ = root->getInt("tilewidth", 32);
    tileH_ = root->getInt("tileheight", 32);
    readProps(root->get("properties"), props_);
    const std::string dir = dirOf(path);

    // ---- tileset (harus embedded di peta)
    Tileset* lastTs = nullptr;
    if (const JsonValue* tss = root->get("tilesets")) {
        for (const JsonValue* t = tss->first(); t != nullptr; t = t->next()) {
            Tileset* ts = new Tileset();
            ts->firstGid = t->getInt("firstgid", 1);
            ts->name = t->getString("name");
            ts->image = joinPath(dir, t->getString("image"));
            ts->tileW = t->getInt("tilewidth", tileW_);
            ts->tileH = t->getInt("tileheight", tileH_);
            ts->columns = t->getInt("columns", 1);
            ts->tileCount = t->getInt("tilecount", 0);
            ts->imageW = t->getInt("imagewidth");
            ts->imageH = t->getInt("imageheight");
            if (const JsonValue* tiles = t->get("tiles")) {
                for (const JsonValue* tl = tiles->first(); tl != nullptr; tl = tl->next()) {
                    const JsonValue* anim = tl->get("animation");
                    if (anim == nullptr || anim->size() == 0) continue;
                    TileAnim* a = new TileAnim();
                    a->tileId = tl->getInt("id");
                    TileAnimFrame* lastF = nullptr;
                    for (const JsonValue* f = anim->first(); f != nullptr; f = f->next()) {
                        TileAnimFrame* fr = new TileAnimFrame{f->getInt("tileid"), f->getInt("duration", 100), nullptr};
                        a->totalMs += fr->durationMs;
                        if (lastF == nullptr) a->frames = fr; else lastF->next = fr;
                        lastF = fr;
                    }
                    a->next = ts->anims;
                    ts->anims = a;
                }
            }
            if (lastTs == nullptr) tilesets_ = ts; else lastTs->next = ts;
            lastTs = ts;
        }
    }

    // ---- layer
    TileLayer* lastL = nullptr;
    MapObject* lastO = nullptr;
    if (const JsonValue* ls = root->get("layers")) {
        for (const JsonValue* l = ls->first(); l != nullptr; l = l->next()) {
            std::string type = l->getString("type");
            if (type == "tilelayer") {
                TileLayer* tl = new TileLayer();
                tl->name = l->getString("name");
                tl->width = l->getInt("width", width_);
                tl->height = l->getInt("height", height_);
                tl->visible = l->getBool("visible", true);
                tl->opacity = static_cast<float>(l->getNumber("opacity", 1.0));
                PropertyList lp;
                readProps(l->get("properties"), lp);
                tl->above = lp.getBool("above", false);
                const int n = tl->width * tl->height;
                tl->data = new int[n];
                int i = 0;
                if (const JsonValue* d = l->get("data")) {
                    for (const JsonValue* c = d->first(); c != nullptr && i < n; c = c->next()) {
                        double raw = c->asNumber();
                        tl->data[i++] = static_cast<int>(static_cast<unsigned>(static_cast<long long>(raw)) & GID_MASK);
                    }
                }
                while (i < n) tl->data[i++] = 0;
                if (tl->name == "collision") { tl->visible = false; collision_ = tl->data; }
                if (lastL == nullptr) layers_ = tl; else lastL->next = tl;
                lastL = tl;
            } else if (type == "objectgroup") {
                const JsonValue* objs = l->get("objects");
                if (objs == nullptr) continue;
                for (const JsonValue* o = objs->first(); o != nullptr; o = o->next()) {
                    MapObject* mo = new MapObject();
                    mo->id = o->getInt("id");
                    mo->name = o->getString("name");
                    mo->type = o->getString("type", o->getString("class"));
                    mo->x = static_cast<float>(o->getNumber("x"));
                    mo->y = static_cast<float>(o->getNumber("y"));
                    mo->w = static_cast<float>(o->getNumber("width"));
                    mo->h = static_cast<float>(o->getNumber("height"));
                    readProps(o->get("properties"), mo->props);
                    if (lastO == nullptr) objects_ = mo; else lastO->next = mo;
                    lastO = mo;
                }
            }
        }
    }
    delete root;
    if (width_ <= 0 || height_ <= 0) {
        if (error) *error = "ukuran peta tidak valid: " + path;
        return false;
    }
    return true;
}

bool TileMap::blocked(int x, int y) const {
    if (!inside(x, y)) return true;
    return collision_ != nullptr && collision_[y * width_ + x] != 0;
}

const Tileset* TileMap::tilesetFor(int gid) const {
    const Tileset* found = nullptr;
    for (const Tileset* t = tilesets_; t != nullptr; t = t->next)
        if (gid >= t->firstGid && (found == nullptr || t->firstGid > found->firstGid)) found = t;
    return found;
}

const MapObject* TileMap::objectAt(int x, int y, const char* type) const {
    for (const MapObject* o = objects_; o != nullptr; o = o->next) {
        if (type != nullptr && o->type != type) continue;
        // objek bisa berukuran > 1 tile (mis. pintu lebar)
        int x0 = o->tileX(tileW_), y0 = o->tileY(tileH_);
        int tw = o->w > 0 ? static_cast<int>((o->w + tileW_ - 1) / tileW_) : 1;
        int th = o->h > 0 ? static_cast<int>((o->h + tileH_ - 1) / tileH_) : 1;
        if (x >= x0 && x < x0 + tw && y >= y0 && y < y0 + th) return o;
    }
    return nullptr;
}

const MapObject* TileMap::findObject(const std::string& name) const {
    for (const MapObject* o = objects_; o != nullptr; o = o->next)
        if (o->name == name) return o;
    return nullptr;
}

int TileMap::typeIndex(const MapObject* target) const {
    int i = 0;
    for (const MapObject* o = objects_; o != nullptr; o = o->next) {
        if (o == target) return i;
        if (o->type == target->type) ++i;
    }
    return -1;
}
