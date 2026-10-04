/**
 * @file main.c
 * @brief Pseudo-hires: a 50 % blend of two layers without colour math
 * @ingroup examples
 *
 * SETINI bit 3 (pseudo-hires) makes the PPU output 512 pixels per line in
 * any BG mode, taking every other one from the sub screen: the sub screen
 * fills the even columns, the main screen the odd ones. A CRT on composite
 * blurs the two neighbours together, so what you see is a half-and-half
 * blend — the effect some games used for transparency, with colour math
 * left free for something else.
 *
 * Here BG1 (red horizontal bars) is on the main screen and BG2 (blue
 * vertical bars, scrolling) on the sub screen. Colour math is off: without
 * pseudo-hires the sub screen is not shown at all. Press A to toggle it.
 *
 * @par SNES Concepts
 * - SETINI ($2133) bit 3 through `videoSetPseudoHires()` (the lib keeps a
 *   shadow of the write-only register)
 * - Main screen / sub screen designation (`setMainScreen`, `setSubScreen`)
 *   without colour math: the sub screen is only seen through pseudo-hires
 * - The interleave: sub screen on even columns, main screen on odd ones
 * - Tiles built at run time (`tileEncode4bpp`), no asset file
 *
 * @par What to Observe
 * - Purple where the bars cross, half-bright red and blue elsewhere (the
 *   emulator's 256-pixel view averages each pixel pair, like a CRT).
 * - Press A: pseudo-hires off, only the red bars remain; press A again.
 * - With an emulator's native 512-pixel output (luna `--native-res`), the
 *   alternating columns are visible one by one.
 *
 * @par Modules Used
 * console (videoSetPseudoHires), dma, background, input, tile
 *
 * @see video.h (videoSetPseudoHires), docs/tutorials/colormath.md
 */

#include <snes.h>
#include <snes/tile.h>   /* tileEncode4bpp */

/** @brief VRAM word addresses: shared char block, the two maps */
#define VRAM_CHR     0x0000
#define VRAM_BG1_MAP 0x2000
#define VRAM_BG2_MAP 0x2400

/** @brief Tilemap entry bits: palette 1 (CGRAM 16-31) */
#define MAP_PAL1     0x0400

/** @brief Probe oracle: 1 while pseudo-hires is on */
u8 ph_on;
/** @brief Probe oracle: BG2's horizontal scroll */
u16 bg2_x;

/** @brief One tile's pixels and its 4bpp encoding */
static u8 px[64];
static u8 tilebuf[32];
/** @brief One tilemap row being built */
static u16 maprow[32];

/** @brief BG palettes 0 and 1: transparent + red, transparent + blue */
static const u16 palettes[32] = {
    RGB(0, 0, 0), RGB(31, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    RGB(0, 0, 0), RGB(6, 10, 31), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int main(void) {
    u16 row, col, i;

    consoleInit();
    setMode(BG_MODE1, 0);
    dmaCopyCGram((u8 *)palettes, 0, 64);

    /* tile 0 transparent, tile 1 solid colour 1 */
    for (i = 0; i < 64; i++)
        px[i] = 0;
    tileEncode4bpp(px, tilebuf);
    dmaCopyVram(tilebuf, VRAM_CHR, 32);
    for (i = 0; i < 64; i++)
        px[i] = 1;
    tileEncode4bpp(px, tilebuf);
    dmaCopyVram(tilebuf, VRAM_CHR + 16, 32);

    /* BG1: red horizontal bars, 16 lines on, 16 off */
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 32; col++)
            maprow[col] = (row & 2) ? 0 : 1;
        dmaCopyVram((u8 *)maprow, VRAM_BG1_MAP + row * 32, 64);
    }
    /* BG2: blue vertical bars, 16 pixels on, 16 off, palette 1 */
    for (col = 0; col < 32; col++)
        maprow[col] = (col & 2) ? 0 : (1 | MAP_PAL1);
    for (row = 0; row < 32; row++)
        dmaCopyVram((u8 *)maprow, VRAM_BG2_MAP + row * 32, 64);

    bgSetGfxPtr(0, VRAM_CHR);
    bgSetGfxPtr(1, VRAM_CHR);
    bgSetMapPtr(0, VRAM_BG1_MAP, BG_MAP_32x32);
    bgSetMapPtr(1, VRAM_BG2_MAP, BG_MAP_32x32);

    /* BG1 on the main screen, BG2 on the sub screen; no colour math, so
     * the sub screen reaches the output only through pseudo-hires */
    setMainScreen(LAYER_BG1);
    setSubScreen(LAYER_BG2);
    videoSetPseudoHires(1);
    ph_on = 1;
    setScreenOn();

    while (1) {
        WaitForVBlank();
        if (padPressed(0) & KEY_A) {
            ph_on ^= 1;
            videoSetPseudoHires(ph_on);
        }
        bg2_x++;
        bgSetScroll(1, bg2_x, 0);
    }
    return 0;
}
