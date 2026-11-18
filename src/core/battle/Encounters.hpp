#pragma once
// Komposisi musuh dan party awal.

#include "Unit.hpp"

enum class EncounterId { Thralls, BellChoir, Candlelight, Gargoyle, ChoirWarden, COUNT };

const char* encounterName(EncounterId id);

// Isi `out` dengan musuh encounter; kembalikan jumlahnya.
int buildEncounter(EncounterId id, Unit out[], int max);

// Isi `out` dengan party awal (Varga, Sable, Ione); kembalikan jumlahnya.
int buildParty(Unit out[], int max);
