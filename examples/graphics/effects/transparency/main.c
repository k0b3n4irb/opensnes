/**
 * @file main.c
 * @brief Color addition transparency between two background layers
 * @ingroup examples
 *
 * Demonstrates the SNES color math unit by blending two background layers.
 * BG1 (main screen) shows a 4bpp landscape, while BG3 (sub screen) holds
 * 2bpp semi-transparent clouds that scroll horizontally. The PPU adds the
 * sub screen pixel colors to the main screen via registers CGWSEL ($2130)
 * and CGADSUB ($2131), producing a translucent overlay effect.
 *
 * Mode 1 is used with BG3 priority high so the 2bpp cloud layer renders
 * above BG1. Color addition is applied to BG1 and the backdrop, meaning
 * the cloud colors are numerically added to whatever is underneath.
 *
 * Based on PVSnesLib Transparency example by Alekmaul.
 *
 * @par SNES Concepts
 * - Color math: addition mode via CGWSEL/CGADSUB registers
 * - Main screen vs sub screen layer assignment
 * - Mode 1 with BG3 priority high for overlay effects
 * - 4bpp (BG1) and 2bpp (BG3) tile formats in the same mode
 *
 * @par What to Observe
 * - A landscape background with semi-transparent clouds scrolling over it
 * - The clouds blend additively, brightening the landscape beneath them
 * - No input required; clouds scroll automatically
 *
 * @par Modules Used
 * console, sprite, dma, background, colormath
 *
 * @see colormath.h, background.h, video.h
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/video.h>
#include <snes/background.h>
#include <snes/colormath.h>

/**
 * @brief Assembly DMA loader for SUPERFREE graphics data.
 *
 * Handles DMA transfers with correct bank bytes for tile/tilemap/palette data
 * that may be placed in ROM banks beyond $00 by the linker. C code cannot
 * reliably specify bank bytes for SUPERFREE sections, so an assembly routine
 * uses the `:label` syntax to get the linker-resolved bank at link time.
 *
 * Defined in data.asm. Loads BG1 (4bpp landscape) and BG3 (2bpp clouds)
 * tile data, tilemaps, and palettes to their respective VRAM/CGRAM addresses.
 */
extern void loadGraphics(void);

/**
 * @brief Entry point: color math transparency with scrolling clouds over a landscape.
 *
 * Sets up Mode 1 with two background layers:
 * - BG1 (main screen): 4bpp landscape, static
 * - BG3 (sub screen): 2bpp semi-transparent clouds, scrolling horizontally
 *
 * The SNES color math unit adds the sub screen pixel colors to the main screen,
 * producing a translucent cloud overlay. BG3 is given priority high so its
 * tiles render above BG1 in Mode 1's layer ordering.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    /** @brief Horizontal scroll offset for the cloud layer (BG3), incremented each frame */
    short scrX = 0;

    consoleInit();

    /* Force blank (bit 7 of INIDISP) disables rendering, allowing unlimited
     * VRAM/CGRAM write time. Required because the graphics data is too large
     * for a single VBlank DMA transfer (~4KB budget). */
    REG_INIDISP = 0x80;

    /* Load all tile data, tilemaps, and palettes via assembly DMA.
     * This handles bank bytes correctly for SUPERFREE ROM data. */
    loadGraphics();

    /* BG1 screen map at VRAM $2000, 32x32 tile arrangement (SC_32x32).
     * The screen size bits (0-1) are 0x00 = 32x32. The upper bits hold
     * the VRAM base address >> 8. */
    REG_BG1SC = (0x2000 >> 8) | 0x00;
    /* BG1+BG2 character (tile) base at VRAM $0000.
     * Low nibble = BG1 base >> 12, high nibble = BG2 base >> 12. */
    REG_BG12NBA = 0x00;

    /* BG3 screen map at VRAM $2400, 32x32 tiles.
     * BG3 uses 2bpp tiles (Mode 1), consuming half the VRAM per tile vs 4bpp. */
    REG_BG3SC = (0x2400 >> 8) | 0x00;
    /* BG3+BG4 tile base. Low nibble = BG3 base ($1000 >> 12 = 1). */
    REG_BG34NBA = 0x01;

    /* Mode 1 with BG3 priority high. In Mode 1, BG3 is normally 2bpp and
     * drawn behind BG1/BG2. Setting BG3_MODE1_PRIORITY_HIGH promotes BG3's
     * high-priority tiles above all other BG layers, which is essential for
     * the cloud overlay to be visible on top of the landscape. */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    /* Main screen: the layers that form the final displayed image.
     * Both BG1 (landscape) and BG3 (clouds) must be on the main screen
     * for the clouds to be visible. */
    setMainScreen(LAYER_BG1 | LAYER_BG3);

    /* Sub screen: the source for color math blending. BG3 on the sub screen
     * provides the color values that will be added to the main screen pixels.
     * Without this, the color math unit has no source data to blend. */
    setSubScreen(LAYER_BG3);

    /* Configure the color math unit for additive blending:
     * - Source = sub screen (BG3 cloud colors)
     * - Operation = ADD (cloud RGB values added to main screen pixels)
     * - Targets = BG1 + backdrop (both receive the cloud color addition)
     *
     * The backdrop is the "background color" (CGRAM entry 0) visible where
     * no BG tile is drawn. Including it ensures clouds blend even over empty
     * areas. */
    colorMathInit();
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    colorMathSetOp(COLORMATH_ADD);
    colorMathEnable(COLORMATH_BG1 | COLORMATH_BACKDROP);

    setScreenOn();

    while (1) {
        /* Scroll BG3 (clouds) one pixel right each frame.
         * bgSetScroll(bg_index, x, y) — bg_index 2 = BG3 (0-indexed).
         * The scroll value wraps naturally at the tilemap width, creating
         * seamless looping if the tilemap tiles horizontally. */
        scrX++;
        bgSetScroll(2, scrX, 0);

        WaitForVBlank();
    }

    return 0;
}
