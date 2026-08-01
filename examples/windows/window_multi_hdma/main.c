/**
 * @file main.c
 * @brief Multi-window HDMA — both windows' edges driven per scanline
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s WindowMultiHDMA demo (PeterLemon/SNES,
 * PPU/Window/WindowMultiHDMA). One HDMA channel in 4-register mode
 * streams WH0-WH3 ($2126-$2129) — BOTH windows' left/right edges —
 * every scanline band, cutting a grid of portholes into BG1. The green
 * backdrop (CGRAM color 0) shows wherever the combined window mask
 * disables the layer; the D-pad scrolls the artwork underneath the
 * static mask.
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
 * A grid of rectangular portholes over a color-ring artwork; green
 * everywhere else. D-pad scrolls the rings behind the fixed portholes.
 *
 * @par Modules Used
 * console, dma, background, hdma, window, input
 *
 * @see https://github.com/PeterLemon/SNES — original demo
 * @see window.h, hdma.h (HDMA_MODE_4REG)
 */

#include <snes.h>

/** @brief 256-color ring artwork (original, procedural) */
extern u8 rings_pic[], rings_pic_end[];
extern u8 rings_map[], rings_map_end[];
extern u8 rings_pal[], rings_pal_end[];

/** @brief krom's VRAM layout: map at word $0000, tiles at word $4000 */
#define VRAM_MAP 0x0000
#define VRAM_GFX 0x4000

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

    dmaCopyCGram(rings_pal, 0, (u16)(rings_pal_end - rings_pal));
    dmaCopyVram(rings_map, VRAM_MAP, (u16)(rings_map_end - rings_map));
    dmaCopyVram(rings_pic, VRAM_GFX, (u16)(rings_pic_end - rings_pic));

    /* Backdrop (CGRAM color 0) = krom's green — what shows outside the
     * windows once TMW masks BG1 there. */
    setColor(0, RGB(0, 31, 0));

    /* krom's window algebra: both windows on BG1, both inverted,
     * combined with AND, mask applied on the main screen. */
    windowEnable(WINDOW_1, WINDOW_BG1);
    windowEnable(WINDOW_2, WINDOW_BG1);
    windowSetInvert(WINDOW_1, WINDOW_BG1, 1);
    windowSetInvert(WINDOW_2, WINDOW_BG1, 1);
    windowSetLogic(WINDOW_BG1, WINDOW_LOGIC_AND);
    windowSetMainMask(WINDOW_BG1);

    /* One channel, 4 registers per line: WH0-WH3 = both windows' edges */
    hdmaSetup(HDMA_CHANNEL_0, HDMA_MODE_4REG, HDMA_DEST_WH0, window_table);
    hdmaEnable(1 << HDMA_CHANNEL_0);

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
