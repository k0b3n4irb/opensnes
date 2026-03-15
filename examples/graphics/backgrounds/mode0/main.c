/**
 * @file main.c
 * @brief Mode 0 — 4-Layer 2bpp Background Demo
 *
 * Demonstrates BG Mode 0 (four 4-color backgrounds, 2bpp each).
 * Each BG uses a separate 4-color palette and has independent scrolling.
 *
 * Ported from PVSnesLib "Mode0" example.
 */

#include <snes.h>

extern u8 t0[], t0_end[], p0[], bgm0[], bgm0_end[];
extern u8 t1[], t1_end[], p1[], bgm1[], bgm1_end[];
extern u8 t2[], t2_end[], p2[], bgm2[], bgm2_end[];
extern u8 t3[], t3_end[], p3[], bgm3[], bgm3_end[];

s16 sxbg1 = 0, sxbg2 = 0, sxbg3 = 0;
u16 flip = 0;

int main(void) {
    /* Load 4 tilesets to VRAM at separate addresses.
     * Mode 0 palette banking: each BG has its own 4-color palette bank.
     * BG0=bank 0, BG1=bank 1, BG2=bank 2, BG3=bank 3 */
    bgInitTileSet(0, t0, p0, 0, t0_end - t0, 8, BG_4COLORS0, 0x2000);
    bgInitTileSet(1, t1, p1, 0, t1_end - t1, 8, BG_4COLORS0, 0x3000);
    bgInitTileSet(2, t2, p2, 0, t2_end - t2, 8, BG_4COLORS0, 0x4000);
    bgInitTileSet(3, t3, p3, 0, t3_end - t3, 16, BG_4COLORS0, 0x5000);

    /* Load 4 tilemaps */
    WaitForVBlank();
    dmaCopyVram(bgm0, 0x0000, bgm0_end - bgm0);
    bgSetMapPtr(0, 0x0000, SC_32x32);

    dmaCopyVram(bgm1, 0x0400, bgm1_end - bgm1);
    bgSetMapPtr(1, 0x0400, SC_32x32);

    dmaCopyVram(bgm2, 0x0800, bgm2_end - bgm2);
    bgSetMapPtr(2, 0x0800, SC_32x32);

    dmaCopyVram(bgm3, 0x0C00, bgm3_end - bgm3);
    bgSetMapPtr(3, 0x0C00, SC_32x32);

    /* Mode 0: 4 BGs — enable all on main screen */
    setMode(BG_MODE0, 0);
    setMainScreen(LAYER_BG1 | LAYER_BG2 | LAYER_BG3 | LAYER_BG4);
    setScreenOn();

    while (1) {
        /* Scroll BG1-3 at different speeds, every 3 frames */
        flip++;
        if (flip == 3) {
            flip = 0;
            sxbg3++;
            sxbg2 += 2;
            sxbg1 += 3;
            bgSetScroll(1, sxbg1, 0);
            bgSetScroll(2, sxbg2, 0);
            bgSetScroll(3, sxbg3, 0);
        }
        WaitForVBlank();
    }

    return 0;
}
