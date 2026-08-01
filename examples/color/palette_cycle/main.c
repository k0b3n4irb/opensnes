/**
 * @file main.c
 * @brief Colour, rung 6b — palette cycling: animate without touching a pixel
 * @ingroup examples
 *
 * The oldest trick in the 2D book. The picture on screen never changes:
 * every tile, every tilemap entry, every scroll register is frozen after
 * init. All that moves is CGRAM — each frame we rotate the 16 palette
 * entries by one slot, and the diagonal bands appear to flow. This is how
 * the SNES animates waterfalls, lava, fire, marquee lights and "loading"
 * shimmers for the cost of a 32-byte CGRAM reload, no VRAM traffic at all.
 *
 * The scene is built procedurally — zero assets. Sixteen solid 4bpp tiles
 * (tile i is a flat fill of palette index i) are generated in C at init,
 * and the tilemap is laid out as `tile = (row + col) & 15` so colour index
 * runs along every diagonal. Cycling the palette then slides those
 * diagonals across the screen.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - CGRAM is a live lookup table: rewrite an entry and every pixel that
 *   indexes it changes colour this frame — no redraw
 * - setColor() / dmaCopyCGram() are VBlank-only (CGRAM port is ignored
 *   during active display) — the rotation happens right after WaitForVBlank
 * - 4bpp planar tile format (32 bytes/tile, two interleaved bitplane
 *   pairs), built byte by byte in C
 * - A seamless colour loop: the 16 entries wrap (entry 15 → entry 0 is
 *   continuous) so the cycle never jumps
 *
 * @par What to Observe
 * Boot: a rainbow of diagonal bands drifting steadily up-and-left. Press
 * START to freeze the cycle — the bands stop dead, proving nothing on
 * screen actually moved; press again to resume. `cycle_pos` counts every
 * rotation (probe oracle).
 *
 * @par Modules Used
 * console, dma, background, input
 *
 * @see lib/include/snes/video.h — setColor(), RGB()
 * @see lib/include/snes/dma.h — dmaCopyCGram()
 */

#include <snes.h>
#include <snes/input.h>

/** @brief Number of palette entries we cycle (one 4bpp palette) */
#define WHEEL_LEN 16

/** @brief Frames between rotations — a calm ~15 steps/second */
#define FRAMES_PER_STEP 4

/** @brief Probe oracle: 1 while the cycle is running, 0 when frozen */
u8 cycling;

/** @brief Probe oracle: total palette rotations since boot */
u16 cycle_pos;

/** @brief The 16 live colours, rotated in place each step (RAM master copy) */
static u16 wheel[WHEEL_LEN];

/** @brief One 4bpp tile (two interleaved bitplane pairs, 32 bytes) */
static u8 tilebuf[32];

/** @brief One tilemap row (32 entries) */
static u16 rowbuf[32];

/**
 * @brief Fill wheel[] with a seamless 16-step rainbow (HSV, S=V=max)
 *
 * A six-segment hue walk over a 192-unit wheel, sampled every 12 units.
 * Full saturation and value keep it vivid; because the walk closes the
 * loop, entry 15 flows back into entry 0 with no visible seam when cycled.
 */
static void build_rainbow(void) {
    u8 i;
    for (i = 0; i < WHEEL_LEN; i++) {
        u16 hue = (u16)(i * 12);        /* 0..180 across a 0..191 wheel */
        u8 seg  = (u8)(hue / 32);       /* which of the 6 colour segments */
        u8 up   = (u8)(hue % 32);       /* 0..31 rising ramp             */
        u8 dn   = (u8)(31 - up);        /* 0..31 falling ramp            */
        u8 r = 0, g = 0, b = 0;
        switch (seg) {
            case 0: r = 31; g = up; b = 0;  break;   /* red   -> yellow  */
            case 1: r = dn; g = 31; b = 0;  break;   /* yellow-> green   */
            case 2: r = 0;  g = 31; b = up; break;   /* green -> cyan    */
            case 3: r = 0;  g = dn; b = 31; break;   /* cyan  -> blue    */
            case 4: r = up; g = 0;  b = 31; break;   /* blue  -> magenta */
            default:r = 31; g = 0;  b = dn; break;   /* magenta-> red    */
        }
        wheel[i] = RGB(r, g, b);
    }
}

/**
 * @brief Fill tilebuf with a solid 4bpp tile of palette index v
 *
 * 4bpp planar layout: rows 0-7 store bitplane pair (0,1) as 16 bytes,
 * then rows 0-7 store pair (2,3). A solid tile makes every row byte of
 * plane p either $FF or $00 depending on bit p of v.
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

int main(void) {
    u16 row, col;
    u8  i, frame = 0;

    consoleInit();

    /* 16 solid tiles at VRAM $0000 (32 bytes = 16 words each) */
    for (i = 0; i < WHEEL_LEN; i++) {
        build_solid_tile(i);
        dmaCopyVram(tilebuf, (u16)(i * 16), 32);
    }

    /* Tilemap at $3C00: tile id = (row + col) & 15 → colour index runs
     * along each diagonal, so cycling the palette slides the bands. */
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 32; col++) {
            rowbuf[col] = (u16)((row + col) & 15);
        }
        dmaCopyVram((u8 *)rowbuf, (u16)(0x3C00 + row * 32), 64);
    }

    /* Seed the live palette and push it to CGRAM 0..15 */
    build_rainbow();
    dmaCopyCGram((u8 *)wheel, 0, WHEEL_LEN * 2);

    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3C00, SC_32x32);
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1);

    cycling   = 1;
    cycle_pos = 0;

    setScreenOn();

    while (1) {
        WaitForVBlank();

        if (padPressed(0) & KEY_START) {
            cycling = (u8)!cycling;
        }

        if (cycling && (++frame >= FRAMES_PER_STEP)) {
            u16 first;
            frame = 0;

            /* Rotate the live palette up by one, wrapping the top back
             * to the bottom, then reload all 16 CGRAM entries at once. */
            first = wheel[0];
            for (i = 0; i < WHEEL_LEN - 1; i++) {
                wheel[i] = wheel[i + 1];
            }
            wheel[WHEEL_LEN - 1] = first;

            dmaCopyCGram((u8 *)wheel, 0, WHEEL_LEN * 2);
            cycle_pos++;
        }
    }

    return 0;
}
