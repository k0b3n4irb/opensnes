/**
 * @file main.c
 * @brief Mode 0 parallax scrolling with four independent 2bpp backgrounds
 * @ingroup examples
 *
 * Showcases BG Mode 0, the only SNES video mode that provides four
 * simultaneous background layers. Each layer is limited to 4 colors (2bpp),
 * giving a total of 16 on-screen palette entries across four independent
 * palette banks. The four layers are loaded from separate BMP assets and
 * scroll horizontally at different speeds every 3 frames, creating a
 * parallax depth effect.
 *
 * This is a direct port of the PVSnesLib "Mode0" example.
 *
 * @par SNES Concepts
 * - Mode 0: four BG layers, each 2bpp (4 colors), each with its own palette bank
 * - Palette banking: BG0 uses palette 0-3, BG1 uses 4-7, BG2 uses 8-11, BG3 uses 12-15
 * - bgInitTileSet() loads tiles, palette, and configures the tile base address in one call
 * - Independent per-layer scrolling via bgSetScroll() for parallax effects
 * - VRAM layout: four separate tilemap regions ($0000/$0400/$0800/$0C00) and tile regions
 *
 * @par What to Observe
 * - Four layered backgrounds visible simultaneously
 * - BG1-BG3 scroll horizontally at different speeds (3x, 2x, 1x) for parallax
 * - BG0 (frontmost) remains stationary
 * - Each layer uses a distinct 4-color palette
 *
 * @par Modules Used
 * console, dma, background
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>

/** @name BG0 asset pointers (defined in data.asm via .incbin)
 * @{ */
extern u8 t0[], t0_end[];    /**< BG0 tile data (2bpp) */
extern u8 p0[];              /**< BG0 palette (4 colors, 8 bytes) */
extern u8 bgm0[], bgm0_end[];/**< BG0 tilemap */
/** @} */

/** @name BG1 asset pointers (defined in data.asm via .incbin)
 * @{ */
extern u8 t1[], t1_end[];    /**< BG1 tile data (2bpp) */
extern u8 p1[];              /**< BG1 palette */
extern u8 bgm1[], bgm1_end[];/**< BG1 tilemap */
/** @} */

/** @name BG2 asset pointers (defined in data.asm via .incbin)
 * @{ */
extern u8 t2[], t2_end[];    /**< BG2 tile data (2bpp) */
extern u8 p2[];              /**< BG2 palette */
extern u8 bgm2[], bgm2_end[];/**< BG2 tilemap */
/** @} */

/** @name BG3 asset pointers (defined in data.asm via .incbin)
 * @{ */
extern u8 t3[], t3_end[];    /**< BG3 tile data (2bpp) */
extern u8 p3[];              /**< BG3 palette */
extern u8 bgm3[], bgm3_end[];/**< BG3 tilemap */
/** @} */

/** @brief Horizontal scroll position for BG1 (fastest layer, 3px per tick) */
s16 sxbg1 = 0;
/** @brief Horizontal scroll position for BG2 (medium layer, 2px per tick) */
s16 sxbg2 = 0;
/** @brief Horizontal scroll position for BG3 (slowest layer, 1px per tick) */
s16 sxbg3 = 0;
/** @brief Frame counter for scroll speed throttle (scrolls every 3 frames) */
u16 flip = 0;

/**
 * @brief Entry point -- load 4 BG layers and animate parallax scrolling
 *
 * Loads four independent 2bpp tilesets and tilemaps into separate VRAM
 * regions, one per Mode 0 background layer. Each layer gets its own
 * palette bank (Mode 0 provides 4 banks of 4 colors). After setup, the
 * main loop increments scroll positions at different speeds every 3
 * frames to create a parallax depth illusion: BG1 (foreground) moves
 * fastest, BG3 (farthest) moves slowest, and BG0 stays stationary.
 *
 * @return Never returns (infinite loop).
 */
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
