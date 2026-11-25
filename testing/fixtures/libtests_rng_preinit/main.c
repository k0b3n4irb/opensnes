/**
 * @file main.c
 * @brief Fixture: rngNext() before consoleInit() must not be stuck at 0.
 *
 * rand_seed was set only by consoleInit()/rngSeed(). Before them it was 0,
 * the LFSR's fixed point: rngNext() returned 0 forever. The fixture draws
 * four values first thing and counts the distinct non-zero ones.
 */
#include <snes.h>

/** @brief the four first values drawn before any Init */
u16 r_v[4];
/** @brief how many of them are non-zero */
u16 r_nonzero;
/** @brief marker: the run reached its end */
u16 r_done;

int main(void) {
    u8 i;

    for (i = 0; i < 4; i++) {
        r_v[i] = rngNext();
        if (r_v[i] != 0) r_nonzero++;
    }
    r_done = 0xC0DE;
    while (1) { }
    return 0;
}
