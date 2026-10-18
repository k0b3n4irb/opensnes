/**
 * @file main.c
 * @brief Backgrounds — offset-per-tile in Mode 2 and Mode 4: per-column scroll from BG3
 * @ingroup examples
 *
 * Modes 2, 4 and 6 have a trick no other mode does: **offset-per-tile
 * (OPT)**. BG3 stops being a drawable layer and becomes an *offset table* —
 * for each 8-pixel column of BG1/BG2 the PPU reads a scroll offset out of
 * BG3's tilemap. That lets each column scroll independently, which is how
 * you get a flag rippling in the wind, heat-haze, or per-column parallax
 * without a scanline of HDMA.
 *
 * The example boots in Mode 2: BG1 is a stack of horizontal colour bands
 * (4bpp); a sine table written into BG3 gives every column its own vertical
 * offset, so the bands ripple. B switches to Mode 4: BG1 becomes a diagonal
 * 256-colour gradient (8bpp), and the same wave goes into Mode 4's single
 * offset row — as vertical words, or as horizontal ones after A, when the
 * columns slide sideways in 8-pixel steps. B again goes back to Mode 2.
 * Both scenes are built in VRAM at boot; a switch is the mode, two BG1
 * pointers and the palette.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Mode 2 OPT: `setMode(BG_MODE2, 0)`; BG3's tilemap is the offset source,
 *   not a picture
 * - Mode 2: per column N≥1 the PPU reads BG3 tile-row 0 for the H offset
 *   word and tile-row 1 for the V offset word; column 0 cannot be offset
 * - Mode 4: ONE row; bit 15 of a word says vertical (1) or horizontal (0)
 * - Offset word: bit 0x2000 = apply to BG1 (0x4000 = BG2); V value in bits
 *   0-9, H value in bits 3-9 (whole tiles)
 * - Mode 4's BG1 is 8bpp (256 colours), Mode 2's is 4bpp
 *
 * @par What to Observe
 * Horizontal colour bands ripple as a travelling sine wave — each column at
 * a different vertical offset, all from one small BG3 table. B: the
 * 256-colour diagonals of Mode 4 ripple the same way; A there: they slide
 * sideways instead. `wave_phase` is the probe oracle.
 *
 * @par Modules Used
 * console, dma, background, input, math, tile
 *
 * @see lib/include/snes/video.h — setMode(BG_MODE2), setMode(BG_MODE4)
 */

#include <snes.h>
#include <snes/tile.h>   /* tileEncode4bpp, tileEncode8bpp */
#include <snes/math.h>

#define BG1_CHR  0x0000   /* Mode 2 scene: 16 4bpp tiles */
#define BG1_MAP  0x2000
#define BG1_CHR4 0x1000   /* Mode 4 scene: 32 8bpp tiles */
#define BG1_MAP4 0x2400
#define BG3_MAP  0x3000   /* BG3 = the OPT offset table, both modes */

#define OPT_VERTICAL 0x8000    /**< Mode 4: 1 = vertical offset, 0 = horizontal */
#define OPT_BG1      0x2000    /**< apply this offset word to BG1 */

/** @brief Probe oracle: wave phase, advances each frame. */
u16 wave_phase;
/** @brief 1 while the Mode 4 scene is shown. */
u8 in_mode4;
/** @brief Mode 4 only: 1 = the wave goes in as vertical words. */
u8 opt_vertical;

static u8  px[64];
static u8  tilebuf[64];
static u16 vwords[32];    /* Mode 2: per-column V-offset words for BG3 tile-row 1 */
static u16 owords[32];    /* Mode 4: the single offset row */
static u16 pal16[16];
static u16 pal256[256];
static u16 maprow[32];

/** @brief A hue around the colour wheel, for the Mode 4 gradient. */
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

/** @brief The Mode 2 scene: 16-colour rainbow, solid tiles, horizontal bands. */
static void buildMode2Scene(void) {
    u16 i, row, col;
    for (i = 0; i < 16; i++) {
        u16 h = (u16)(i * 12);
        u8 seg = (u8)(h / 32), up = (u8)(h % 32), dn = (u8)(31 - up);
        u8 r = 0, g = 0, b = 0;
        switch (seg) {
            case 0: r = 31; g = up; break;  case 1: r = dn; g = 31; break;
            case 2: g = 31; b = up; break;  case 3: g = dn; b = 31; break;
            case 4: r = up; b = 31; break;  default: r = 31; b = dn; break;
        }
        pal16[i] = RGB(r, g, b);
    }
    for (i = 0; i < 16; i++) {
        u8 j; for (j = 0; j < 64; j++) px[j] = (u8)i;
        tileEncode4bpp(px, tilebuf);
        dmaCopyVram(tilebuf, (u16)(BG1_CHR + i * 16), 32);
    }
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 32; col++) maprow[col] = (u16)(row & 15);
        dmaCopyVram((u8 *)maprow, (u16)(BG1_MAP + row * 32), 64);
    }
}

/** @brief The Mode 4 scene: 256 colours, 32 tiles of 8 lines each, diagonals. */
static void buildMode4Scene(void) {
    u16 i, row, col;
    for (i = 0; i < 256; i++)
        pal256[i] = hue(i);          /* colour 0 is the backdrop, never drawn */
    for (i = 0; i < 32; i++) {       /* tile t: 8 lines of colours 8t .. 8t+7 */
        u8 r, c;
        for (r = 0; r < 8; r++)
            for (c = 0; c < 8; c++) {
                u8 v = (u8)(i * 8 + r);
                px[r * 8 + c] = v ? v : 1;
            }
        tileEncode8bpp(px, tilebuf);
        dmaCopyVram(tilebuf, (u16)(BG1_CHR4 + i * 32), 64);
    }
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 32; col++)
            maprow[col] = (u16)((row + col) & 31);
        dmaCopyVram((u8 *)maprow, (u16)(BG1_MAP4 + row * 32), 64);
    }
}

/** @brief Show one scene: mode, BG1's two pointers, the palette, BG3's row 0. */
static void showScene(u8 mode4) {
    u16 i;
    for (i = 0; i < 32; i++) maprow[i] = 0;
    if (mode4) {
        setMode(BG_MODE4, 0);
        bgSetGfxPtr(0, BG1_CHR4);
        bgSetMapPtr(0, BG1_MAP4, SC_32x32);
        dmaCopyCGram((u8 *)pal256, 0, 512);
    } else {
        setMode(BG_MODE2, 0);
        bgSetGfxPtr(0, BG1_CHR);
        bgSetMapPtr(0, BG1_MAP, SC_32x32);
        dmaCopyCGram((u8 *)pal16, 0, 32);
        dmaCopyVram((u8 *)maprow, BG3_MAP, 64);   /* row 0 = H offsets, all off */
    }
    in_mode4 = mode4;
}

int main(void) {
    u16 i;

    consoleInit();

    buildMode2Scene();
    buildMode4Scene();

    bgSetMapPtr(2, BG3_MAP, SC_32x32);   /* BG3 = OPT source */
    bgSetScroll(2, 0, 0);
    showScene(0);
    setMainScreen(LAYER_BG1);

    opt_vertical = 1;
    wave_phase = 0;
    setScreenOn();

    while (1) {
        /* Build the next frame's offset table during ACTIVE display, so the
         * VRAM upload can fire the instant VBlank starts. Computing it after
         * WaitForVBlank would push the DMA past VBlank's end and the PPU
         * would silently drop the write. */
        for (i = 0; i < 32; i++) {
            u8 ang = (u8)(i * 8 + wave_phase);
            if (!in_mode4) {
                s16 off = (s16)(64 + ((fixSin(ang) * 48) >> 8));   /* 16..112 */
                vwords[i] = (u16)(OPT_BG1 | ((u16)off & 0x3FF));
            } else {
                s16 off = (s16)(32 + ((fixSin(ang) * 24) >> 8));   /* 8..56 */
                if (opt_vertical)
                    owords[i] = (u16)(OPT_VERTICAL | OPT_BG1 | ((u16)off & 0x3FF));
                else
                    owords[i] = (u16)(OPT_BG1 | ((u16)off & 0x3F8));
            }
        }
        wave_phase = (u16)(wave_phase + 2);
        WaitForVBlank();
        if (!in_mode4)
            dmaCopyVram((u8 *)vwords, (u16)(BG3_MAP + 32), 64);    /* row 1 = V */
        else
            dmaCopyVram((u8 *)owords, BG3_MAP, 64);                /* the only row */

        if (padPressed(0) & KEY_B)
            showScene(in_mode4 ^ 1);
        if (in_mode4 && (padPressed(0) & KEY_A))
            opt_vertical ^= 1;
    }
    return 0;
}
