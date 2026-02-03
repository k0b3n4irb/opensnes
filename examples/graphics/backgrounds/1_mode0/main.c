/**
 * @file main.c
 * @brief Mode 0 Background Example
 *
 * Port of PVSnesLib Mode0 example.
 * Demonstrates Mode 0 with 4 background layers, each with 4 colors.
 * Features parallax scrolling on BG1, BG2, and BG3.
 *
 * Mode 0 characteristics:
 * - 4 background layers
 * - 4 colors per BG (2bpp tiles)
 * - Each BG has its own palette section:
 *   - BG1: CGRAM 0-31
 *   - BG2: CGRAM 32-63
 *   - BG3: CGRAM 64-95
 *   - BG4: CGRAM 96-127
 */

#include <snes.h>

/* External assembly functions */
extern void load_mode0_graphics(void);
extern void setup_mode0_regs(void);
extern void update_scrolling(void);

int main(void) {
    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Load all 4 backgrounds to VRAM */
    load_mode0_graphics();

    /* Configure PPU for Mode 0 */
    setup_mode0_regs();

    /* Enable display at full brightness */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Update parallax scrolling */
        update_scrolling();
    }

    return 0;
}
