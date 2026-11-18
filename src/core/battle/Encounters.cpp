#include "Encounters.hpp"

const char* encounterName(EncounterId id) {
    switch (id) {
        case EncounterId::Thralls:     return "Hollow Thralls";
        case EncounterId::BellChoir:   return "Lonceng Kematian";
        case EncounterId::Candlelight: return "Cahaya Lilin";
        case EncounterId::Gargoyle:    return "Penjaga Batu";
        case EncounterId::ChoirWarden: return "BOSS: The Choir Warden";
        case EncounterId::SisterOfAsh: return "BOSS: Sister of Ash";
        case EncounterId::LastFlame:   return "BOSS: The Last Flame";
        default:                       return "?";
    }
}

// Tambahkan musuh; jika jenis yang sama muncul lebih dari sekali beri akhiran A, B, C.
static void push(Unit out[], int& n, int max, EnemyKind k) {
    if (n >= max) return;
    out[n] = makeEnemy(k);
    int same = 0;
    for (int i = 0; i < n; ++i)
        if (out[i].kind == static_cast<int>(k)) ++same;
    if (same > 0) {
        if (same == 1) {   // beri akhiran juga ke yang pertama
            for (int i = 0; i < n; ++i)
                if (out[i].kind == static_cast<int>(k)) out[i].name += " A";
        }
        out[n].name += std::string(" ") + static_cast<char>('A' + same);
    }
    ++n;
}

int buildEncounter(EncounterId id, Unit out[], int max) {
    int n = 0;
    switch (id) {
        case EncounterId::Thralls:
            push(out, n, max, EnemyKind::HollowThrall);
            push(out, n, max, EnemyKind::HollowThrall);
            push(out, n, max, EnemyKind::HollowThrall);
            break;
        case EncounterId::BellChoir:
            push(out, n, max, EnemyKind::HollowThrall);
            push(out, n, max, EnemyKind::BellRinger);
            push(out, n, max, EnemyKind::HollowThrall);
            break;
        case EncounterId::Candlelight:
            push(out, n, max, EnemyKind::CandleWraith);
            push(out, n, max, EnemyKind::CandleWraith);
            break;
        case EncounterId::Gargoyle:
            push(out, n, max, EnemyKind::HollowThrall);
            push(out, n, max, EnemyKind::Gargoyle);
            break;
        case EncounterId::SisterOfAsh:
            push(out, n, max, EnemyKind::SisterOfAsh);
            break;
        case EncounterId::LastFlame:
            push(out, n, max, EnemyKind::Wisp);
            push(out, n, max, EnemyKind::LastFlame);
            break;
        case EncounterId::ChoirWarden:
        default:
            push(out, n, max, EnemyKind::ChoirWarden);
            break;
    }
    return n;
}

int buildParty(Unit out[], int max) {
    int n = 0;
    if (n < max) out[n++] = makeHero(HeroKind::Varga);
    if (n < max) out[n++] = makeHero(HeroKind::Sable);
    if (n < max) out[n++] = makeHero(HeroKind::Ione);
    return n;
}
