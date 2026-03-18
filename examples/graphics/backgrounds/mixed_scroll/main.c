/**
 * @file main.c
 * @brief Mixed scrolling with fixed and auto-scrolling background layers
 * @ingroup examples
 *
 * Layers two Mode 1 backgrounds: BG1 displays a repeating shader pattern that
 * auto-scrolls diagonally each frame, while BG2 shows a static logo that
 * remains fixed. Each layer uses its own tile set and palette slot in VRAM
 * and CGRAM (BG1 tiles at $4000 with palette slot 1, BG2 tiles at $5000
 * with palette slot 0). The tilemaps are placed at non-overlapping VRAM
 * addresses ($1800 for BG1, $1400 for BG2). This demonstrates how the SNES
 * PPU composites multiple BG layers with independent scroll offsets, making
 * it straightforward to combine static UI elements with animated backgrounds.
 *
 * @par SNES Concepts
 * - Independent per-layer scroll registers (BG1 scrolls, BG2 stays fixed)
 * - Multiple tile sets sharing VRAM without overlap
 * - Separate palette slots per layer via gfx4snes `-e` flag
 * - Mode 1 dual-layer compositing (BG1 behind BG2)
 *
 * @par What to Observe
 * - The shader pattern (BG1) scrolls diagonally and wraps seamlessly
 * - The logo (BG2) remains stationary in the center
 * - Both layers composite together with proper priority ordering
 *
 * @par Modules Used
 * console, sprite, dma, background
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>

extern u8 bg1_tiles, bg1_tiles_end;
extern u8 bg1_pal, bg1_pal_end;
extern u8 bg1_map, bg1_map_end;

extern u8 bg2_tiles, bg2_tiles_end;
extern u8 bg2_pal, bg2_pal_end;
extern u8 bg2_map, bg2_map_end;

short scrX = 0, scrY = 0;

int main(void) {
    consoleInit();

    /* Load BG2 tiles + palette (pvsneslib logo, palette slot 0, tiles at $5000) */
    bgInitTileSet(1, &bg2_tiles, &bg2_pal, 0,
                  &bg2_tiles_end - &bg2_tiles,
                  &bg2_pal_end - &bg2_pal,
                  BG_16COLORS, 0x5000);

    /* Load BG1 tiles + palette (shader, palette slot 1, tiles at $4000) */
    bgInitTileSet(0, &bg1_tiles, &bg1_pal, 1,
                  &bg1_tiles_end - &bg1_tiles,
                  &bg1_pal_end - &bg1_pal,
                  BG_16COLORS, 0x4000);

    /* Set tilemap locations */
    bgSetMapPtr(1, 0x1400, SC_32x32);
    bgSetMapPtr(0, 0x1800, SC_32x32);

    /* Copy tilemaps to VRAM */
    WaitForVBlank();
    dmaCopyVram(&bg2_map, 0x1400, &bg2_map_end - &bg2_map);
    dmaCopyVram(&bg1_map, 0x1800, &bg1_map_end - &bg1_map);

    /* Mode 1, enable only BG1 + BG2 (BG3 disabled) */
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_BG2;

    setScreenOn();

    while (1) {
        scrX++;
        scrY++;
        bgSetScroll(0, scrX, scrY);

        WaitForVBlank();
    }
    return 0;
}
