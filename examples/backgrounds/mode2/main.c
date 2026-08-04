/**
 * @file main.c
 * @brief Backgrounds — Mode 2 offset-per-tile: per-column scroll from BG3
 * @ingroup examples
 *
 * Modes 2, 4 and 6 have a trick no other mode does: **offset-per-tile
 * (OPT)**. BG3 stops being a drawable layer and becomes an *offset table* —
 * for each 8-pixel column of BG1/BG2 the PPU reads a scroll offset out of
 * BG3's tilemap. That lets each column scroll independently, which is how
 * you get a flag rippling in the wind, heat-haze, or per-column parallax
 * without a scanline of HDMA.
 *
 * Here BG1 is a stack of horizontal colour bands; a sine table written into
 * BG3 gives every column its own vertical offset, so the bands ripple. The
 * phase advances each frame. Only BG1 is shown — BG3 is pure offset data.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Mode 2 OPT: `setMode(BG_MODE2, 0)`; BG3's tilemap is the offset source,
 *   not a picture
 * - Per column N≥1 the PPU reads BG3 tile-row 0 for the H offset word and
 *   tile-row 1 for the V offset word; column 0 cannot be offset
 * - Offset word: bit 0x2000 = apply to BG1 (0x4000 = BG2); V value in bits
 *   0-9, H value in bits 3-9. This example writes only V words
 * - Modes 4 and 6 reuse this exact data path (mode 4 packs H/V into one word)
 *
 * @par What to Observe
 * Horizontal colour bands ripple as a travelling sine wave — each column at
 * a different vertical offset, all from one small BG3 table. `wave_phase` is
 * the probe oracle.
 *
 * @par Modules Used
 * console, dma, background, math
 *
 * @see lib/include/snes/video.h — setMode(BG_MODE2)
 */

#include <snes.h>
#include <snes/math.h>

#define BG1_CHR 0x0000
#define BG1_MAP 0x2000
#define BG3_MAP 0x3000    /* BG3 = the OPT offset table */

#define OPT_BG1 0x2000    /* apply this offset word to BG1 */

/** @brief Probe oracle: wave phase, advances each frame. */
u16 wave_phase;

static u8  px[64];
static u8  tilebuf[32];
static u16 vwords[32];    /* per-column V-offset words for BG3 tile-row 1 */

static void encode_4bpp(void) {
    u8 pair, row, col; u16 o = 0;
    for (pair = 0; pair < 4; pair += 2)
        for (row = 0; row < 8; row++) {
            u8 lo = 0, hi = 0;
            for (col = 0; col < 8; col++) {
                u8 v = px[row * 8 + col];
                if (v & (1 << pair))       lo |= (u8)(0x80 >> col);
                if (v & (1 << (pair + 1))) hi |= (u8)(0x80 >> col);
            }
            tilebuf[o++] = lo; tilebuf[o++] = hi;
        }
}

int main(void) {
    u16 pal[16];
    u16 i, row, col;

    consoleInit();

    /* 16-colour vertical rainbow → BG1 palette 0 */
    for (i = 0; i < 16; i++) {
        u16 hue = (u16)(i * 12);
        u8 seg = (u8)(hue / 32), up = (u8)(hue % 32), dn = (u8)(31 - up);
        u8 r = 0, g = 0, b = 0;
        switch (seg) {
            case 0: r = 31; g = up; break;  case 1: r = dn; g = 31; break;
            case 2: g = 31; b = up; break;  case 3: g = dn; b = 31; break;
            case 4: r = up; b = 31; break;  default: r = 31; b = dn; break;
        }
        pal[i] = RGB(r, g, b);
    }
    dmaCopyCGram((u8 *)pal, 0, 32);

    /* 16 solid tiles; BG1 map = horizontal bands (tile = row & 15) */
    for (i = 0; i < 16; i++) {
        u8 j; for (j = 0; j < 64; j++) px[j] = (u8)i;
        encode_4bpp();
        dmaCopyVram(tilebuf, (u16)(BG1_CHR + i * 16), 32);
    }
    for (row = 0; row < 32; row++) {
        u16 rb[32];
        for (col = 0; col < 32; col++) rb[col] = (u16)(row & 15);
        dmaCopyVram((u8 *)rb, (u16)(BG1_MAP + row * 32), 64);
    }

    /* BG3 offset table: zero tile-row 0 (no H offsets). Row 1 (V) is
     * rewritten every frame. */
    {
        u16 zero[32];
        for (i = 0; i < 32; i++) zero[i] = 0;
        dmaCopyVram((u8 *)zero, BG3_MAP, 64);          /* row 0 = H, all off */
    }

    bgSetGfxPtr(0, BG1_CHR);
    bgSetMapPtr(0, BG1_MAP, SC_32x32);
    bgSetMapPtr(2, BG3_MAP, SC_32x32);   /* BG3 = OPT source */
    bgSetScroll(2, 0, 0);
    setMode(BG_MODE2, 0);
    setMainScreen(LAYER_BG1);

    wave_phase = 0;
    setScreenOn();

    while (1) {
        /* Build the next frame's offset table during ACTIVE display, so the
         * VRAM upload can fire the instant VBlank starts. Computing it after
         * WaitForVBlank would push the DMA past VBlank's end and the PPU
         * would silently drop the write. */
        for (i = 0; i < 32; i++) {
            u8 ang = (u8)(i * 8 + wave_phase);
            s16 off = (s16)(64 + ((fixSin(ang) * 48) >> 8));   /* 16..112 */
            vwords[i] = (u16)(OPT_BG1 | ((u16)off & 0x3FF));
        }
        wave_phase = (u16)(wave_phase + 2);

        WaitForVBlank();
        dmaCopyVram((u8 *)vwords, (u16)(BG3_MAP + 32), 64);    /* row 1 = V */
    }
    return 0;
}
