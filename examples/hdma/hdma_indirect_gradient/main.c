/**
 * @file main.c
 * @brief Indirect HDMA — 32-band backdrop gradient from a pointer table
 * @ingroup examples
 *
 * The indirect-HDMA counterpart of gradient_colors: instead of a table
 * carrying the color DATA inline, the table carries [count][pointer]
 * entries and the PPU fetches each band's 4-byte payload through the
 * pointer ($43x7 selects the data bank). The payload blocks live in a
 * separate WRAM array the CPU can edit freely — the classic layout for
 * dynamic per-scanline palettes (the HiColor building block).
 *
 * C port of "SNES Red Space Indirect HDMA Demo" by krom (Peter Lemon),
 * github.com/PeterLemon/SNES, PPU/HDMA/RedSpaceIndirectHDMA — same
 * configuration: no BG layers at all (main screen = 0), the gradient IS
 * the backdrop, rewritten every 7 scanlines through CGRAM color 0.
 * No assets, static image, zero per-frame CPU.
 *
 * @par SNES Concepts
 * - INDIRECT HDMA (DMAP bit 6): table entries are [count][ptr16]; the
 *   pointed-to data lives in the bank programmed in $43x7 — which plain
 *   hdmaSetup never writes; hdmaSetupIndirect() handles both
 * - Non-repeat count semantics: count=7 fetches the payload ONCE and
 *   holds it for 7 scanlines (a repeat count $87 would fetch a fresh
 *   payload every line — that is the HiColor pattern)
 * - HDMA_MODE_2REG_2X to CGADD: bytes stream as $2121,$2121,$2122,$2122
 *   so each 4-byte block is [CGADD][CGADD][color_lo][color_hi] —
 *   rewriting CGRAM color 0 (the backdrop) per band
 * - Extracting a C pointer's bank for $43x7:
 *   (u8)((u32)(void *)data >> 16) (post-A6 far pointers)
 *
 * @par What to Observe
 * - A pure red gradient: brightest red (BGR555 $001F) at the top fading
 *   to black at the bottom, in 32 bands of 7 scanlines
 * - No tiles, no map, no sprites are loaded — everything on screen is
 *   the backdrop color being rewritten mid-frame by the HDMA channel
 *
 * @par Modules Used
 * console, dma, hdma
 *
 * @see hdma.h (hdmaSetupIndirect), examples/hdma/gradient_colors
 */

#include <snes.h>
#include <snes/hdma.h>

/** @brief Gradient bands (krom: 32 bands x 7 scanlines = 224 lines) */
#define NBANDS       32
/** @brief Scanlines per band (non-repeat count in each table entry) */
#define BAND_LINES   7

/**
 * @brief Indirect payload blocks, one per band: [0][0][color_lo][color_hi].
 *
 * In HDMA_MODE_2REG_2X the four bytes stream as $2121,$2121,$2122,$2122:
 * CGADD is set to 0 (twice), then the 16-bit BGR555 color is written to
 * CGDATA — i.e. CGRAM color 0, the backdrop. WRAM (bank $00), so the
 * CPU could animate the colors at any time without touching the table.
 */
static u8 band_data[NBANDS][4];

/** @brief The indirect table: [count][ptr_lo][ptr_hi] per band + terminator */
static u8 itable[NBANDS * 3 + 1];

/**
 * @brief Entry point — static 32-band red gradient, zero per-frame CPU.
 *
 * Faithful to krom's register sequence: DMAP0=%01000011 (indirect |
 * 2REG_2X), BBAD0=$21, A1T0=table, A1B0=$00, DASB0=$00, HDMAEN=ch0,
 * TM=0, screen on. After hdmaEnable() the CPU idles — the PPU rebuilds
 * the gradient from the pointer table on every frame by itself.
 *
 * @return Never returns (infinite loop)
 */
int main(void) {
    u8 i;
    u8 *p = itable;

    consoleInit();

    /* Red ramp $1F..$00 (BGR555: red is bits 0-4, so high byte stays 0) */
    for (i = 0; i < NBANDS; i++) {
        band_data[i][0] = 0;                    /* $2121: CGADD = 0     */
        band_data[i][1] = 0;                    /* $2121 (second write) */
        band_data[i][2] = (u8)(0x1F - i);       /* $2122: color low     */
        band_data[i][3] = 0;                    /* $2122: color high    */
    }

    /* Table: fetch band i's payload once, hold it for 7 scanlines */
    for (i = 0; i < NBANDS; i++) {
        u16 ptr = (u16)(u32)(void *)band_data[i];
        *p++ = BAND_LINES;
        *p++ = (u8)(ptr & 0xFF);
        *p++ = (u8)(ptr >> 8);
    }
    *p = 0;                                     /* end of HDMA table */

    hdmaSetupIndirect(HDMA_CHANNEL_0, HDMA_MODE_2REG_2X, HDMA_DEST_CGADD,
                      itable,
                      (u8)((u32)(void *)band_data >> 16));
    hdmaEnable(1 << HDMA_CHANNEL_0);

    /* krom: TM = 0 — no layers; the backdrop is the whole picture */
    setMainScreen(0);
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
