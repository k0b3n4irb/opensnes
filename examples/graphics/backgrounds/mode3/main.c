/**
 * @file main.c
 * @brief Mode 3 — 256-Color Background Demo
 *
 * Demonstrates BG Mode 3 (256 colors, 8bpp).
 * The tileset is >32KB, requiring split DMA via assembly loader.
 *
 * Ported from PVSnesLib "Mode3" example.
 */

#include <snes.h>

/* ASM DMA loader — handles bank bytes for SUPERFREE data */
extern void loadGraphics(void);

int main(void) {
    setScreenOff();

    /* Load all assets via ASM (correct bank bytes for >32KB tileset) */
    loadGraphics();

    bgSetMapPtr(0, 0x6000, SC_32x32);

    /* Mode 3: 256 colors, BG1 only */
    setMode(BG_MODE3, 0);
    setMainScreen(LAYER_BG1);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
