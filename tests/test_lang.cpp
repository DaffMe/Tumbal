// Unit test bahasa: kamus tr/trf, ganti bahasa, dan cakupan terjemahan Inggris untuk semua teks
// yang tampil ke pemain (skrip event, data game, nama peta).
#include <cstdio>
#include <string>
#include "TestFramework.hpp"
#include "../src/core/data/GameData.hpp"
#include "../src/core/util/Json.hpp"
#include "../src/core/util/Lang.hpp"
#include "../src/core/world/World.hpp"

static std::string assetsRoot() {
    if (std::FILE* f = std::fopen("assets/lang/en.json", "rb")) { std::fclose(f); return "assets/"; }
    return "../assets/";
}

static int g_missing = 0;

static void need(const std::string& key) {
    if (key.empty() || lang::has(key)) return;
    if (g_missing < 10) std::printf("  belum diterjemahkan: \"%s\"\n", key.c_str());
    ++g_missing;
}

static void walkScript(const JsonValue* cmds) {
    for (const JsonValue* c = cmds ? cmds->first() : nullptr; c != nullptr; c = c->next()) {
        std::string op = c->getString("cmd");
        if (op == "text") { need(c->getString("who")); need(c->getString("text")); }
        if (op == "notice") need(c->getString("text"));
        if (const JsonValue* o = c->get("options"))
            for (const JsonValue* x = o->first(); x != nullptr; x = x->next()) need(x->asString());
        if (const JsonValue* b = c->get("branches"))
            for (const JsonValue* x = b->first(); x != nullptr; x = x->next()) walkScript(x);
        walkScript(c->get("then"));
        walkScript(c->get("else"));
    }
}

static void testDictionary() {
    SECTION("Bahasa: tr/trf, fallback ke teks Indonesia, ganti bahasa");
    lang::clear();
    lang::setLanguage(LANG_EN);
    CHECK(tr("Sukma") == "Sukma");                          // tanpa kamus: teks sumber
    CHECK(lang::load(assetsRoot() + "lang/en.json"));
    CHECK(lang::size() > 400);
    CHECK(tr("Sukma") == "Souls");
    CHECK(tr("Mulai Baru") == "New Game");
    CHECK(trf("Didapat: %s x%d", tr("Bijih Besi").c_str(), 2) == "Obtained: Iron Ore x2");
    CHECK(tr("nama pemain bebas") == "nama pemain bebas");  // bukan kunci: apa adanya
    CHECK(tr("") == "");
    lang::setLanguage(LANG_ID);
    CHECK(tr("Sukma") == "Sukma");
    CHECK(trf("Sukma +%d", 5) == "Sukma +5");
    lang::setLanguage(LANG_EN);
    CHECK(trf("Sukma +%d", 5) == "Souls +5");
}

static void testCoverage() {
    SECTION("Bahasa: setiap teks cerita, data game, dan nama peta punya terjemahan Inggris");
    g_missing = 0;
    const char* files[] = {"events/lumbung.json", "events/desa.json", "events/ladang.json", "events/akhir.json"};
    for (const char* f : files) {
        JsonValue* root = JsonValue::parseFile(assetsRoot() + f);
        CHECK(root != nullptr);
        if (root == nullptr) continue;
        for (const JsonValue* s = root->first(); s != nullptr; s = s->next()) walkScript(s);
        delete root;
    }
    for (int i = 1; i < IT_COUNT; ++i) if (const ItemDef* d = itemDef(i)) { need(d->name); need(d->desc); }
    for (int i = 1; i < SKL_COUNT; ++i) if (const SkillDef* d = skillDef(i)) { need(d->name); need(d->desc); }
    for (int i = 1; i < EN_COUNT; ++i) if (const EnemyDef* d = enemyDef(i)) need(d->name);
    for (int i = 1; i < TR_COUNT; ++i) if (const TroopDef* d = troopDef(i)) need(d->name);
    for (int i = 0; i < KL_COUNT; ++i) { need(classDef(i).name); need(classDef(i).desc); need(weaponDef(i).name); }
    for (int i = 1; i < ST_COUNT; ++i) { need(statusDef(i).name); need(statusDef(i).shortName); }
    for (int i = 0; i < shopCount(); ++i) need(itemDef(shopEntry(i).item)->name);
    World w;
    std::string err;
    CHECK(w.load(assetsRoot() + "maps/world.json", &err));
    w.graph().forEachVertex([&](World::MapGraph::Vertex* v) { need(v->data.name); });
    if (g_missing > 0) std::printf("  total belum diterjemahkan: %d\n", g_missing);
    CHECK(g_missing == 0);
    lang::clear();                                          // test lain memakai teks sumber
}

void runLangTests() {
    testDictionary();
    testCoverage();
}
