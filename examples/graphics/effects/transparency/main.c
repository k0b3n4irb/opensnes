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

/* Assembly loader — handles DMA with correct bank bytes for SUPERFREE data. */
extern void loadGraphics(void);

int main(void) {
    short scrX = 0;

    consoleInit();

    /* Force blank for VRAM/CGRAM writes */
    REG_INIDISP = 0x80;

    /* Load all graphics via assembly DMA (correct bank bytes) */
    loadGraphics();

    /* BG1: tilemap at $2000, tiles at $0000 (4bpp landscape) */
    REG_BG1SC = (0x2000 >> 8) | 0x00;   /* SC_32x32 */
    REG_BG12NBA = 0x00;                  /* BG1 tiles at $0000 */

    /* BG3: tilemap at $2400, tiles at $1000 (2bpp clouds) */
    REG_BG3SC = (0x2400 >> 8) | 0x00;   /* SC_32x32 */
    REG_BG34NBA = 0x01;                  /* BG3 tiles at $1000 */

    /* Mode 1, BG3 priority high */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    /* Main screen: BG1 + BG3 only (disable BG2) */
    setMainScreen(LAYER_BG1 | LAYER_BG3);

    /* Subscreen: BG3 (clouds provide color data for blending) */
    setSubScreen(LAYER_BG3);

    /* Color addition: blend subscreen (clouds) into BG1 + backdrop */
    colorMathInit();
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    colorMathSetOp(COLORMATH_ADD);
    colorMathEnable(COLORMATH_BG1 | COLORMATH_BACKDROP);

    setScreenOn();

    while (1) {
        /* Scroll clouds horizontally */
        scrX++;
        bgSetScroll(2, scrX, 0);

        WaitForVBlank();
    }

    return 0;
}
