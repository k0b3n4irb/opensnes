/**
 * @file main.c
 * @brief 16.16 fixed-point orbit demo (B5 capstone)
 * @ingroup examples
 *
 * A single 8x8 sprite orbits the screen centre. The orbit is driven
 * entirely by the lib's `<snes/fixed32.h>` API — `fix32Sin`,
 * `fix32Cos`, `fix32Mul`, plus straight Kl arithmetic on `fixed32`
 * accumulators.
 *
 * What this example demonstrates
 * ------------------------------
 * - `fixed32` (s32 16.16) for an angle accumulator that stays smooth
 *   across many frames without 8.8 rounding drift.
 * - `fix32Sin` / `fix32Cos` lifted from the 8.8 LUT to 16.16 (8 bits
 *   of fractional precision, ~25 cycles each — adequate for sub-pixel
 *   animation).
 * - `fix32Mul(radius, fix32Cos(angle))` as the canonical pattern for
 *   trig-driven motion: a `radius * sin/cos` term used in every 2D
 *   game that needs aim/orbit/parallax-around-a-point.
 *
 * Inputs / interaction
 * --------------------
 * None — the sprite orbits autonomously. The example is structured as
 * the simplest possible fixed32 fixture; later examples (likemario,
 * aim_target, breakout) compose the same primitives into game logic.
 *
 * @par Modules Used
 * console, sprite, dma, background, fixed32, math
 */

#include <snes.h>
#include <snes/fixed32.h>

/*============================================================================
 * Sprite asset: a single 8×8 4bpp tile, solid colour-1 (white).
 *
 * 4bpp tiles use planar format: 8 bytes per plane × 4 planes = 32 bytes.
 * Setting plane 0 to all 0xFF and planes 1-3 to zero gives every pixel
 * the palette index 1, which we colour white below.
 *============================================================================*/

static const u8 orbiter_tile[32] = {
    /* Plane 0 (bit 0 of each pixel): all set */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* Plane 1: clear */
    0, 0, 0, 0, 0, 0, 0, 0,
    /* Plane 2: clear */
    0, 0, 0, 0, 0, 0, 0, 0,
    /* Plane 3: clear */
    0, 0, 0, 0, 0, 0, 0, 0,
};

/* 4 bytes = 2 BGR555 colours.
 *   Colour 0 = palette transparent (any value — PPU ignores it for sprites).
 *   Colour 1 = white ($7FFF, little-endian = 0xFF, 0x7F). */
static const u8 orbiter_pal[4] = {
    0x00, 0x00,
    0xFF, 0x7F,
};

/*============================================================================
 * Orbit state — exposed as globals for easy MCP inspection during dev.
 *
 * `g_angle` is a 16.16 accumulator; its integer part wraps mod 256 (treated
 * as an 8-bit angle index into the sin LUT). `g_omega` controls speed:
 * FIX32(1) per frame = 1 LUT step per frame = 1/256 of a full turn per
 * frame = ~234 frames for a full revolution.
 *============================================================================*/

fixed32 g_angle;
fixed32 g_omega;
fixed32 g_radius;
s16     g_sx;
s16     g_sy;
u8      g_angle8;

int main(void) {
    g_angle  = 0;
    g_omega  = FIX32(1);      /* 1 LUT-step per frame → ~3.9 s per orbit at 60 Hz */
    g_radius = FIX32(60);     /* 60 px = comfortably inside the 256×224 visible area */

    consoleInit();
    WaitForVBlank();

    /* Load the sprite tile to VRAM and its palette to the sprite-palette
     * region of CGRAM. */
    dmaCopyVram((u8 *)orbiter_tile, 0x4000, sizeof orbiter_tile);
    dmaCopyCGram((u8 *)orbiter_pal, OBJ_CGRAM_BASE, sizeof orbiter_pal);

    /* OBJSEL: sprite tile base at $4000 word-addr, small/large = 8x8 / 16x16. */
    REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x4000);

    oamInit(OBJ_SIZE8_L16, 1);
    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        /* Advance the angle accumulator. The high half of g_angle wraps
         * naturally as an unsigned 16-bit count, but we only consume the
         * low byte of UNFIX32 so any drift in the upper bits is invisible. */
        g_angle   = g_angle + g_omega;
        g_angle8  = (u8)UNFIX32(g_angle);

        /* Compute the orbit offset: (radius * cos, radius * sin). */
        fixed32 ox = fix32Mul(g_radius, fix32Cos(g_angle8));
        fixed32 oy = fix32Mul(g_radius, fix32Sin(g_angle8));

        /* Project to integer screen coordinates centred at (128, 96). */
        g_sx = 128 + (s16)UNFIX32(ox);
        g_sy = 96  + (s16)UNFIX32(oy);

        oamSet(0, g_sx, g_sy, 0, 0, 3, 0);
        WaitForVBlank();
    }
    return 0;
}
