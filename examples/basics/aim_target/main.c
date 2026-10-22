/**
 * @file main.c
 * @brief Aim, distance, and angle showcase for `<snes/math.h>`
 * @ingroup examples
 *
 * Demonstrates the four math primitives a 2D action game leans on the
 * most: **`sqrt16`** (pixel distance), **`atan2_8`** (8-bit angle to
 * a target), and the **`fixCos` / `fixSin` LUTs** (project a velocity
 * vector along that angle). Everything updates live as the user moves
 * the target with the D-pad — the numbers on screen are real
 * arithmetic, not pre-baked.
 *
 * A security turret sits on a roof at the screen centre; the drone it
 * aims at starts upper-right and follows the D-pad. Each frame the example
 * recomputes dx/dy from the two positions, calls `sqrt16(dx² + dy²)` for
 * the pixel distance, calls `atan2_8(dy, dx)` for the 8-bit angle, then
 * calls `fixCos` and `fixSin` on that angle to read back the unit
 * direction vector. All five values are printed live in the text panel.
 * The turret turns to face the drone: it has sixteen frames, one per
 * sixteenth of a turn, and the angle picks one (`(angle + 8) >> 4`, the
 * nearest heading) — the "rotate a sprite to face X" pattern, with no
 * rotation hardware.
 *
 * A second drone orbits the roof on its own: its angle is a 16.16
 * accumulator (`fixed32`) advanced by a 16.16 angular velocity every frame,
 * and its position is `radius * fix32Cos` / `radius * fix32Sin` through
 * `fix32Mul` — the motion half of the fixed-point story (it was the
 * fix32_orbit example).
 *
 * @par SNES Concepts
 * - `<snes/math.h>` square-root and atan2:
 *     - `sqrt16(n)` → integer floor of √n, range 0–255
 *     - `atan2_8(dy, dx)` → 8-bit angle in the same convention as
 *       the sin/cos LUT (0 = +X, 64 = +Y, 128 = −X, 192 = −Y)
 * - `fixSin`/`fixCos` LUT chaining: feed the `atan2_8` output back
 *   into the trig LUT to recover a unit direction vector that points
 *   from the turret to the drone — the canonical pattern for projectile
 *   aiming, pursuit AI, and any "rotate sprite to face X" code.
 * - `<snes/fixed32.h>` motion: an angle accumulator and a velocity in
 *   16.16, `fix32Mul(radius, fix32Cos(angle))` for the position; the
 *   integer part of the accumulator (`UNFIX32`) indexes the 256-step LUT
 * - Mode 0 + sprite overlay: BG1 hosts the text via the `text`
 *   module, BG2 the roof (2bpp, its own four colours at CGRAM 32), and
 *   the turret and the drones are 32×32 OBJ sprites over both.
 * - A sprite's position is its top-left corner: the example keeps the
 *   centres (what the maths should aim between) and draws each 32×32
 *   sprite 16 pixels up and left of its centre.
 *
 * @par What to Observe
 * - The turret stays at the screen centre and turns its barrel to the
 *   drone, in steps of a sixteenth of a turn.
 * - The drone follows the D-pad. Diagonal input moves
 *   diagonally — both axes update on the same frame.
 * - As you move the target, the live readout panel updates on
 *   every frame:
 *     - `DX` / `DY` are the signed distances from the turret to the drone.
 *     - `DIST` is `sqrt16(dx² + dy²)`, in pixels (0–255).
 *     - `ANGLE` is the 8-bit angle from the turret to the drone,
 *       printed in hex; the equivalent in degrees follows in
 *       parentheses (≈ angle × 360 / 256).
 *     - `COS 8.8` / `SIN 8.8` are the 8.8 fixed-point components of
 *       the unit direction vector. At `ANGLE = 0x40` (90°, target is
 *       straight south) you see `COS = 0x0000`, `SIN = 0x0100`.
 *     - `COS 16.16` / `SIN 16.16` are the SAME values lifted to the
 *       fix32 type. Look at the bottom hex
 *       digits: they're always `00` because `fix32Cos`/`fix32Sin`
 *       reuse the 8.8 LUT and shift up. A future 16-bit LUT would
 *       fill those bits with real precision — the comparison row
 *       makes the 8-bit-LUT precision floor visible at runtime.
 *
 * @par 8.8 and 16.16 side by side
 * The `fixCos`/`fixSin` (8.8) rows have `fix32Cos`/`fix32Sin` (16.16)
 * rows beside them, so the two representations can be compared; the
 * trailing zeros of the 16.16 ones show that they lift the same 8.8
 * table.
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, gameloop, math, fixed32
 *
 * @see math.h, fixed32.h, sprite.h, text.h, gameloop.h
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/math.h>
#include <snes/fixed32.h>      /* fix32Cos / fix32Sin */
#include <snes/gameloop.h>

/*============================================================================
 * Graphics (converted by the build from res/, one settings file beside each)
 *============================================================================*/

#include "res/units.inc"
#include "res/roof.inc"
/* generated: units_tiles/_pal (each with _end), roof_tiles/_pal/_map */

/** @brief Where BG2 (the roof) keeps its tiles and its map; the text has $0000 and $3800. */
#define ROOF_TILES 0x1000
#define ROOF_MAP   0x3C00
/** @brief The sprites' tiles, after the font and the maps. */
#define OBJ_VRAM   0x4000
/** @brief The drones' frames in res/units.png, after the turret's sixteen. */
#define TILE_DRONE   256
#define TILE_ORBITER 260

/*============================================================================
 * State
 *============================================================================*/

/**
 * @brief The turret's centre — fixed at the screen centre.
 *
 * Made into a `#define` rather than a runtime variable so the
 * compiler folds the constant into the dx/dy expressions and we
 * don't pay a load+sub for a quantity that never changes.
 */
#define TURRET_X 128
#define TURRET_Y 112

/** @brief Half a 32×32 sprite: from its centre to the top-left corner OAM wants. */
#define HALF 16

/** @brief The drone's centre, X (movable, 16–240: the sprite stays on screen). */
static s16 target_x;
/** @brief The drone's centre, Y (movable, 16–208). */
static s16 target_y;
/** @brief The turret's heading, 0–15 (sixteenths of a turn): its frame. */
u8 turret_heading;
/** @brief Previous target X — for change detection (skip text reformat
 *         when nothing moved). */
static s16 prev_target_x;
/** @brief Previous target Y — see prev_target_x. */
static s16 prev_target_y;

/** @brief The orbiter: angle (16.16, its integer part a LUT step), velocity, radius */
fixed32 g_angle;
fixed32 g_omega;
fixed32 g_radius;
/** @brief The orbiter's centre on screen and its angle as a LUT index */
s16     g_sx;
s16     g_sy;
u8      g_angle8;

/**
 * @brief Advance the orbiter one frame: centre (128, 96), radius 60 px,
 * one LUT step a frame (an orbit in about 4.3 s at 60 Hz).
 */
static void orbit_step(void) {
    fixed32 ox, oy;
    g_angle  = g_angle + g_omega;
    g_angle8 = (u8)UNFIX32(g_angle);
    ox = fix32Mul(g_radius, fix32Cos(g_angle8));
    oy = fix32Mul(g_radius, fix32Sin(g_angle8));
    g_sx = 128 + (s16)UNFIX32(ox);
    g_sy = 96  + (s16)UNFIX32(oy);
    oamSet(2, g_sx - HALF, g_sy - HALF, TILE_ORBITER, 2, 3, 0);
}

/**
 * @brief The OAM tile of the turret's frame for @p heading (0–15)
 *
 * A 32×32 frame is 4×4 tiles and a row of OBJ tiles is 16 wide, so heading
 * h (four a row of the sheet) starts at column (h % 4) * 4 of tile row
 * (h / 4) * 4.
 */
static u16 turret_tile(u8 heading) {
    return (u16)(((heading & 3) << 2) + ((heading >> 2) << 6));
}

/*============================================================================
 * Lifecycle
 *============================================================================*/

static void on_init(void) {
    /* Text mode FIRST — `textModeInit` does `consoleInit` + Mode 0
     * + loads the font at VRAM $0000 + tilemap setup, all in one
     * go. We override only its `setMainScreen(LAYER_BG1)` at the
     * end of this function so that sprites also show. */
    textModeInit();

    /* BG2: the roof, under the text (in Mode 0, BG1 is in front of
     * BG2). Its four colours go to CGRAM 32-35: in Mode 0 each BG has
     * its own palettes (BG_4COLORS0). */
    bgInitTileSet(1, roof_tiles, roof_pal, 0,
                  roof_tiles_end - roof_tiles,
                  roof_pal_end - roof_pal,
                  BG_4COLORS0, ROOF_TILES);
    dmaCopyVram(roof_map, ROOF_MAP, roof_map_end - roof_map);
    bgSetMapPtr(1, ROOF_MAP, SC_32x32);

    /* Sprite tiles go to VRAM $4000 (NOT $0000) so they don't
     * collide with the text font that textModeInit just loaded.
     * `oamInitGfxSet` also recomputes the OBJ tile base from the
     * VRAM address (`base = addr >> 13` → 2 for $4000), updates
     * `REG_OBJSEL`, and calls `oamInit` for us. The sheet is 264
     * tiles: the drones are past 255, in the second name table. */
    oamInitGfxSet(units_tiles, units_tiles_end - units_tiles,
                  units_pal, units_pal_end - units_pal,
                  0, OBJ_VRAM, OBJ_SIZE16_L32);

    /* Initial target position: upper-right quadrant — far enough
     * from the centre that DIST and ANGLE both have non-trivial
     * values out of the gate. Seed prev_* with a sentinel so the
     * first on_update() unconditionally formats the panel. */
    g_angle  = 0;
    g_omega  = FIX32(1);      /* 1 LUT step per frame */
    g_radius = FIX32(60);     /* 60 px, inside the 256x224 screen */

    target_x = 200;
    target_y = 60;
    prev_target_x = -1;
    prev_target_y = -1;

    /* Three sprites, all 32×32 (the LARGE size of OBJ_SIZE16_L32):
     *   index 0 = the turret at the screen centre, palette 0
     *   index 1 = the drone you fly, palette 1
     *   index 2 = the orbiter, palette 2 (orbit_step)
     * oamSet signature: (id, x, y, tile, palette, priority, flags). */
    oamSet(0, TURRET_X - HALF, TURRET_Y - HALF, turret_tile(0), 0, 3, 0);
    oamSet(1, target_x - HALF, target_y - HALF, TILE_DRONE, 1, 3, 0);
    oamSetSize(0, OBJ_LARGE);
    oamSetSize(1, OBJ_LARGE);
    oamSetSize(2, OBJ_LARGE);

    /* Static labels — drawn once, never overwritten. The dynamic
     * rows (DX, DY, DIST, ANGLE, COS, SIN) live further down. */
    textPrintAt(2,  1, "AIM, DISTANCE, AND ANGLE");
    textPrintAt(2,  2, "MATH HEADER SHOWCASE");
    textPrintAt(2,  4, "D-PAD: FLY THE DRONE");
    textPrintAt(2,  5, "THE TURRET TURNS TO FACE IT");

    textPrintAt(2, 17, "DX:");
    textPrintAt(2, 18, "DY:");
    textPrintAt(2, 19, "DIST:");
    textPrintAt(2, 20, "ANGLE:");
    /* COS / SIN shown in both formats to expose the 8.8 vs 16.16
     * tradeoff at runtime: fixed (8.8) and fixed32 (16.16). Since
     * fix32Cos/Sin lift the same 8.8 LUT, the low byte of the
     * 16.16 fractional half is always zero — visible in the hex
     * readout as a trailing `00`. A future 16-bit LUT would fill
     * those bits with real precision. */
    textPrintAt(2, 22, "COS 8.8:");
    textPrintAt(2, 23, "SIN 8.8:");
    textPrintAt(2, 24, "COS 16.16:");
    textPrintAt(2, 25, "SIN 16.16:");

    /* BG1 (text), BG2 (the roof) and OBJ (sprites) on the main screen. */
    setMainScreen(TM_BG1 | TM_BG2 | TM_OBJ);

    WaitForVBlank();
    setScreenOn();
}

static void on_update(void) {
    u16 pad;
    s16 dx, dy;
    u16 abs_dx, abs_dy;
    u16 dx2, dy2;
    u16 dist;
    u8  angle;
    fixed   cos_v,  sin_v;   /* 8.8 lib trig (original) */
    fixed32 cos32, sin32;    /* 16.16 lifted trig */

    /* The orbiter moves every frame, before the early return below that
     * skips the aim maths while the target stands still. */
    orbit_step();

    pad = padHeld(0);

    /* D-pad → drone movement. Bounds keep the sprite on screen. */
    if ((pad & KEY_UP)    && target_y > HALF)       target_y--;
    if ((pad & KEY_DOWN)  && target_y < 224 - HALF) target_y++;
    if ((pad & KEY_LEFT)  && target_x > HALF)       target_x--;
    if ((pad & KEY_RIGHT) && target_x < 256 - HALF) target_x++;

    /* Skip the recompute + text refresh when the target hasn't
     * moved. The text writers are heavy (formatting loops + DMA
     * flush flag) and re-running them every frame burned half the
     * frame budget on idle frames — caught by the lag-detection
     * phase before this guard was added. */
    if (target_x == prev_target_x && target_y == prev_target_y) return;
    prev_target_x = target_x;
    prev_target_y = target_y;

    oamSet(1, target_x - HALF, target_y - HALF, TILE_DRONE, 1, 3, 0);

    /* Compute the dx/dy vector from the turret → the drone, centre to
     * centre. The turret is at a fixed location so the compiler folds
     * the constants. */
    dx = (s16)(target_x - TURRET_X);
    dy = (s16)(target_y - TURRET_Y);

    /* Pixel distance via Pythagoras. dx and dy are bounded by the
     * screen geometry: |dx| ≤ 112, |dy| ≤ 96, so |dx|² ≤ 12544
     * and |dy|² ≤ 9216 — both fit in u16, and so does their sum
     * (≤ 21760). No 32-bit promotion needed; sqrt16 returns a
     * value bounded by 255 (since sqrt(65535) ≈ 255.99). */
    abs_dx = (dx < 0) ? (u16)(0 - dx) : (u16)dx;
    abs_dy = (dy < 0) ? (u16)(0 - dy) : (u16)dy;
    dx2  = (u16)(abs_dx * abs_dx);
    dy2  = (u16)(abs_dy * abs_dy);
    dist = sqrt16((u16)(dx2 + dy2));

    /* Angle from the turret → the drone. atan2_8 takes signed dy/dx and
     * returns the 8-bit angle convention used by fixSin/fixCos. */
    angle = atan2_8(dy, dx);

    /* The turret faces the drone: sixteen frames, a sixteenth of a turn
     * (16 angle steps) apart. Adding half a step (8) before the shift
     * picks the NEAREST heading instead of the one just below. */
    turret_heading = (u8)(((u8)(angle + 8) >> 4) & 15);
    oamSet(0, TURRET_X - HALF, TURRET_Y - HALF, turret_tile(turret_heading), 0, 3, 0);

    /* Project the unit vector at this angle. fixCos/fixSin return
     * 8.8 fixed (256 = 1.0). The pair (cos_v, sin_v) is exactly
     * what you'd multiply a projectile speed by to make it travel
     * toward the target. */
    cos_v = fixCos(angle);
    sin_v = fixSin(angle);

    /* Same angle, lifted to 16.16. Cost: ~25 cycles each (table
     * lookup + xba byte-swap + sign-extend). The orbiter uses the same
     * calls for its sub-pixel-precise circular motion (orbit_step). */
    cos32 = fix32Cos(angle);
    sin32 = fix32Sin(angle);

    /* Live readout — all six lines update every frame. The
     * trailing spaces ensure leading-zero changes overwrite stale
     * digits from a previous longer print. */
    /* dx and dy are signed and the text module prints a u16: the sign
     * first, then the magnitude computed above for the distance. */
    textSetPos(9, 17); textPrint(dx < 0 ? "-" : " "); textPrintU16(abs_dx); textPrint("    ");
    textSetPos(9, 18); textPrint(dy < 0 ? "-" : " "); textPrintU16(abs_dy); textPrint("    ");
    textSetPos(9, 19); textPrintU16(dist);     textPrint("    ");

    textSetPos(9, 20); textPrint("0X");
    textPrintHex(angle, 2);
    textPrint(" (");
    /* Map 8-bit angle (0..255) → degrees (0..359). The natural
     * formula is `angle * 360 / 256`, but `angle * 360` overflows
     * u16 once `angle > 181`. Refactored to `angle * 45 >> 5`,
     * which is the same expression with `360 / 256 = 45/32`
     * substituted: `angle * 45` peaks at 11475 (well within u16)
     * and the final `>> 5` divides by 32. */
    textPrintU16((u16)((u16)((u16)angle * 45) >> 5));
    textPrint(" DEG)   ");

    textSetPos(13, 22); textPrint("0X");
    textPrintHex((u16)cos_v, 4);
    textPrint("    ");
    textSetPos(13, 23); textPrint("0X");
    textPrintHex((u16)sin_v, 4);
    textPrint("    ");

    /* 16.16 display: high half then low half. Notice the low half
     * is always 0x00XX (low byte zero) because fix32Cos/Sin lift
     * the 8.8 LUT — the bottom 8 fractional bits are unused. */
    textSetPos(13, 24); textPrint("0X");
    textPrintHex((u16)((u32)cos32 >> 16), 4);
    textPrintHex((u16)((u32)cos32 & 0xFFFF), 4);
    textSetPos(13, 25); textPrint("0X");
    textPrintHex((u16)((u32)sin32 >> 16), 4);
    textPrintHex((u16)((u32)sin32 & 0xFFFF), 4);
}

int main(void) {
    GameLoopConfig cfg = {
        .init = on_init,
        .update = on_update,
    };
    gameLoopRun(&cfg);
    return 0;
}
