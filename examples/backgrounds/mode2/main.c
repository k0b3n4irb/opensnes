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
 * The example boots in Mode 2 on the Rue basse under the rain (4bpp, the
 * picture of backgrounds/mode1): a sine table written into BG3 gives every
 * column its own vertical offset, a few lines up or down, and the air
 * shimmers over the street like the heat above a steam vent. B switches to
 * Mode 4 and the canal at night (8bpp, 256 colours, the picture of
 * backgrounds/mode3), with the same wave in Mode 4's single offset row — as
 * vertical words, or as horizontal ones after A, when the columns slide
 * sideways in 8-pixel steps. B again goes back to Mode 2. The two pictures
 * do not fit in VRAM together (28 KB and 50 KB), so a switch reloads the
 * scene in force blank: one dark frame.
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
 * - An offset word replaces the layer's scroll register for its column, so
 *   it is the raw value: the -1 the lib adds to a vertical scroll is not
 *   there, and a word of 0x3FF (-1) lines a column up with column 0
 * - Mode 4's BG1 is 8bpp (256 colours), Mode 2's is 4bpp
 *
 * @par What to Observe
 * The street shimmers as a travelling wave, each 8-pixel column a few lines
 * higher or lower, all from one small BG3 table; column 0, at the left
 * edge, stays still. B: the canal in 256 colours ripples the same way; A
 * there: its columns slide sideways instead, by whole tiles.
 * `wave_phase` is the probe oracle.
 *
 * @par Modules Used
 * console, dma, background, input, math
 *
 * @see lib/include/snes/video.h — setMode(BG_MODE2), setMode(BG_MODE4)
 */

#include <snes.h>
#include <snes/math.h>

#include "res/street.inc"   /* generated: street_tiles[], street_pal[], street_map[], each with _end */
#include "res/canal.inc"    /* generated: canal_tiles[] + canal_tiles_1[] (two bank-sized parts), canal_pal[], canal_map[] */

#define SCENE_CHR   0x0000   /* BG1's tiles, either scene: the street's 883, or the canal's first 32 KB */
#define SCENE_CHR_1 0x4000   /* the canal's second part */
#define BG1_MAP     0x7000
#define BG3_MAP     0x7400   /* BG3 = the OPT offset table, both modes */

#define OPT_VERTICAL 0x8000    /**< Mode 4: 1 = vertical offset, 0 = horizontal */
#define OPT_BG1      0x2000    /**< apply this offset word to BG1 */

/** @brief Probe oracle: wave phase, advances each frame. */
u16 wave_phase;
/** @brief 1 while the Mode 4 scene is shown. */
u8 in_mode4;
/** @brief Mode 4 only: 1 = the wave goes in as vertical words. */
u8 opt_vertical;

static u16 vwords[32];    /* Mode 2: per-column V-offset words for BG3 tile-row 1 */
static u16 owords[32];    /* Mode 4: the single offset row */
static u16 zeros[32];     /* Mode 2: BG3 tile-row 0, no H offsets */

/** @brief Load one scene in force blank: its tiles, map and palette, the mode, BG1's pointers. */
static void loadScene(u8 mode4) {
    setScreenOff();
    if (mode4) {
        dmaCopyVram(canal_tiles,   SCENE_CHR,   canal_tiles_end - canal_tiles);
        dmaCopyVram(canal_tiles_1, SCENE_CHR_1, canal_tiles_1_end - canal_tiles_1);
        dmaCopyVram(canal_map, BG1_MAP, canal_map_end - canal_map);
        dmaCopyCGram(canal_pal, 0, canal_pal_end - canal_pal);
        setMode(BG_MODE4, 0);
    } else {
        dmaCopyVram(street_tiles, SCENE_CHR, street_tiles_end - street_tiles);
        dmaCopyVram(street_map, BG1_MAP, street_map_end - street_map);
        dmaCopyCGram(street_pal, 0, street_pal_end - street_pal);
        dmaCopyVram((u8 *)zeros, BG3_MAP, 64);    /* row 0 = H offsets, all off */
        setMode(BG_MODE2, 0);
    }
    bgSetGfxPtr(0, SCENE_CHR);
    bgSetMapPtr(0, BG1_MAP, SC_32x32);
    in_mode4 = mode4;
    setScreenOn();
}

int main(void) {
    u16 i;

    consoleInit();

    bgSetMapPtr(2, BG3_MAP, SC_32x32);   /* BG3 = OPT source */
    bgSetScroll(2, 0, 0);
    setMainScreen(LAYER_BG1);
    opt_vertical = 1;
    wave_phase = 0;
    loadScene(0);

    while (1) {
        /* Build the next frame's offset table during ACTIVE display, so the
         * VRAM upload can fire the instant VBlank starts. Computing it after
         * WaitForVBlank would push the DMA past VBlank's end and the PPU
         * would silently drop the write. */
        for (i = 0; i < 32; i++) {
            u8 ang = (u8)(i * 16 + wave_phase);
            if (opt_vertical || !in_mode4) {
                /* -1..3 lines: -1 is column 0's place (see SNES Concepts) */
                s16 off = (s16)(1 + ((fixSin(ang) * 2) >> 8));
                vwords[i] = (u16)(OPT_BG1 | ((u16)off & 0x3FF));
                owords[i] = (u16)(OPT_VERTICAL | vwords[i]);
            } else {
                s16 off = (s16)(12 + ((fixSin(ang) * 12) >> 8));  /* 0..24 px */
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
            loadScene(in_mode4 ^ 1);
        if (in_mode4 && (padPressed(0) & KEY_A))
            opt_vertical ^= 1;
    }
    return 0;
}
