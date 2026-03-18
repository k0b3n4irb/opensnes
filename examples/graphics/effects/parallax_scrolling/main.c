/**
 * @file main.c
 * @brief HDMA-driven parallax scrolling with three speed zones
 * @ingroup examples
 *
 * Demonstrates parallax scrolling using HDMA (H-Blank DMA) to override
 * the BG1 horizontal scroll register (BG1HOFS, $210D) at different
 * scanline boundaries. The screen is divided into three horizontal zones,
 * each scrolling at a different speed to create the illusion of depth.
 *
 * HDMA channel 6 writes a 2-byte scroll value (mode: 2-register) to
 * BG1HOFS at each zone boundary. The scroll table lives in RAM so the
 * main loop can increment each zone's offset at different rates every frame.
 *
 * Ported from PVSnesLib ParallaxScrolling example.
 *
 * @par SNES Concepts
 * - HDMA (H-Blank DMA) for per-scanline register writes
 * - BG horizontal scroll register (BG1HOFS, $210D)
 * - RAM-based HDMA tables for dynamic per-frame updates
 * - Parallax depth illusion via differential scroll speeds
 *
 * @par What to Observe
 * - Three horizontal bands scroll at different speeds automatically
 * - Top (sky): slow, middle: medium, bottom (foreground): fast
 * - No input required; the effect runs continuously
 *
 * @par Modules Used
 * console, dma, background, sprite, hdma, input, math
 *
 * @see hdma.h, background.h, dma.h
 */

#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[];
extern u8 palette[];

/**
 * HDMA scroll table in RAM (bank $00 < $2000).
 *
 * Format: [line_count] [scroll_lo] [scroll_hi] ... [0x00 = end]
 * 3 entries x 3 bytes + 1 terminator = 10 bytes
 */
u8 scroll_table[10];

int main(void) {
    consoleInit();

    /* Load tiles to VRAM at $1000 (word address) */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  16 * 2 * 2,
                  BG_16COLORS, 0x1000);

    /* Load 64x32 tilemap to VRAM at $0000 */
    bgSetMapPtr(0, 0x0000, SC_64x32);
    dmaCopyVram(tilemap, 0x0000, 64 * 32 * 2);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;  /* Enable only BG1 */
    setScreenOn();

    /* Initialize HDMA scroll table:
     * 3 zones with different initial scroll speeds */
    scroll_table[0] = 72;                       /* Top: 72 scanlines */
    *(u16 *)&scroll_table[1] = 1;               /* Initial scroll = 1 */
    scroll_table[3] = 88;                       /* Middle: 88 scanlines */
    *(u16 *)&scroll_table[4] = 2;               /* Initial scroll = 2 */
    scroll_table[6] = 64;                       /* Bottom: 64 scanlines */
    *(u16 *)&scroll_table[7] = 4;               /* Initial scroll = 4 */
    scroll_table[9] = 0x00;                     /* End of HDMA table */

    /* Set up HDMA on channel 6, targeting BG1 (bg=0) horizontal scroll */
    hdmaParallax(HDMA_CHANNEL_6, 0, scroll_table);
    hdmaEnable(1 << HDMA_CHANNEL_6);

    while (1) {
        /* Increment scroll offsets — each zone moves at its own speed */
        *(u16 *)&scroll_table[1] += 1;          /* +1 px/frame (slow) */
        *(u16 *)&scroll_table[4] += 2;          /* +2 px/frame (medium) */
        *(u16 *)&scroll_table[7] += 4;          /* +4 px/frame (fast) */

        WaitForVBlank();
    }

    return 0;
}
