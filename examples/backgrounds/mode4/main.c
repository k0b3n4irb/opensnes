/**
 * @file main.c
 * @brief Backgrounds — Mode 4: a 256-colour layer with offset-per-tile
 * @ingroup examples
 *
 * Mode 4 is the cartridge-game mode of offset-per-tile: an 8bpp (256
 * colour) BG1, a 2bpp BG2, and BG3 as an offset table like Mode 2. The
 * difference is in that table: Mode 4 reads only **one** row of it, so
 * each column gets one offset word that is either horizontal or vertical,
 * chosen by bit 15 of the word.
 *
 * Here BG1 is a diagonal 256-colour gradient. A sine wave is written into
 * the single offset row each frame: as vertical words, the columns ripple
 * up and down; press A and the same wave is written as horizontal words,
 * and the columns slide sideways instead (8 pixels at a time — the low 3
 * bits of a horizontal offset are ignored).
 *
 * @par SNES Concepts
 * - Mode 4: `setMode(BG_MODE4, 0)`, BG1 8bpp + BG2 2bpp, BG3 = offset map
 * - The Mode 4 offset word: bit 15 = 1 vertical / 0 horizontal, bit 13 =
 *   apply to BG1 (bit 14 = BG2), value in bits 0-9; one row of 32 words
 * - Column 0 is never offset: word 0 applies to the second visible column
 * - 8bpp tiles built at run time (`tileEncode8bpp`), a 256-entry palette
 *
 * @par What to Observe
 * - The gradient's columns ripple vertically as a travelling wave; the
 *   leftmost column stays put.
 * - Press A: the columns now shift horizontally, in 8-pixel steps.
 *
 * @par Modules Used
 * console, dma, background, input, math, tile
 *
 * @see backgrounds/mode2 (the two-row table), docs/tutorials/graphics.md
 */

#include <snes.h>
#include <snes/tile.h>   /* tileEncode8bpp */
#include <snes/math.h>

/** @brief VRAM word addresses: BG1 8bpp chars, BG1 map, BG3 offset map */
#define BG1_CHR 0x0000
#define BG1_MAP 0x2000
#define BG3_MAP 0x3000

/** @brief Mode 4 offset word bits */
#define OPT_VERTICAL 0x8000    /**< 1 = vertical offset, 0 = horizontal */
#define OPT_BG1      0x2000    /**< apply this word to BG1 */

/** @brief Probe oracles: wave phase, 1 while the words are vertical */
u16 wave_phase;
u8 opt_vertical;

/** @brief One 8bpp tile's pixels and its encoding */
static u8 px[64];
static u8 tilebuf[64];
/** @brief The 256-colour palette, built at boot */
static u16 pal[256];
/** @brief The single offset row, rebuilt every frame */
static u16 owords[32];
/** @brief One tilemap row being built */
static u16 maprow[32];

/** @brief Colour i of a 256-step hue ramp, 32 steps per sextant pair */
static u16 hue(u16 i) {
    u8 seg = (u8)(i / 43), up = (u8)((i % 43) * 31 / 42), dn = (u8)(31 - up);
    switch (seg) {
        case 0:  return RGB(31, up, 0);
        case 1:  return RGB(dn, 31, 0);
        case 2:  return RGB(0, 31, up);
        case 3:  return RGB(0, dn, 31);
        case 4:  return RGB(up, 0, 31);
        default: return RGB(31, 0, dn);
    }
}

int main(void) {
    u16 i, row, col;

    consoleInit();
    setMode(BG_MODE4, 0);

    /* 256 colours; colour 0 is the backdrop, never drawn by these tiles */
    for (i = 0; i < 256; i++)
        pal[i] = hue(i);
    dmaCopyCGram((u8 *)pal, 0, 512);

    /* 32 tiles: tile t is 8 horizontal lines of colours 8t .. 8t+7
     * (colour 0 replaced by 1, so nothing is transparent) */
    for (i = 0; i < 32; i++) {
        u8 r, c;
        for (r = 0; r < 8; r++)
            for (c = 0; c < 8; c++) {
                u8 v = (u8)(i * 8 + r);
                px[r * 8 + c] = v ? v : 1;
            }
        tileEncode8bpp(px, tilebuf);
        dmaCopyVram(tilebuf, (u16)(BG1_CHR + i * 32), 64);
    }
    /* BG1 map: diagonal bands, tile = (row + col) mod 32 */
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 32; col++)
            maprow[col] = (u16)((row + col) & 31);
        dmaCopyVram((u8 *)maprow, (u16)(BG1_MAP + row * 32), 64);
    }

    bgSetGfxPtr(0, BG1_CHR);
    bgSetMapPtr(0, BG1_MAP, SC_32x32);
    bgSetMapPtr(2, BG3_MAP, SC_32x32);   /* BG3 = the offset map */
    bgSetScroll(2, 0, 0);
    setMainScreen(LAYER_BG1);

    opt_vertical = 1;
    wave_phase = 0;
    setScreenOn();

    while (1) {
        /* Build next frame's row during active display, so the upload
         * fits at the very start of VBlank (see backgrounds/mode2). */
        for (i = 0; i < 32; i++) {
            u8 ang = (u8)(i * 8 + wave_phase);
            s16 off = (s16)(32 + ((fixSin(ang) * 24) >> 8));    /* 8..56 */
            if (opt_vertical)
                owords[i] = (u16)(OPT_VERTICAL | OPT_BG1 | ((u16)off & 0x3FF));
            else
                owords[i] = (u16)(OPT_BG1 | ((u16)off & 0x3F8));
        }
        wave_phase = (u16)(wave_phase + 2);

        WaitForVBlank();
        dmaCopyVram((u8 *)owords, BG3_MAP, 64);    /* row 0: the only row */
        if (padPressed(0) & KEY_A)
            opt_vertical ^= 1;
    }
    return 0;
}
