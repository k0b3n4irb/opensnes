/**
 * @file main.c
 * @brief Backgrounds — Mode 6: one hi-res layer with offset-per-tile
 * @ingroup examples
 *
 * Mode 6 is the hi-res member of the offset-per-tile family: a single
 * 4bpp BG1 drawn 512 half-pixels wide, like Mode 5, and BG3 as the offset
 * table, like Mode 2 — one row of horizontal offset words, one row of
 * vertical ones. Like Mode 5, its tiles are always 16 half-pixels wide
 * (character N on the left, N+1 on the right), and that applies to BG3's
 * table too: each offset word moves a 16-half-pixel column, so 32 words
 * still cover the screen (anomie-regs, *Backgrounds / Mode 6*).
 *
 * BG1 here is diagonal colour bands drawn in one-half-pixel stripes, a
 * pattern 256-pixel modes cannot show. A sine wave written into the
 * vertical row makes the columns ripple; press A and the wave moves to the
 * horizontal row, and the columns slide sideways instead.
 *
 * @par SNES Concepts
 * - Mode 6: `setMode(BG_MODE6, 0)`, BG1 4bpp in hi-res, BG3 = offset map
 * - Hi-res: the sub screen draws the even half-pixel columns and the main
 *   screen the odd ones, so BG1 goes on both (`setMainScreen` and
 *   `setSubScreen`), as in Mode 5
 * - 16-half-pixel tiles: a map entry for character N draws N and N + 1
 * - The offset table as in Mode 2: BG3 row 0 = horizontal words, row 1 =
 *   vertical words; bit 13 = apply to BG1; column 0 is never offset
 *
 * @par What to Observe
 * - Diagonal colour bands with fine vertical stripes ripple up and down as a
 *   travelling wave; the leftmost column stays put.
 * - Press A: the columns slide sideways instead. A again: back.
 *
 * @par Modules Used
 * console, dma, background, input, math, tile
 *
 * @see backgrounds/mode2 (the same table, 256 pixels wide),
 *      backgrounds/mode5_hires (hi-res without the table)
 */

#include <snes.h>
#include <snes/tile.h>   /* tileEncode4bpp */
#include <snes/math.h>

/** @brief VRAM word addresses: BG1 chars, BG1 map, BG3 offset map */
#define BG1_CHR 0x0000
#define BG1_MAP 0x2000
#define BG3_MAP 0x3000

/** @brief Offset word bit: apply this word to BG1 */
#define OPT_BG1 0x2000

/** @brief Number of colour bands, one 16x8 tile each */
#define BANDS 8

/** @brief Probe oracles: wave phase, 1 while the wave is vertical */
u16 wave_phase;
u8 opt_vertical;

/** @brief One 4bpp character's pixels and its encoding */
static u8 px[64];
static u8 tilebuf[32];
/** @brief The offset words of the row that carries the wave */
static u16 owords[32];
/** @brief A row of zero words, for the row that does not */
static u16 zwords[32];
/** @brief One tilemap row being built */
static u16 maprow[32];

/** @brief Band colours (palette 0, colours 1..8) and the stripe colour 9 */
static const u16 band_colours[BANDS + 1] = {
    RGB(31, 0, 0), RGB(31, 16, 0), RGB(31, 31, 0), RGB(0, 31, 0),
    RGB(0, 31, 31), RGB(0, 12, 31), RGB(16, 0, 31), RGB(31, 0, 24),
    RGB(4, 4, 8),
};

int main(void) {
    u16 i, row, col;

    consoleInit();
    setMode(BG_MODE6, 0);

    dmaCopyCGram((u8 *)band_colours, 1, sizeof(band_colours));

    /* Band b is characters 2b (left half) and 2b + 1 (right half), both
     * the same: colour b + 1 on even half-pixel columns, the dark stripe
     * colour on odd ones. One-half-pixel stripes only exist in hi-res. */
    for (i = 0; i < BANDS; i++) {
        u8 r, c;
        for (r = 0; r < 8; r++)
            for (c = 0; c < 8; c++)
                px[r * 8 + c] = (c & 1) ? (u8)(BANDS + 1) : (u8)(i + 1);
        tileEncode4bpp(px, tilebuf);
        dmaCopyVram(tilebuf, (u16)(BG1_CHR + (i * 2) * 16), 32);
        dmaCopyVram(tilebuf, (u16)(BG1_CHR + (i * 2 + 1) * 16), 32);
    }
    /* BG1 map: diagonal bands, tile = band (row + col) mod 8, so a
     * column shows its offset whichever way it moves */
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 32; col++)
            maprow[col] = (u16)(((row + col) % BANDS) * 2);
        dmaCopyVram((u8 *)maprow, (u16)(BG1_MAP + row * 32), 64);
    }

    bgSetGfxPtr(0, BG1_CHR);
    bgSetMapPtr(0, BG1_MAP, SC_32x32);
    bgSetMapPtr(2, BG3_MAP, SC_32x32);   /* BG3 = the offset map */
    bgSetScroll(2, 0, 0);

    /* Hi-res: the sub screen draws the even half-pixel columns */
    setMainScreen(LAYER_BG1);
    setSubScreen(LAYER_BG1);

    for (i = 0; i < 32; i++)
        zwords[i] = 0;
    dmaCopyVram((u8 *)zwords, BG3_MAP, 64);          /* row 0: H, none */

    opt_vertical = 1;
    wave_phase = 0;
    setScreenOn();

    while (1) {
        /* Build next frame's words during active display, so the upload
         * fits at the very start of VBlank (see backgrounds/mode2). */
        for (i = 0; i < 32; i++) {
            u8 ang = (u8)(i * 8 + wave_phase);
            s16 off = (s16)(32 + ((fixSin(ang) * 24) >> 8));    /* 8..56 */
            owords[i] = (u16)(OPT_BG1 | ((u16)off & 0x3FF));
        }
        wave_phase = (u16)(wave_phase + 2);

        WaitForVBlank();
        if (opt_vertical) {
            dmaCopyVram((u8 *)owords, (u16)(BG3_MAP + 32), 64);  /* row 1: V */
        } else {
            dmaCopyVram((u8 *)owords, BG3_MAP, 64);              /* row 0: H */
        }
        if (padPressed(0) & KEY_A) {
            opt_vertical ^= 1;
            /* the row the wave leaves goes back to zero next frame */
            dmaCopyVram((u8 *)zwords,
                        opt_vertical ? BG3_MAP : (u16)(BG3_MAP + 32), 64);
        }
    }
    return 0;
}
