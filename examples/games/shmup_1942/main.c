/**
 * @file main.c
 * @brief shmup_1942 — S1: Static tile library preview
 * @ingroup examples
 *
 * First stage of an iterative shoot 'em up project built on Kenney's CC0
 * *Pixel Shmup* pack. S1 ships only the import pipeline: gfx4snes converts
 * `res/ground.png` (the 192×112 grass-biome terrain library split out of
 * `tiles_packed.png`) into a tileset + palette + tilemap, and main()
 * displays it on BG1 in Mode 1 with no scrolling and no sprites.
 *
 * The 24×14 tilemap sits in the top-left of the 32×32 SNES screen; the rest
 * of the area falls back to tile 0 from the converted library. The result is
 * a deliberately raw tile-library snapshot, not a designed level — the
 * actual screen composition + player + scrolling come in S2-S3.
 *
 * @par SNES Concepts
 * - Mode 1: BG1 is 4bpp (up to 16 colors per palette slot)
 * - DECLARE_BG_ASSET bundles tiles + palette + tilemap with the colour-mode
 *   and map-size locked at declaration time
 * - bgLoad() collapses three manual VRAM/CGRAM setup calls into one
 * - VRAM layout: tilemap at $0000, tile graphics at $4000 (no overlap)
 *
 * @par What to Observe
 * - The 192×112 grass-biome library appears in the top-left of the screen
 * - The rest of the screen tiles with the first tile of the library
 * - No movement; everything is static
 *
 * @par Modules Used
 * console, dma, background, asset
 *
 * @see background.h, asset.h
 */

#include <snes.h>
#include <snes/asset.h>

/**
 * @brief Ground tileset bundle (4bpp, 16 colors, 32×32 tilemap).
 *
 * `DECLARE_BG_ASSET(ground, ...)` expands to extern declarations for
 * `ground_tiles`, `ground_pal`, `ground_map` (with matching `_end` labels)
 * and a `static const BgAsset ground = {...}`. The data symbols themselves
 * live in `data.asm`.
 */
DECLARE_BG_ASSET(ground, BG_16COLORS, SC_32x32);

/**
 * @brief Entry point — load the ground tile library and hold a static frame.
 *
 * @return Never returns (infinite VBlank wait).
 */
int main(void) {
    setScreenOff();

    /* BG1: tilemap at VRAM $0000, tiles at $4000, palette slot 0 */
    bgLoad(0, &ground, 0, 0x4000, 0x0000);

    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1);
    bgSetScroll(0, 0, 0);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
