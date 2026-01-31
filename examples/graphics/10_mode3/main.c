/**
 * @file main.c
 * @brief Mode 3 Example - 256-color Background
 *
 * Port of PVSnesLib Mode3 example.
 * Demonstrates Mode 3 with 256-color (8bpp) graphics.
 *
 * Mode 3 characteristics:
 * - BG1: 256 colors (8bpp)
 * - BG2: 16 colors (4bpp)
 * - Good for detailed single-layer backgrounds
 */

#include <snes.h>

extern void load_mode3_graphics(void);
extern void setup_mode3_regs(void);

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    load_mode3_graphics();
    setup_mode3_regs();

    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
