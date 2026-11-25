/**
 * @file main.c
 * @brief Fixture: isPAL() before consoleInit() must report the real region.
 *
 * isPAL() returned 0 (NTSC) until consoleInit() had read $213F. The fixture
 * asks first thing, before any Init, and keeps the answer.
 */
#include <snes.h>

/** @brief isPAL() as asked before any Init */
u16 r_ispal;
/** @brief marker: the run reached its end */
u16 r_done;

int main(void) {
    r_ispal = isPAL();
    r_done = 0xC0DE;
    while (1) { }
    return 0;
}
