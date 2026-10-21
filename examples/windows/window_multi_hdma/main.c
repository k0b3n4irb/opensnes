/**
 * @file main.c
 * @brief Multi-window HDMA — both windows' edges driven per scanline
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s WindowMultiHDMA demo (PeterLemon/SNES,
 * PPU/Window/WindowMultiHDMA). One HDMA channel in 4-register mode
 * streams WH0-WH3 ($2126-$2129) — BOTH windows' left/right edges —
 * every scanline band, cutting a grid of portholes into BG1: the four
 * windows of an aerial tram, with the city at dusk behind them. The
 * backdrop (CGRAM color 0, the tram's wall) shows wherever the combined
 * window mask disables the layer; the D-pad scrolls the city underneath
 * the static mask.
 *
 * Window algebra (krom's exact setup): W1 and W2 both enabled on BG1
 * and both INVERTED, combined with AND — the layer is masked where
 * (outside W1) AND (outside W2), i.e. visible inside either window.
 *
 * @par SNES Concepts
 * - HDMA_MODE_4REG: one channel drives 4 consecutive PPU registers/line
 * - Two windows animated per scanline (hdmaWindowShape covers only one)
 * - W12SEL / WBGLOG / TMW via windowEnable/windowSetInvert/
 *   windowSetLogic/windowSetMainMask
 * - Backdrop color as the "outside the windows" fill
 *
 * @par What to Observe
 * Four portholes in the tram's dark wall, the city and its reflection in
 * a canal behind them. D-pad scrolls the city behind the fixed portholes.
 *
 * @par Modules Used
 * console, dma, background, hdma, window, input
 *
 * @see https://github.com/PeterLemon/SNES — original demo
 * @see window.h, hdma.h (HDMA_MODE_4REG)
 */

#include <snes.h>

/** @brief The city behind the portholes, 8bpp (devtools/art_assets/build.py) */
#include "res/city.inc"   /* generated: city_tiles[] and city_tiles_1[] (the tiles, split at a bank), city_map[], city_pal[], each with _end */

/** @brief VRAM: the map at word $0000, the tiles at $1000 (546 8bpp tiles,
 * 34 KB, up to $543F: krom's $4000 would run past the end of VRAM). A BG's
 * character base moves in steps of $1000 words (BG12NBA), so $1000 is the
 * first one past the map. */
#define VRAM_MAP 0x0000
#define VRAM_GFX 0x1000

/**
 * @brief krom's exact HDMA window table: 16-line bands alternating
 * "both windows empty" (edges 1..0 = degenerate = fully masked = green)
 * and "two portholes" (W1 16-112, W2 144-240). 5-byte entries:
 * [count][W1L][W1R][W2L][W2R], terminator 0.
 */
static const u8 window_table[] = {
    16,   1,   0,   1,   0,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,   1,   0,   1,   0,
    16,   1,   0,   1,   0,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,  16, 112, 144, 240,
    16,   1,   0,   1,   0,
    0,
};

int main(void) {
    u16 pad;
    u16 sx = 0, sy = 0;

    consoleInit();

    /* krom: BGMODE = %00001011 — Mode 3, priority bit, 8x8 tiles */
    setMode(BG_MODE3, 0x08);

    bgSetGfxPtr(0, VRAM_GFX);
    bgSetMapPtr(0, VRAM_MAP, SC_32x32);

    dmaCopyCGram(city_pal, 0, (u16)(city_pal_end - city_pal));
    dmaCopyVram(city_map, VRAM_MAP, (u16)(city_map_end - city_map));
    /* 34 KB of tiles is more than a LoROM bank: the converter splits them,
     * city_tiles (the first 32 KB) then city_tiles_1, which follows it in VRAM
     * (each byte is a word's half: 32 KB is 0x4000 words). */
    dmaCopyVram(city_tiles, VRAM_GFX, (u16)(city_tiles_end - city_tiles));
    dmaCopyVram(city_tiles_1, VRAM_GFX + 0x4000, (u16)(city_tiles_1_end - city_tiles_1));

    /* Backdrop (CGRAM color 0) = the tram's wall, the picture's colour 0
     * (krom's was green) — what shows outside the windows once TMW masks
     * BG1 there. */

    /* krom's window algebra: both windows on BG1, both inverted,
     * combined with AND, mask applied on the main screen. */
    windowEnable(WINDOW_1, LAYER_BG1);
    windowEnable(WINDOW_2, LAYER_BG1);
    windowSetInvert(WINDOW_1, LAYER_BG1, 1);
    windowSetInvert(WINDOW_2, LAYER_BG1, 1);
    windowSetLogic(LAYER_BG1, WINDOW_LOGIC_AND);
    windowSetMainMask(LAYER_BG1);

    /* One channel, 4 registers per line: WH0-WH3 = both windows' edges */
    hdmaSetup(HDMA_CHANNEL_2, HDMA_MODE_4REG, HDMA_DEST_WH0, window_table);   /* not 0 (dmaCopyVram) nor 7 (the NMI's DMA) */
    hdmaEnableMask(1 << HDMA_CHANNEL_2);

    setMainScreen(LAYER_BG1);
    setScreenOn();

    while (1) {
        WaitForVBlank();

        pad = padHeld(0);
        if (pad & KEY_UP)    sy--;
        if (pad & KEY_DOWN)  sy++;
        if (pad & KEY_LEFT)  sx--;
        if (pad & KEY_RIGHT) sx++;
        bgSetScroll(0, sx, sy);
    }

    return 0;
}
