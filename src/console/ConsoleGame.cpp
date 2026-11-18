#include "ConsoleGame.hpp"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>

#include "core/battle/Battle.hpp"
#include "core/battle/Encounters.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

// ---------------------------------------------------------------- warna ANSI
const char* RST  = "\x1b[0m";
const char* BOLD = "\x1b[1m";
const char* DIM  = "\x1b[90m";
const char* RED  = "\x1b[91m";
const char* GRN  = "\x1b[92m";
const char* YEL  = "\x1b[93m";
const char* BLU  = "\x1b[94m";
const char* MAG  = "\x1b[95m";
const char* CYN  = "\x1b[96m";

bool g_trace  = true;    // tampilkan operasi struktur data
bool g_ambush = false;   // simulasi lentera gelap (hubungan M2 → M1)

void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

std::string readLine() {
    std::printf("%s> %s", YEL, RST);
    std::fflush(stdout);
    std::string s;
    if (!std::getline(std::cin, s)) return "q";   // EOF → keluar
    // trim spasi
    size_t a = s.find_first_not_of(" \t\r");
    size_t b = s.find_last_not_of(" \t\r");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

// Parse bilangan bulat non-negatif; -1 jika bukan angka.
int parseInt(const std::string& s) {
    if (s.empty() || s.size() > 6) return -1;
    int v = 0;
    for (char ch : s) {
        if (ch < '0' || ch > '9') return -1;
        v = v * 10 + (ch - '0');
    }
    return v;
}

std::string bar(int cur, int max, int width, const char* color) {
    int filled = max > 0 ? (cur * width + max - 1) / max : 0;
    std::string s = color;
    for (int i = 0; i < width; ++i) s += (i < filled) ? "█" : "░";
    return s + RST;
}

std::string pad(const std::string& s, size_t w) {
    return s.size() >= w ? s : s + std::string(w - s.size(), ' ');
}

std::string statusList(const Unit& u) {
    std::string s;
    for (int i = 0; i < u.statusCount; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof buf, " %s[%s %d]%s", MAG, statusName(u.statuses[i].type), u.statuses[i].turns, RST);
        s += buf;
    }
    return s;
}

const char* sideColor(const Unit& u) { return u.side == Side::Hero ? CYN : RED; }

// ------------------------------------------------------------- event queue
// Kosongkan Battle Event Queue dan cetak tiap event berurutan (FIFO).
void drainEvents(Battle& b) {
    BattleEvent e;
    while (b.pollEvent(e)) {
        switch (e.type) {
            case EventType::DsOp:
                if (g_trace) std::printf("%s      [DS] %s%s\n", DIM, e.text.c_str(), RST);
                break;
            case EventType::TurnStart:
                std::printf("\n%s%s── %s ──%s\n", BOLD, sideColor(b.unit(e.source)), e.text.c_str(), RST);
                break;
            case EventType::Damage:
                std::printf("   %s%s%s\n", RED, e.text.c_str(), RST);
                break;
            case EventType::Heal:
                std::printf("   %s%s%s\n", GRN, e.text.c_str(), RST);
                break;
            case EventType::Break:
            case EventType::Interject:
            case EventType::ExtraTurn:
                std::printf("   %s%s%s%s\n", BOLD, YEL, e.text.c_str(), RST);
                break;
            case EventType::AvShift:
            case EventType::Summon:
            case EventType::Rewind:
                std::printf("   %s%s%s\n", BLU, e.text.c_str(), RST);
                break;
            case EventType::Death:
                std::printf("   %s%s✝ %s%s\n", BOLD, RED, e.text.c_str(), RST);
                break;
            case EventType::Victory:
                std::printf("\n%s%s★ %s ★%s\n", BOLD, GRN, e.text.c_str(), RST);
                break;
            case EventType::Defeat:
                std::printf("\n%s%s✝ %s ✝%s\n", BOLD, RED, e.text.c_str(), RST);
                break;
            default:
                std::printf("   %s\n", e.text.c_str());
                break;
        }
    }
}

// ------------------------------------------------------------- tampilan state
void printUnits(Battle& b, Side side) {
    for (int i = 0; i < b.unitCount(); ++i) {
        const Unit& u = b.unit(i);
        if (u.side != side) continue;
        if (!u.alive && side == Side::Enemy) continue;
        bool isCur = (i == b.currentIndex());
        std::printf(" %s%s[%d] %s%s", isCur ? "▶" : " ", sideColor(u), i, pad(u.name, 18).c_str(), RST);
        if (!u.alive) {
            std::printf("%s(tumbang)%s\n", DIM, RST);
            continue;
        }
        std::printf("HP %3d/%-3d %s", u.hp, u.maxHp, bar(u.hp, u.maxHp, 12, side == Side::Hero ? GRN : RED).c_str());
        if (side == Side::Hero) {
            std::printf("  Resolve %3d%%%s", u.resolve, u.resolve >= MAX_RESOLVE ? " ✦" : "  ");
        } else if (u.maxToughness > 0) {
            if (u.broken) std::printf("  %s%sBREAK%s       ", BOLD, YEL, RST);
            else std::printf("  Tough %s", bar(u.toughness, u.maxToughness, 6, YEL).c_str());
        }
        std::printf("  SPD %.0f%s\n", u.spd(), statusList(u).c_str());
    }
}

void printTimeline(Battle& b) {
    // Isi node Doubly Linked List apa adanya (HEAD → TAIL)
    std::printf("\n %sTimeline (DLL):%s HEAD", BOLD, RST);
    b.timeline().forEach([&](const Timeline<Unit>::Node& n) {
        std::printf(" ⇄ %s%s %.0f%s", sideColor(*n.item), n.item->name.c_str(), n.av, RST);
    });
    std::printf(" ⇄ TAIL\n");

    // Prediksi 8 giliran berikutnya (simulasi pada salinan timeline)
    int order[8];
    int n = b.predictTurnOrder(8, order);
    std::printf(" %sPrediksi    :%s ", BOLD, RST);
    if (b.currentIndex() >= 0) std::printf("%s[%s]%s", BOLD, b.unit(b.currentIndex()).name.c_str(), RST);
    for (int i = 0; i < n; ++i) {
        const Unit& u = b.unit(order[i]);
        std::printf(" → %s%s%s", sideColor(u), u.name.c_str(), RST);
    }
    std::printf("\n");
}

void printState(Battle& b) {
    std::printf("\n%s════════════════════════════════════════════════════════════════════%s\n", DIM, RST);
    std::string ember;
    for (int i = 0; i < MAX_EMBER; ++i) ember += (i < b.ember()) ? "◆" : "◇";
    std::printf(" Giliran %d   Ember %s%s%s   Hourglass %d   Snapshot di Stack: %d\n",
                b.turnNumber(), YEL, ember.c_str(), RST, b.hourglass(), b.historySize());
    std::printf("\n %sPARTY%s\n", BOLD, RST);
    printUnits(b, Side::Hero);
    std::printf("\n %sMUSUH%s\n", BOLD, RST);
    printUnits(b, Side::Enemy);
    printTimeline(b);
}

// ------------------------------------------------------------- input pemain
int chooseTarget(Battle& b, const Skill& s) {
    if (!needsTarget(s.target)) return -1;
    int cur = b.currentIndex();
    std::printf(" Pilih target:");
    int first = -1;
    for (int i = 0; i < b.unitCount(); ++i) {
        if (!b.isValidTarget(cur, s.target, i)) continue;
        if (first < 0) first = i;
        std::printf("  %s[%d] %s%s", sideColor(b.unit(i)), i, b.unit(i).name.c_str(), RST);
    }
    std::printf("   (Enter = [%d], b = batal)\n", first);
    while (true) {
        std::string in = readLine();
        if (in == "b" || in == "q") return -2;
        if (in.empty()) return first;
        int t = parseInt(in);
        if (b.isValidTarget(cur, s.target, t)) return t;
        std::printf(" %sTarget tidak valid.%s\n", RED, RST);
    }
}

bool doInterject(Battle& b) {
    std::printf(" Hero yang bisa Interject:");
    int any = 0;
    for (int i = 0; i < b.unitCount(); ++i) {
        if (!b.canInterject(i)) continue;
        std::printf("  %s[%d] %s (%s)%s", CYN, i, b.unit(i).name.c_str(), b.unit(i).ultimate->name, RST);
        ++any;
    }
    if (any == 0) {
        std::printf(" %stidak ada (Resolve harus 100%%)%s\n", DIM, RST);
        return false;
    }
    std::printf("\n");
    int idx = parseInt(readLine());
    if (!b.interject(idx)) {
        std::printf(" %sInterject gagal.%s\n", RED, RST);
        return false;
    }
    return true;
}

// Satu keputusan pemain. Mengembalikan false jika pemain menyerah.
bool playerTurn(Battle& b) {
    const Unit& cur = b.unit(b.currentIndex());
    std::printf("\n %s%sGiliran %s%s — pilih aksi:\n", BOLD, CYN, cur.name.c_str(), RST);

    if (cur.empowered) {
        const Skill* u = cur.ultimate;
        std::printf("  %su)%s %s%s%s — %s\n", YEL, RST, BOLD, u->name, RST, u->desc);
    } else {
        for (int s = 0; s < cur.skillCount; ++s) {
            const Skill* sk = cur.skills[s];
            bool ok = b.canUseSkill(s);
            std::printf("  %s%d)%s %s%s%s", ok ? YEL : DIM, s + 1, RST, ok ? "" : DIM, sk->name, RST);
            if (sk->emberCost > 0) std::printf(" %s[%d Ember]%s", YEL, sk->emberCost, RST);
            std::printf(" — %s%s%s\n", DIM, sk->desc, RST);
        }
    }
    std::printf("  %si)%s Interject   %sr)%s Rewind (%d)   %sd)%s Trace DS: %s   %sq)%s Menyerah\n",
                YEL, RST, YEL, RST, b.hourglass(), YEL, RST, g_trace ? "ON" : "OFF", YEL, RST);

    while (true) {
        std::string in = readLine();
        if (in == "q") return false;
        if (in == "d") {
            g_trace = !g_trace;
            std::printf(" Trace struktur data: %s\n", g_trace ? "ON" : "OFF");
            continue;
        }
        if (in == "i") {
            doInterject(b);
            drainEvents(b);
            return true;     // tampilkan ulang state (timeline berubah)
        }
        if (in == "r") {
            if (!b.rewindTurn()) std::printf(" %sTidak bisa rewind (butuh Hourglass dan giliran sebelumnya).%s\n", RED, RST);
            drainEvents(b);
            return true;
        }

        int slot = -1;
        if (cur.empowered && (in == "u" || in == "1")) slot = ULT_SLOT;
        else if (!cur.empowered) {
            int n = parseInt(in);
            if (n >= 1 && n <= cur.skillCount) slot = n - 1;
        }
        if (slot < 0) {
            std::printf(" %sPilihan tidak dikenal.%s\n", RED, RST);
            continue;
        }
        if (!b.canUseSkill(slot)) {
            std::printf(" %sEmber tidak cukup.%s\n", RED, RST);
            continue;
        }
        const Skill* sk = b.skillAt(slot);
        int target = chooseTarget(b, *sk);
        if (target == -2) return true;
        if (!b.playerUseSkill(slot, target)) std::printf(" %sAksi gagal.%s\n", RED, RST);
        drainEvents(b);
        return true;
    }
}

// ------------------------------------------------------------- satu battle
void playBattle(EncounterId id) {
    Unit heroes[3];
    int hc = buildParty(heroes, 3);
    Unit enemies[MAX_UNITS];
    int ec = buildEncounter(id, enemies, MAX_UNITS);

    Battle b(heroes, hc, enemies, ec, static_cast<std::uint32_t>(std::time(nullptr)), 3, g_ambush);
    std::printf("\n%s%s⚔  %s%s\n", BOLD, RED, encounterName(id), RST);
    b.start();
    drainEvents(b);

    while (b.state() == BattleState::WaitingForPlayer) {
        printState(b);
        if (!playerTurn(b)) {
            std::printf("\n Party mundur dari pertarungan.\n");
            return;
        }
    }
    std::printf("\n Pertarungan selesai dalam %d giliran.\n", b.turnNumber());
}

}  // namespace

int runConsole() {
    setupConsole();
    std::printf("%s%s\n   C I N D E R W A K E   — mode console%s\n", BOLD, YEL, RST);
    std::printf("%s   Logika & struktur data yang sama dengan versi GUI.%s\n", DIM, RST);

    while (true) {
        std::printf("\n%sPilih pertarungan:%s\n", BOLD, RST);
        for (int i = 0; i < static_cast<int>(EncounterId::COUNT); ++i)
            std::printf("  %s%d)%s %s\n", YEL, i + 1, RST, encounterName(static_cast<EncounterId>(i)));
        std::printf("  %sa)%s Lentera gelap / ambush: %s\n", YEL, RST, g_ambush ? "ON" : "OFF");
        std::printf("  %sd)%s Trace struktur data: %s\n", YEL, RST, g_trace ? "ON" : "OFF");
        std::printf("  %sq)%s Keluar\n", YEL, RST);

        std::string in = readLine();
        if (in == "q") break;
        if (in == "a") { g_ambush = !g_ambush; continue; }
        if (in == "d") { g_trace = !g_trace; continue; }
        int n = parseInt(in);
        if (n >= 1 && n <= static_cast<int>(EncounterId::COUNT)) playBattle(static_cast<EncounterId>(n - 1));
    }
    return 0;
}
