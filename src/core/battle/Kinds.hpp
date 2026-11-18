#pragma once
// Identitas jenis hero/musuh. Dipakai data battle dan (nanti) GUI untuk
// memilih sprite yang sesuai.

enum class Side { Hero, Enemy };

enum class HeroKind { Varga, Sable, Ione, COUNT };

enum class EnemyKind {
    HollowThrall,
    BellRinger,
    CandleWraith,
    Wisp,
    Gargoyle,
    ChoirWarden,   // boss floor 1
    COUNT
};
