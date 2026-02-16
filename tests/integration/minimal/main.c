/**
 * @file main.c
 * @brief Minimal Smoke Test
 *
 * The simplest possible ROM: sets a blue background and loops.
 * Verifies the toolchain (compiler, assembler, linker) works end-to-end.
 */

#include <snes.h>

int main(void) {
    consoleInit();

    /* Set palette color 0 to blue */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x7C;

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
