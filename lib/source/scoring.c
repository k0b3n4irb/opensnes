/**
 * @file scoring.c
 * @brief Score management implementation
 *
 * Pure C implementation of PVSnesLib's scoring system.
 * Uses a two-part u16 representation with carry at 10000 (0x2710).
 */

#include <snes/scoring.h>

void scoreClear(scoMemory *score) {
    score->scolo = 0;
    score->scohi = 0;
}

void scoreAdd(scoMemory *score, u16 value) {
    score->scolo = score->scolo + value;
    while (score->scolo >= 0x2710) {
        score->scolo = score->scolo - 0x2710;
        score->scohi = score->scohi + 1;
    }
}

void scoreCpy(scoMemory *src, scoMemory *dst) {
    dst->scolo = src->scolo;
    dst->scohi = src->scohi;
}

u8 scoreCmp(scoMemory *a, scoMemory *b) {
    if (a->scohi > b->scohi) return 0xFF;
    if (a->scohi < b->scohi) return 1;
    if (a->scolo > b->scolo) return 0xFF;
    if (a->scolo < b->scolo) return 1;
    return 0;
}
