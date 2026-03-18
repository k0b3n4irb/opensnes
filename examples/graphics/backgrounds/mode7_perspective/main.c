/**
 * @file main.c
 * @brief Mode 7 perspective with HDMA split-screen (F-Zero style)
 * @ingroup examples
 *
 * Creates an F-Zero-style pseudo-3D ground plane by combining two BG modes
 * on a single screen using HDMA mid-frame register switching. The top portion
 * (96 scanlines) displays a Mode 3 sky background, while the bottom portion
 * (128 scanlines) renders a Mode 7 ground plane with per-scanline perspective
 * scaling. Four HDMA channels cooperate each frame: one switches BGMODE from
 * Mode 3 to Mode 7 at the split line, another toggles the main screen layer
 * enable (BG2 for sky, BG1 for ground), and two more write per-scanline M7A
 * and M7D values to create the perspective foreshortening effect where distant
 * rows appear more compressed.
 *
 * @par SNES Concepts
 * - Mid-frame BG mode switching via HDMA (Mode 3 sky + Mode 7 ground)
 * - Per-scanline M7A/M7D writes for perspective projection
 * - HDMA-driven main screen layer enable toggling
 * - Mode 7 ground plane scrolling with D-pad input
 * - Multi-channel HDMA coordination (4 channels synchronized)
 *
 * @par What to Observe
 * - The screen is split: a flat sky image on top and a perspective ground below
 * - Press D-pad to scroll the ground plane in any direction
 * - Distant ground rows appear more compressed (perspective foreshortening)
 *
 * @par Modules Used
 * console, dma, background, sprite, input, mode7
 *
 * @see mode7.h, hdma.h, background.h, input.h
 */

#include <snes.h>

/**
 * @brief Load sky background tiles and tilemap to VRAM via assembly DMA.
 *
 * Loads the Mode 3 sky image (tiles at VRAM $5000, tilemap at VRAM $4000).
 * Uses assembly because the data may span multiple ROM banks (SUPERFREE
 * sections), requiring correct bank byte handling that the C-level
 * dmaCopyVram() cannot provide.
 *
 * Must be called during forced blank.
 */
extern void asm_loadSkyData(void);

/**
 * @brief Build and activate the 4-channel HDMA perspective split.
 *
 * Configures four HDMA channels that cooperate each frame:
 * - Channel A: Switches BGMODE register from Mode 3 (sky) to Mode 7 (ground)
 *   at the split scanline (scanline 96).
 * - Channel B: Toggles TM (main screen enable) from BG2 (sky) to BG1 (ground).
 * - Channel C: Writes per-scanline M7A values for horizontal perspective scaling.
 * - Channel D: Writes per-scanline M7D values for vertical perspective scaling.
 *
 * The perspective tables compress distant rows and expand near rows, creating
 * the illusion of a 3D ground plane receding into the horizon. The scroll
 * offsets (sx, sy) control the camera position on the Mode 7 ground.
 *
 * @param sx Horizontal scroll offset for the Mode 7 ground plane.
 * @param sy Vertical scroll offset for the Mode 7 ground plane.
 */
extern void asm_setupHdmaPerspective(u16 sx, u16 sy);

/** @brief Mode 7 ground tilemap (128x128 entries, one byte per tile index).
 *
 * Mode 7 tilemaps are 128x128 bytes. Each byte is a tile index (0-255)
 * referencing the 8bpp tile set. Loaded to VRAM via dmaCopyVramMode7()
 * which handles the interleaved format (map in low bytes, tiles in high bytes). */
extern u8 ground_map[], ground_map_end[];

/** @brief Mode 7 ground tile pixel data (256 tiles, 64 bytes each = 16KB).
 *
 * Each tile is 8x8 pixels at 8bpp (one byte per pixel, 64 bytes/tile).
 * Written to the high bytes of VRAM words by dmaCopyVramMode7(). */
extern u8 ground_tiles[], ground_tiles_end[];

/** @brief 256-color palette for the ground plane (512 bytes). */
extern u8 ground_pal[], ground_pal_end[];

u16 pad0;           /**< Current joypad button state */
u16 sx;             /**< Mode 7 ground horizontal scroll offset (camera X) */
u16 sy;             /**< Mode 7 ground vertical scroll offset (camera Y) */

/**
 * @brief Entry point -- F-Zero-style Mode 7 perspective with HDMA split-screen.
 *
 * Sets up a dual-mode display: the top 96 scanlines render a Mode 3 sky
 * background (BG2), while the bottom 128 scanlines render a Mode 7 ground
 * plane (BG1) with per-scanline perspective scaling via HDMA. The D-pad
 * scrolls the ground plane. HDMA tables are rebuilt every frame by the
 * assembly helper to reflect the current scroll position.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    consoleInit();

    /* Force blank for VRAM loading -- all VRAM/CGRAM writes must happen
     * during VBlank or forced blank on the SNES. */
    setScreenOff();

    /* Load Mode 7 ground data to VRAM $0000 using the special interleaved
     * DMA function. Mode 7 VRAM format: low byte = tilemap, high byte = pixel.
     * The ground plane is 128x128 tiles at 256 colors (8bpp). */
    dmaCopyVramMode7(ground_map, ground_map_end - ground_map,
                     ground_tiles, ground_tiles_end - ground_tiles);
    dmaCopyCGram(ground_pal, 0, ground_pal_end - ground_pal);

    /* Load sky tiles+map to VRAM $4000/$5000 via assembly DMA loader.
     * The sky is a standard Mode 3 (8bpp) background displayed on BG2. */
    asm_loadSkyData();

    /* Configure BG2 for sky display (used during Mode 3 portion).
     * REG_BG2SC: bits 7-2 = tilemap base ($4000 >> 10 = 0x10, shifted left 2 = 0x40),
     * bits 1-0 = size (01 = 64x32 tiles). REG_BG12NBA: high nybble = BG2 tiles
     * at $5000, low nybble = BG1 tiles at $0000. */
    REG_BG2SC = 0x41;    /* ($4000 / $400) << 2 | 1 (64x32) */
    REG_BG12NBA = 0x50;  /* BG1 tiles at $0000, BG2 tiles at $5000 */

    /* Start in Mode 7 -- the HDMA tables will switch to Mode 3 for the sky
     * portion of the screen. Mode 7 is needed as the base because it governs
     * the M7A-M7D matrix registers used for perspective. */
    setMode(BG_MODE7, 0);
    mode7Init();

    /* Initialize scroll position (top-left corner of ground plane) */
    sx = 0;
    sy = 0;

    /* Build the initial HDMA perspective tables and write Mode 7 scroll
     * registers. This must be done before setScreenOn() so the first
     * visible frame has correct perspective geometry. */
    asm_setupHdmaPerspective(sx, sy);

    /* Display on */
    setScreenOn();

    while (1) {
        pad0 = padHeld(0);

        if (pad0 & KEY_LEFT)  sx--;
        if (pad0 & KEY_RIGHT) sx++;
        if (pad0 & KEY_UP)    sy--;
        if (pad0 & KEY_DOWN)  sy++;

        /* Rebuild HDMA perspective tables with updated scroll position.
         * The assembly helper writes M7HOFS/M7VOFS for the new camera position
         * and reconstructs the per-scanline M7A/M7D tables. HDMA channels are
         * re-enabled each frame because the NMI handler may have consumed them. */
        asm_setupHdmaPerspective(sx, sy);

        WaitForVBlank();
    }

    return 0;
}
