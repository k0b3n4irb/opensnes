/**
 * @file main.c
 * @brief Direct color — the pixel byte IS the color (CGWSEL bit 0)
 * @ingroup examples
 *
 * Effects arc 4/7 (#109). In direct color mode the PPU stops looking
 * 8bpp BG pixels up in CGRAM: each pixel byte is read as BBGGGRRR and
 * expanded straight to 15-bit BGR. This example builds a 16x16 chart
 * of all 256 pixel codes procedurally (zero assets, 256 solid-color
 * tiles generated at init) and loads a deliberately DIFFERENT CGRAM
 * palette — a grayscale ramp — so one button press flips the SAME
 * VRAM bytes between two readings:
 *
 * - direct ON  (boot state): the BBGGGRRR color cube — red ramps
 *   left-to-right within each block row, green down the chart, blue
 *   in four broad bands;
 * - direct OFF: the CGRAM grayscale ramp of the very same tile data.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - CGWSEL bit 0 via colorMathSetDirectColor() — the colormath module
 *   shadows the register, so this composes with any blending setup
 * - BBGGGRRR expansion and the tilemap-palette-bits low-bit extension
 *   (2048 colors max — this chart uses pal bits 0, the base 256)
 * - Mode 3: BG1 is 8bpp; 8bpp tile format = 4 interleaved bitplane
 *   pairs (64 bytes/tile), built here byte by byte in C
 *
 * @par What to Observe
 * Boot shows the color cube. Press A to re-read the same VRAM as
 * grayscale (CGRAM mode), press again to return. Nothing is uploaded
 * on toggle — one register bit changes the meaning of every pixel.
 *
 * @par Modules Used
 * console, dma, background, colormath, input
 *
 * @see lib/include/snes/colormath.h — colorMathSetDirectColor()
 */

#include <snes.h>
#include <snes/colormath.h>
#include <snes/input.h>

/** @brief Probe oracle: 1 while direct color is active */
u8 direct_on;

/** @brief One 8bpp tile (4 interleaved bitplane pairs, 64 bytes) */
static u8 tilebuf[64];

/** @brief One tilemap row (32 entries) */
static u16 rowbuf[32];

/** @brief Grayscale CGRAM ramp (256 x 15-bit) */
static u16 graybuf[256];

/**
 * @brief Fill tilebuf with a solid 8bpp tile of pixel value v
 *
 * 8bpp planar layout: rows store plane pairs 0-1, then 2-3, 4-5, 6-7
 * in 16-byte groups. A solid tile makes every row byte of plane p
 * either $FF or $00 depending on bit p of v.
 */
static void build_solid_tile(u8 v) {
    u8 row, pair;
    u16 o = 0;
    for (pair = 0; pair < 8; pair += 2) {
        for (row = 0; row < 8; row++) {
            tilebuf[o++] = (u8)((v & (1 << pair)) ? 0xFF : 0x00);
            tilebuf[o++] = (u8)((v & (1 << (pair + 1))) ? 0xFF : 0x00);
        }
    }
}

int main(void) {
    u16 i, row, col;
    u16 keys;

    consoleInit();

    /* 256 solid tiles at VRAM $0000 (64 bytes = 32 words each) */
    for (i = 0; i < 256; i++) {
        build_solid_tile((u8)i);
        dmaCopyVram(tilebuf, (u16)(i * 32), 64);
    }

    /* Tilemap at $3C00: a 16x16 chart of tile ids 0..255, centered
     * (rows 6-21, columns 8-23); the border stays tile 0 (code 0 =
     * black in both readings). Palette bits 0 -> base 256 codes. */
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 32; col++) {
            u16 t = 0;
            if (row >= 6 && row < 22 && col >= 8 && col < 24) {
                t = (u16)((row - 6) * 16 + (col - 8));
            }
            rowbuf[col] = t;
        }
        dmaCopyVram((u8 *)rowbuf, (u16)(0x3C00 + row * 32), 64);
    }

    /* CGRAM: a grayscale ramp — deliberately NOT the direct-color
     * expansion, so toggling modes visibly re-reads the same pixels */
    for (i = 0; i < 256; i++) {
        u16 g = (u16)(i >> 3);                  /* 0..31 */
        graybuf[i] = (u16)(g | (g << 5) | (g << 10));
    }
    dmaCopyCGram((u8 *)graybuf, 0, 512);

    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3C00, SC_32x32);
    setMode(BG_MODE3, 0);
    setMainScreen(LAYER_BG1);

    colorMathSetDirectColor(1);                 /* the feature: boot in direct mode */
    direct_on = 1;

    setScreenOn();

    while (1) {
        WaitForVBlank();
        keys = padPressed(0);
        if (keys & KEY_A) {
            direct_on = (u8)!direct_on;
            colorMathSetDirectColor(direct_on);
        }
    }

    return 0;
}
