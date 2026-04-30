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

/** @brief 4bpp background tile data (defined in data.asm, stored in ROM) */
extern u8 tiles[], tiles_end[];
/** @brief 64x32 tilemap data for the parallax background */
extern u8 tilemap[];
/** @brief 16-color palette for the background */
extern u8 palette[];

/**
 * @brief HDMA scroll table in RAM (must be in bank $00 WRAM, < $2000).
 *
 * This table defines three horizontal screen zones, each with an independent
 * scroll offset. HDMA channel 6 reads this table during HBlank to override
 * the BG1 horizontal scroll register (BG1HOFS, $210D) at zone boundaries.
 *
 * Table format (mode: 2-register write to $210D/$210E):
 *   [line_count] [scroll_lo] [scroll_hi]  -- zone entry (3 bytes)
 *   ...repeated for each zone...
 *   [0x00]                                -- end-of-table marker
 *
 * The table lives in RAM (not ROM) because scroll values are updated every
 * frame by the main loop. HDMA re-reads the table each frame automatically.
 *
 * Total: 3 zones x 3 bytes + 1 terminator = 10 bytes.
 */
u8 scroll_table[10];

/**
 * @brief Entry point: set up parallax scrolling with three speed zones.
 *
 * Loads a 64x32 tile background, configures HDMA to split the screen into
 * three horizontal bands, and updates each band's scroll offset at a
 * different rate every frame to create a parallax depth illusion.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    consoleInit();

    /* Load 4bpp tiles to VRAM word address $1000, palette to CGRAM slot 0.
     * 2 * PALETTE_16_SIZE = 64 bytes = 2 palettes. */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  2 * PALETTE_16_SIZE,
                  BG_16COLORS, 0x1000);

    /* Load 64x32 tilemap to VRAM $0000.
     * SC_64x32 means two 32x32 screen pages side-by-side (4096 bytes total).
     * This provides a 512-pixel-wide virtual background for smooth scrolling. */
    bgSetMapPtr(0, 0x0000, SC_64x32);
    dmaCopyVram(tilemap, 0x0000, 64 * 32 * 2);

    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1);  /**< Enable only BG1 on the main screen */
    setScreenOn();

    /* Build the initial HDMA scroll table with three zones.
     * Each entry: [scanline_count] [scroll_lo] [scroll_hi].
     * The u16 cast writes both scroll bytes at once (little-endian). */
    scroll_table[0] = 72;                       /**< Zone 1 (sky): 72 scanlines */
    *(u16 *)&scroll_table[1] = 1;               /**< Initial horizontal scroll = 1 pixel */
    scroll_table[3] = 88;                       /**< Zone 2 (hills): 88 scanlines */
    *(u16 *)&scroll_table[4] = 2;               /**< Initial horizontal scroll = 2 pixels */
    scroll_table[6] = 64;                       /**< Zone 3 (ground): 64 scanlines */
    *(u16 *)&scroll_table[7] = 4;               /**< Initial horizontal scroll = 4 pixels */
    scroll_table[9] = 0x00;                     /**< End-of-table marker */

    /* Configure HDMA channel 6 to write the scroll table to BG1HOFS ($210D).
     * hdmaParallax() sets up a 2-register HDMA that writes scroll_lo to $210D
     * and scroll_hi to $210E at each zone boundary during HBlank. */
    hdmaParallax(HDMA_CHANNEL_6, 0, scroll_table);
    hdmaEnable(1 << HDMA_CHANNEL_6);

    while (1) {
        /* Each frame, advance each zone's scroll offset by a different amount.
         * Slow zones (sky) create the illusion of being far away, while fast
         * zones (ground) appear close — this is the parallax depth effect.
         * HDMA automatically picks up the new values next frame. */
        *(u16 *)&scroll_table[1] += 1;          /**< Sky: +1 pixel/frame (slow) */
        *(u16 *)&scroll_table[4] += 2;          /**< Hills: +2 pixels/frame (medium) */
        *(u16 *)&scroll_table[7] += 4;          /**< Ground: +4 pixels/frame (fast) */

        WaitForVBlank();
    }

    return 0;
}
