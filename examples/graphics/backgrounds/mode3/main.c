/**
 * @file main.c
 * @brief Mode 3 full-color (256-color, 8bpp) background with assembly DMA loader
 * @ingroup examples
 *
 * Displays a 256-color background using Mode 3, the highest color-depth
 * tiled mode on the SNES. Each tile is 64 bytes (8 bitplanes), so even a
 * modest tileset exceeds 32 KB and cannot fit in a single ROM bank. The
 * tile data is placed in SUPERFREE sections by the linker, which may
 * assign them to bank $01 or higher. A hand-written assembly DMA loader
 * (loadGraphics in data.asm) uses the linker-provided bank byte (:label)
 * to transfer tile data, tilemap, and palette to VRAM/CGRAM correctly
 * regardless of which bank the data lands in.
 *
 * This is a direct port of the PVSnesLib "Mode3" example.
 *
 * @par SNES Concepts
 * - Mode 3: single BG layer at 8bpp (256 colors from a 256-entry CGRAM palette)
 * - 8bpp tile format: 64 bytes per tile (8 interleaved bitplanes)
 * - Bank overflow: >32 KB tile data requires SUPERFREE sections + assembly DMA with :label bank byte
 * - Assembly DMA loader pattern: sets $4304 (bank), $4302 (address), $4305 (size) per transfer
 * - VRAM layout: tiles at $0000 (large), tilemap at $6000
 *
 * @par What to Observe
 * - A single full-screen image with up to 256 unique colors
 * - Richer color gradients and detail compared to Mode 1 (16 colors)
 * - Static display with no scrolling
 *
 * @par Modules Used
 * console, dma, background
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>

/**
 * @brief Assembly DMA loader for large 8bpp tile data (defined in data.asm)
 *
 * Because Mode 3 tile data (64 bytes per 8x8 tile) typically exceeds 32KB,
 * it cannot fit in a single ROM bank. The linker places SUPERFREE sections
 * in whatever bank has space, potentially bank $01 or higher. C code cannot
 * access these banks directly (lda.l $0000,x always reads bank $00), so an
 * assembly routine uses the linker-provided bank byte (:label) to set the
 * DMA source bank register ($4304) correctly for each transfer.
 */
extern void loadGraphics(void);

/**
 * @brief Entry point -- load 256-color tileset via ASM and display Mode 3 image
 *
 * Forces blank, calls the assembly DMA loader to transfer 8bpp tile data,
 * tilemap, and 256-color palette into VRAM/CGRAM with correct bank bytes,
 * then configures Mode 3 and turns on the screen. Mode 3 supports a single
 * BG layer with 256 colors per tile (8 bitplanes), providing the richest
 * color depth available in any tiled SNES mode. The tradeoff is that only
 * one BG layer is available (plus a second 4-color direct-color BG that
 * is unused here).
 *
 * @return Never returns (infinite loop).
 */
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
