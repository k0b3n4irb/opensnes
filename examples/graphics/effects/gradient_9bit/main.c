/**
 * @file main.c
 * @brief "9-bit" gradient — brightness-dithered backdrop, two HDMA channels
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s RedSpace9BitHDMA demo (PeterLemon/SNES,
 * PPU/HDMA/RedSpace9BitHDMA). The SNES has 5 bits per color channel
 * (32 levels); this classic trick fakes more: channel 0 rewrites the
 * backdrop color every scanline (2-register mode into CGADD/CGDATA)
 * while channel 1 rewrites INIDISP brightness (16 levels) on the SAME
 * line. Red level x brightness, plus krom's line-to-line jitter in both
 * tables, dithers the 224-line gradient into far more perceptual steps
 * than 5 bits allow — "9-bit" color.
 *
 * There is no BG at all (TM = 0): the whole image is the backdrop.
 * INIDISP itself is HDMA-driven — the demo never calls setScreenOn().
 *
 * @par SNES Concepts
 * - Two HDMA channels cooperating on one visual (color + brightness)
 * - HDMA into INIDISP ($2100) — per-scanline master brightness
 * - HDMA_MODE_2REG_2X into CGADD: [addr16][data16] = one palette write/line
 * - Backdrop-only rendering (TM = 0)
 *
 * @par What to Observe
 * A red-to-black vertical gradient noticeably smoother than 32 bands —
 * compare with graphics/effects/hdma_indirect_gradient (pure 5-bit ramp).
 *
 * @par Modules Used
 * console, dma, hdma
 *
 * @see https://github.com/PeterLemon/SNES — original demo & tables
 */

#include <snes.h>

/** @brief krom's exact tables: [1][CGADD=0 word][color word] x224 + term,
 *  and [1][brightness] x224 + term */
extern u8 color_table[], brightness_table[];

int main(void) {
    consoleInit();

    /* Backdrop only — no layer on the main screen (krom: stz TM) */
    setMainScreen(0);

    /* ch0: one palette write per line — CGADD twice (addr word 0) then
     * CGDATA twice (the gradient color word) */
    hdmaSetup(HDMA_CHANNEL_0, HDMA_MODE_2REG_2X, HDMA_DEST_CGADD, color_table);
    /* ch1: master brightness per line — the "extra bits" */
    hdmaSetup(HDMA_CHANNEL_1, HDMA_MODE_1REG, HDMA_DEST_INIDISP, brightness_table);
    hdmaEnable((1 << HDMA_CHANNEL_0) | (1 << HDMA_CHANNEL_1));

    /* Deliberately NO setScreenOn(): INIDISP is owned by channel 1 —
     * the HDMA writes are the screen-on. */

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
