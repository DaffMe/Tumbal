#pragma once
// Framework test minimal buatan sendiri: CHECK(kondisi) mencatat lulus/gagal.

#include <cstdio>

inline int g_pass = 0;
inline int g_fail = 0;

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (cond) { ++g_pass; }                                                           \
        else { ++g_fail; std::printf("  GAGAL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

#define SECTION(name) std::printf("[%s]\n", name)

void runAdtTests();
void runWorldTests();
void runBattleTests();
