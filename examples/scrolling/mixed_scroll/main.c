/**
 * @file main.c
 * @brief Mixed scrolling: three layers, three scrolls — a diagonal pattern, a fixed logo, a moving message
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
 * A third layer, BG3 (2bpp), carries a line of text from the text module,
 * scrolled horizontally at its own pace: a scrolling message is a scrolled
 * background like any other (it was the scroll_message example).
 *
 * @par SNES Concepts
 * - Independent per-layer scroll registers (BG1 scrolls, BG2 stays fixed)
 * - Multiple tile sets sharing VRAM without overlap
 * - Separate palette slots per layer (`palette-entry` in a picture's settings file)
 * - Mode 1 dual-layer compositing (BG1 behind BG2)
 * - BG3 in Mode 1: 2bpp, its palettes in CGRAM 0-31 (shared with the 4bpp
 *   layers' — the text takes 2bpp slot 5, colours 20-23, which BG1 leaves
 *   free), above the others with BG3_MODE1_PRIORITY_HIGH and the tile
 *   priority bit
 * - The text module on a layer of your choice: textLoadFont / textInit
 *
 * @par What to Observe
 * - The shader pattern (BG1) scrolls diagonally and wraps seamlessly
 * - The logo (BG2) remains stationary in the center
 * - Both layers composite together with proper priority ordering
 * - The message (BG3) slides left one pixel a frame, in front of both
 *
 * @par Modules Used
 * console, sprite, dma, background, text
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>
#include <snes/text.h>
#include "vram_map.h"  /* generated VRAM bases (devtools/vram_layout) */

/** @brief BG1 tile data -- repeating shader pattern (4bpp, tiles at VRAM $4000). */
#include "res/opensnes.inc"   /* generated: opensnes_tiles/_map/_pal, each with _end (BG2) */
#include "res/shader.inc"     /* generated: shader_tiles/_map/_pal (BG1) */
/** @brief BG1 palette (16 colors loaded to CGRAM slot 1, offset 16). */
/** @brief BG1 tilemap (32x32 tile grid loaded to VRAM $1800). */

/** @brief BG2 tile data -- static logo (4bpp, tiles at VRAM $5000). */
/** @brief BG2 palette (16 colors loaded to CGRAM slot 0, offset 0). */
/** @brief BG2 tilemap (32x32 tile grid loaded to VRAM $1400). */

/** @brief BG1 horizontal scroll offset (auto-incremented each frame). */
s16 scrX = 0;
/** @brief BG1 vertical scroll offset (auto-incremented each frame). */
s16 scrY = 0;
/** @brief The message's horizontal scroll (BG3) */
u16 msgX = 0;

/**
 * @brief Entry point -- mixed scrolling with auto-scrolling BG1 and fixed BG2.
 *
 * Loads two Mode 1 background layers into non-overlapping VRAM regions:
 * - BG1: shader pattern (tiles at $4000, tilemap at $1800, palette slot 1)
 * - BG2: static logo (tiles at $5000, tilemap at $1400, palette slot 0)
 *
 * Each frame, BG1's scroll position is incremented diagonally by 1 pixel in
 * both X and Y, causing the shader pattern to scroll continuously. BG2 has
 * no scroll updates, so it remains fixed on screen. The SNES PPU composites
 * both layers together, with BG2 (the logo) appearing on top of BG1 due to
 * default priority ordering.
 *
 * @note The tile, map and palette symbols come from the generated
 *       res/opensnes.inc and res/shader.inc (opensnes-tileset writes them
 *       from the settings files beside the pictures).
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    consoleInit();

    /* Load BG2 tiles + palette (static logo).
     * Palette slot 0 = CGRAM offset 0 (colors 0-15).
     * Tiles at VRAM $5000 to avoid overlapping BG1 tiles at $4000. */
    bgInitTileSet(1, opensnes_tiles, opensnes_pal, 0,
                  opensnes_tiles_end - opensnes_tiles,
                  opensnes_pal_end - opensnes_pal,
                  BG_16COLORS, VRAM_BG2_TILES);

    /* Load BG1 tiles + palette (shader pattern).
     * Palette slot 1 = CGRAM offset 16 (colors 16-31).
     * Each layer needs its own palette slot to avoid color conflicts. */
    bgInitTileSet(0, shader_tiles, shader_pal, 1,
                  shader_tiles_end - shader_tiles,
                  shader_pal_end - shader_pal,
                  BG_16COLORS, VRAM_BG1_TILES);

    /* Set tilemap locations -- tilemaps must not overlap each other
     * or the tile data regions. Each SC_32x32 tilemap is 2KB (0x400 words). */
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);

    /* Copy tilemaps to VRAM -- must happen during VBlank or forced blank
     * because the PPU silently ignores VRAM writes during active display. */
    WaitForVBlank();
    dmaCopyVram(opensnes_map, VRAM_BG2_MAP, opensnes_map_end - opensnes_map);
    dmaCopyVram(shader_map, VRAM_BG1_MAP, shader_map_end - shader_map);

    /* Mode 1, enable only BG1 + BG2 (BG3 disabled).
     * TM_BG1 | TM_BG2 enables both layers on the main screen. */
    /* BG3: the text module's 2bpp font and its tilemap, palette slot 5 */
    textLoadFont(VRAM_BG3_FONT);
    textInit(VRAM_BG3_MAP, 0, 5);
    text_config.priority = 1;            /* in front of BG1 and BG2 */
    setColor(21, RGB(31, 31, 31));       /* slot 5, colour 1: white */
    bgSetGfxPtr(2, VRAM_BG3_FONT);
    bgSetMapPtr(2, VRAM_BG3_MAP, SC_32x32);
    textPrintAt(2, 24, "OPENSNES -- TEXT THAT MOVES!");

    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    setMainScreen(TM_BG1 | TM_BG2 | TM_BG3);

    setScreenOn();

    while (1) {
        /* Increment both scroll axes by 1 pixel per frame, creating
         * diagonal motion. The tilemap wraps naturally at 256 pixels
         * (32 tiles x 8 pixels), producing a seamless repeating effect. */
        scrX++;
        scrY++;
        bgSetScroll(0, scrX, scrY);
        msgX++;
        bgSetScroll(2, msgX, 0);         /* the message slides left */

        WaitForVBlank();
    }
    return 0;
}
