/**
 * Transparency — Color addition between two background layers
 *
 * Demonstrates SNES color math by blending clouds (BG3, subscreen)
 * over a landscape (BG1, main screen). The clouds scroll automatically.
 *
 * Mode 1: BG1 = 4bpp (16 colors), BG3 = 2bpp (4 colors)
 * Color addition: subscreen colors are added to BG1 + backdrop.
 *
 * Based on PVSnesLib Transparency example by Alekmaul.
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
