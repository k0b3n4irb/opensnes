/**
 * @file main.c
 * @brief Colour, rung 6c.2 — shadow & tint: recolour a whole scene at once
 * @ingroup examples
 *
 * Colour math against a *fixed* colour is how a game changes mood without
 * touching a single tile or palette entry. Subtract a grey and the scene
 * darkens — dusk, a dungeon, nightfall. Add a colour and the scene takes
 * that cast — the blue of being underwater, the orange of a sunset or lava
 * glow, the red flash of taking a hit. One register pair does it to every
 * pixel of a layer, every frame, for free.
 *
 * The scene here is a 4x4 chart of sixteen game-world colours (sky, water,
 * foliage, earth, stone, accents) built procedurally — zero assets — so
 * you can watch the effect land on every hue at once. Press A to cycle:
 *
 *   none -> shadow (darken) -> underwater tint -> sunset tint -> none
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Colour math with the FIXED-colour source (COLDATA $2132): result is
 *   main-screen ± a constant, applied per enabled layer
 * - colorMathShadow() = subtract grey (darken); colorMathTint() = add a
 *   colour (cast) — the two share the fixed-colour register and the
 *   add/subtract bit, so only one is active at a time
 * - The effect is a PPU pass over the final image: no VRAM, CGRAM or OAM
 *   traffic, so it costs nothing per frame and scales to the whole screen
 *
 * @par What to Observe
 * Boot: the plain colour chart. Press A to darken it (shadow), then wash
 * it blue (underwater), then warm it (sunset), then back to plain. Every
 * swatch shifts together. `fx_state` is the probe oracle (0..3).
 *
 * @par Modules Used
 * console, dma, background, colormath, input
 *
 * @see lib/include/snes/colormath.h — colorMathShadow(), colorMathTint()
 */

#include <snes.h>
#include <snes/colormath.h>
#include <snes/input.h>

/** @brief Probe oracle: current effect (0 none, 1 shadow, 2 underwater, 3 sunset) */
u8 fx_state;

/** @brief 16 game-world colours, filled at init (RAM — no ROM const) */
static u16 scene_pal[16];

/** @brief One solid 4bpp tile (two interleaved bitplane pairs, 32 bytes) */
static u8 tilebuf[32];

/** @brief One tilemap row (32 entries) */
static u16 rowbuf[32];

/**
 * @brief Fill scene_pal[] with a believable slice of a game world
 *
 * Four rows of four: sky/water, foliage, earth/stone, accents. Vivid
 * enough that both a darken and a colour cast read clearly on every one.
 */
static void build_scene_palette(void) {
    scene_pal[0]  = RGB(12, 20, 31);  /* sky blue    */
    scene_pal[1]  = RGB( 6, 28, 31);  /* cyan        */
    scene_pal[2]  = RGB( 2,  6, 24);  /* deep blue   */
    scene_pal[3]  = RGB( 4, 22, 20);  /* teal water  */

    scene_pal[4]  = RGB(10, 26,  6);  /* grass green */
    scene_pal[5]  = RGB( 4, 16,  4);  /* dark green  */
    scene_pal[6]  = RGB(16, 18,  4);  /* olive       */
    scene_pal[7]  = RGB(14,  8,  2);  /* brown       */

    scene_pal[8]  = RGB(30, 26, 16);  /* sand        */
    scene_pal[9]  = RGB(24, 18, 12);  /* tan         */
    scene_pal[10] = RGB(18, 18, 18);  /* stone grey  */
    scene_pal[11] = RGB( 8,  8, 10);  /* dark stone  */

    scene_pal[12] = RGB(30,  4,  4);  /* red         */
    scene_pal[13] = RGB(31, 16,  2);  /* orange      */
    scene_pal[14] = RGB(31, 30,  6);  /* yellow      */
    scene_pal[15] = RGB(20,  4, 24);  /* purple      */
}

/**
 * @brief Fill tilebuf with a solid 4bpp tile of palette index v
 *
 * 4bpp planar: rows 0-7 store bitplane pair (0,1) as 16 bytes, then pair
 * (2,3). Each plane byte is $FF or $00 per the matching bit of v.
 */
static void build_solid_tile(u8 v) {
    u8 row, pair;
    u16 o = 0;
    for (pair = 0; pair < 4; pair += 2) {
        for (row = 0; row < 8; row++) {
            tilebuf[o++] = (u8)((v & (1 << pair))       ? 0xFF : 0x00);
            tilebuf[o++] = (u8)((v & (1 << (pair + 1))) ? 0xFF : 0x00);
        }
    }
}

/** @brief Apply the effect for the current fx_state (call after VBlank) */
static void apply_effect(void) {
    switch (fx_state) {
        case 1: colorMathShadow(COLORMATH_BG1, 14);       break; /* darken   */
        case 2: colorMathTint(COLORMATH_BG1, 0, 4, 18);   break; /* underwater */
        case 3: colorMathTint(COLORMATH_BG1, 20, 8, 0);   break; /* sunset   */
        default: colorMathDisable();                      break; /* none     */
    }
}

int main(void) {
    u16 row, col;
    u8  i;

    consoleInit();
    colorMathInit();

    /* 16 solid tiles at VRAM $0000 (32 bytes = 16 words each) */
    for (i = 0; i < 16; i++) {
        build_solid_tile(i);
        dmaCopyVram(tilebuf, (u16)(i * 16), 32);
    }

    /* Tilemap at $3C00: a 4x4 grid of big swatches (each ~8x7 tiles). */
    for (row = 0; row < 32; row++) {
        u16 band = (u16)((row / 7) * 4);
        for (col = 0; col < 32; col++) {
            u16 sw = (u16)(band + (col / 8));
            if (sw > 15) sw = 15;
            rowbuf[col] = sw;
        }
        dmaCopyVram((u8 *)rowbuf, (u16)(0x3C00 + row * 32), 64);
    }

    build_scene_palette();
    dmaCopyCGram((u8 *)scene_pal, 0, 32);

    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3C00, SC_32x32);
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1);

    fx_state = 0;               /* boot: plain scene, no colour math */
    apply_effect();

    setScreenOn();

    while (1) {
        WaitForVBlank();

        if (padPressed(0) & KEY_A) {
            fx_state = (u8)((fx_state + 1) & 3);
            apply_effect();
        }
    }

    return 0;
}
