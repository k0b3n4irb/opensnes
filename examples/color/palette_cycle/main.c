/**
 * @file main.c
 * @brief Colour — palette cycling: animate without touching a pixel
 * @ingroup examples
 *
 * The oldest trick in the 2D book. The picture on screen never changes:
 * every tile, every tilemap entry, every scroll register is frozen after
 * init. All that moves is CGRAM — every four frames we rotate eight
 * palette entries by one slot, and the light runs along a neon sign while
 * the coolant flows through a pipe. This is how
 * the SNES animates waterfalls, lava, fire, marquee lights and "loading"
 * shimmers for the cost of a 32-byte CGRAM reload, no VRAM traffic at all.
 *
 * The scene is a noodle bar's sign and a coolant pipe on a brick wall
 * (res/sign.png, drawn by devtools/art_assets/build.py). It was drawn for
 * the cycle: each letter of NOODLES is painted with its own colour index,
 * 8 to 14 of BG palette 0, and the coolant with indices 8 to 15 of BG
 * palette 1, which follow one another along the pipe. The wall, the frame
 * and the pipe's metal use the other indices, and never change. Rotating
 * entries 8-15 of the two palettes sends a wave of light along the letters
 * and makes the coolant run.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - CGRAM is a live lookup table: rewrite an entry and every pixel that
 *   indexes it changes colour this frame — no redraw
 * - setColor() / dmaCopyCGram() are VBlank-only (CGRAM port is ignored
 *   during active display) — the rotation happens right after WaitForVBlank
 * - Artwork drawn for cycling: the indices that rotate are kept apart
 *   from the ones that do not, and laid out in the order the light runs
 * - A seamless colour loop: the 8 entries wrap (entry 15 → entry 8 is
 *   continuous) so the cycle never jumps
 *
 * @par What to Observe
 * Boot: the sign's letters light up one after another, and the coolant
 * runs along the pipe. Press START to freeze the cycle — both stop dead,
 * proving nothing on screen actually moved; press again to resume. `cycle_pos` counts every
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

/** @brief Number of palette entries we cycle in each palette: 8 to 15 */
#define WHEEL_LEN 8
/** @brief The first cycled entry of a palette */
#define WHEEL_FIRST 8

/** @brief Frames between rotations — a calm ~15 steps/second */
#define FRAMES_PER_STEP 4

/** @brief Probe oracle: 1 while the cycle is running, 0 when frozen */
u8 cycling;

/** @brief Probe oracle: total palette rotations since boot */
u16 cycle_pos;

#include "res/sign.inc"   /* generated: sign_tiles[], sign_map[], sign_pal[] (two palettes, 32 colours), each with _end */

/** @brief The sign's letters: entries 8-15 of BG palette 0, rotated in place (RAM copy) */
static u16 sign_wheel[WHEEL_LEN];
/** @brief The coolant: entries 8-15 of BG palette 1, rotated in place (RAM copy) */
static u16 flow_wheel[WHEEL_LEN];

/** @brief Rotate a wheel down by one, the last entry coming first: each colour
 *  moves to the next index, so the light runs the way the indices are laid out
 *  (left to right) */
static void rotate(u16 *w) {
    u16 last = w[WHEEL_LEN - 1];
    u8 i;
    for (i = WHEEL_LEN - 1; i > 0; i--)
        w[i] = w[i - 1];
    w[0] = last;
}

int main(void) {
    const u16 *pal = (const u16 *)sign_pal;
    u8 i, frame = 0;

    consoleInit();

    /* The wall, the sign and the pipe: tiles at VRAM $0000, map at $3C00,
     * both palettes (CGRAM 0-31) */
    dmaCopyVram(sign_tiles, 0x0000, sign_tiles_end - sign_tiles);
    dmaCopyVram(sign_map, 0x3C00, sign_map_end - sign_map);
    dmaCopyCGram(sign_pal, 0, sign_pal_end - sign_pal);

    /* The two wheels start as the picture's own colours */
    for (i = 0; i < WHEEL_LEN; i++) {
        sign_wheel[i] = pal[WHEEL_FIRST + i];
        flow_wheel[i] = pal[16 + WHEEL_FIRST + i];
    }

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
            frame = 0;

            /* Rotate both wheels by one, then reload their 8 CGRAM
             * entries each: 8-15 (the letters) and 24-31 (the coolant). */
            rotate(sign_wheel);
            rotate(flow_wheel);
            dmaCopyCGram((u8 *)sign_wheel, WHEEL_FIRST, WHEEL_LEN * 2);
            dmaCopyCGram((u8 *)flow_wheel, 16 + WHEEL_FIRST, WHEEL_LEN * 2);
            cycle_pos++;
        }
    }

    return 0;
}
