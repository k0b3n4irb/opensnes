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
 * The player ("P") sits at screen center. The target ("X") starts
 * upper-right and follows the D-pad. Each frame the example recomputes
 * dx/dy from the two positions, calls `sqrt16(dx² + dy²)` for the
 * pixel distance, calls `atan2_8(dy, dx)` for the 8-bit angle, then
 * calls `fixCos` and `fixSin` on that angle to read back the unit
 * direction vector. All five values are printed live in the text
 * panel above the play area.
 *
 * @par SNES Concepts
 * - `<snes/math.h>` square-root and atan2 (chantier B6, 2026-05-09):
 *     - `sqrt16(n)` → integer floor of √n, range 0–255
 *     - `atan2_8(dy, dx)` → 8-bit angle in the same convention as
 *       the sin/cos LUT (0 = +X, 64 = +Y, 128 = −X, 192 = −Y)
 * - `fixSin`/`fixCos` LUT chaining: feed the `atan2_8` output back
 *   into the trig LUT to recover a unit direction vector that points
 *   from player to target — the canonical pattern for projectile
 *   aiming, pursuit AI, and any "rotate sprite to face X" code.
 * - Mode 0 + sprite overlay: BG1 hosts the text via the `text`
 *   module; the player and target are 8×8 OBJ sprites on the OBJ
 *   layer so they don't disturb the text display.
 *
 * @par What to Observe
 * - The "P" sprite stays anchored at the screen centre.
 * - The "X" sprite follows the D-pad. Diagonal input moves
 *   diagonally — both axes update on the same frame.
 * - As you move the target, the live readout panel updates on
 *   every frame:
 *     - `DX` / `DY` are the signed distances from player to target.
 *     - `DIST` is `sqrt16(dx² + dy²)`, in pixels (0–255).
 *     - `ANGLE` is the 8-bit angle from the player to the target,
 *       printed in hex; the equivalent in degrees follows in
 *       parentheses (≈ angle × 360 / 256).
 *     - `COS` / `SIN` are the 8.8 fixed-point components of the unit
 *       direction vector. Their values match the angle: at
 *       `ANGLE = 0x40` (90°, target is straight south on a SNES
 *       screen) you see `COS = 0x0000` and `SIN = 0x0100`.
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, gameloop, math
 *
 * @see math.h, sprite.h, text.h, gameloop.h
 *
 * @copyright MIT License — chantier B6 (2026-05-09)
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/math.h>
#include <snes/gameloop.h>

/*============================================================================
 * Sprite tile data (inline 4bpp, 32 bytes per 8×8 tile)
 *============================================================================*/

/**
 * @brief Two 8×8 sprite tiles in SNES 4bpp format (64 bytes total).
 *
 * Tile 0 ("P", player): a filled square edged with a dot — easy
 * to spot at the screen centre.
 *
 * Tile 1 ("X", target): a crosshair / plus sign — points at its
 * own centre so the user knows exactly where the target's
 * coordinate is.
 *
 * Both tiles use bitplanes 0 and 1; planes 2 and 3 are zero so
 * every drawn pixel is colour index 3 in the chosen palette.
 */
static const u8 sprite_tiles[64] = {
    /* Tile 0: filled diamond (player). Planes 0 AND 1 are set so
     * each drawn pixel = colour index 3 (binary 0011). The shape
     * is an 8×8 diamond: rows 18,3C,7E,FF,FF,7E,3C,18. */
    0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18,  /* plane 0 */
    0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18,  /* plane 1 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1: thick X (target). Two-pixel-wide diagonals so the
     * shape is unambiguously visible at 1× zoom. The X intersects
     * at the sprite's centre — what the player sees as "where the
     * target is" matches the (x, y) coordinate the math reads
     * back. Both planes set so colour index 3 again. */
    0xC3, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x66, 0xC3,  /* plane 0 */
    0xC3, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x66, 0xC3,  /* plane 1 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/**
 * @brief Sprite palette — single palette shared by player and target.
 *
 * Both sprites use palette 0; the visual difference between them is
 * entirely the tile shape (diamond vs. X). Keeping the palette
 * minimal trims four bytes off the CGRAM upload and avoids the
 * second `dmaCopyCGram` call that an earlier draft used.
 *
 * BGR555 layout: `[R5G5B5]` packed into two bytes, low byte first.
 * Only colour 3 is rendered — the tile bitplanes above always
 * resolve to index 3.
 */
static const u8 sprite_pal[] = {
    0x00, 0x00,                /* 0: transparent */
    0x00, 0x00,                /* 1: black (unused) */
    0x00, 0x00,                /* 2: black (unused) */
    0xFF, 0x7F,                /* 3: white */
};

/*============================================================================
 * State
 *============================================================================*/

/**
 * @brief Player position — fixed at screen centre.
 *
 * Made into a `#define` rather than a runtime variable so the
 * compiler folds the constant into the dx/dy expressions and we
 * don't pay a load+sub for a quantity that never changes.
 */
#define PLAYER_X 124
#define PLAYER_Y 108

/** @brief Target X coordinate (movable, 0–247). */
static s16 target_x;
/** @brief Target Y coordinate (movable, 0–215). */
static s16 target_y;
/** @brief Previous target X — for change detection (skip text reformat
 *         when nothing moved). */
static s16 prev_target_x;
/** @brief Previous target Y — see prev_target_x. */
static s16 prev_target_y;

/*============================================================================
 * Lifecycle
 *============================================================================*/

static void on_init(void) {
    /* Text mode FIRST — `textModeInit` does `consoleInit` + Mode 0
     * + loads the font at VRAM $0000 + tilemap setup, all in one
     * go. We override only its `setMainScreen(LAYER_BG1)` at the
     * end of this function so that sprites also show. */
    textModeInit();

    /* Sprite tiles go to VRAM $4000 (NOT $0000) so they don't
     * collide with the text font that textModeInit just loaded.
     * `oamInitGfxSet` also recomputes the OBJ tile base from the
     * VRAM address (`base = addr >> 13` → 2 for $4000), updates
     * `REG_OBJSEL`, and calls `oamInit` for us. */
    oamInitGfxSet((u8 *)sprite_tiles, 64,
                  (u8 *)sprite_pal, 8,
                  0, 0x4000, OBJ_SIZE8_L16);

    /* Initial target position: upper-right quadrant — far enough
     * from the centre that DIST and ANGLE both have non-trivial
     * values out of the gate. Seed prev_* with a sentinel so the
     * first on_update() unconditionally formats the panel. */
    target_x = 200;
    target_y = 60;
    prev_target_x = -1;
    prev_target_y = -1;

    /* Two sprites:
     *   index 0 = player at the screen centre — tile 0 (diamond)
     *   index 1 = target — tile 1 (X)
     * Both use palette 0; the shape carries the meaning.
     * oamSet signature: (id, x, y, tile, palette, priority, flags). */
    oamSet(0, PLAYER_X, PLAYER_Y, 0, 0, 0, 0);
    oamSet(1, target_x,  target_y, 1, 0, 0, 0);

    /* Static labels — drawn once, never overwritten. The dynamic
     * rows (DX, DY, DIST, ANGLE, COS, SIN) live further down. */
    textPrintAt(2,  1, "AIM, DISTANCE, AND ANGLE");
    textPrintAt(2,  2, "MATH HEADER SHOWCASE");
    textPrintAt(2,  4, "USE D-PAD TO MOVE THE X");
    textPrintAt(2,  5, "P = PLAYER (FIXED CENTRE)");
    textPrintAt(2,  6, "X = TARGET (YOU MOVE IT)");

    textPrintAt(2, 17, "DX:");
    textPrintAt(2, 18, "DY:");
    textPrintAt(2, 19, "DIST:");
    textPrintAt(2, 20, "ANGLE:");
    textPrintAt(2, 22, "COS(ANGLE):");
    textPrintAt(2, 23, "SIN(ANGLE):");

    /* Both BG1 (text) and OBJ (sprites) on the main screen. */
    setMainScreen(TM_BG1 | TM_OBJ);

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
    fixed cos_v, sin_v;

    pad = padHeld(0);

    /* D-pad → target movement. Bounds keep the sprite on screen. */
    if ((pad & KEY_UP)    && target_y > 0)   target_y--;
    if ((pad & KEY_DOWN)  && target_y < 215) target_y++;
    if ((pad & KEY_LEFT)  && target_x > 0)   target_x--;
    if ((pad & KEY_RIGHT) && target_x < 247) target_x++;

    /* Skip the recompute + text refresh when the target hasn't
     * moved. The text writers are heavy (formatting loops + DMA
     * flush flag) and re-running them every frame burned half the
     * frame budget on idle frames — caught by the lag-detection
     * phase before this guard was added. */
    if (target_x == prev_target_x && target_y == prev_target_y) return;
    prev_target_x = target_x;
    prev_target_y = target_y;

    oamSet(1, target_x, target_y, 1, 0, 0, 0);

    /* Compute the dx/dy vector from player → target. Player is at
     * a fixed location so the compiler folds the constants. */
    dx = (s16)(target_x - PLAYER_X);
    dy = (s16)(target_y - PLAYER_Y);

    /* Pixel distance via Pythagoras. dx and dy are bounded by the
     * screen geometry: |dx| ≤ 124, |dy| ≤ 108, so |dx|² ≤ 15376
     * and |dy|² ≤ 11664 — both fit in u16, and so does their sum
     * (≤ 27040). No 32-bit promotion needed; sqrt16 returns a
     * value bounded by 255 (since sqrt(65535) ≈ 255.99). */
    abs_dx = (dx < 0) ? (u16)(0 - dx) : (u16)dx;
    abs_dy = (dy < 0) ? (u16)(0 - dy) : (u16)dy;
    dx2  = (u16)(abs_dx * abs_dx);
    dy2  = (u16)(abs_dy * abs_dy);
    dist = sqrt16((u16)(dx2 + dy2));

    /* Angle from player → target. atan2_8 takes signed dy/dx and
     * returns the 8-bit angle convention used by fixSin/fixCos. */
    angle = atan2_8(dy, dx);

    /* Project the unit vector at this angle. fixCos/fixSin return
     * 8.8 fixed (256 = 1.0). The pair (cos_v, sin_v) is exactly
     * what you'd multiply a projectile speed by to make it travel
     * toward the target. */
    cos_v = fixCos(angle);
    sin_v = fixSin(angle);

    /* Live readout — all six lines update every frame. The
     * trailing spaces ensure leading-zero changes overwrite stale
     * digits from a previous longer print. */
    textSetPos(9, 17); textPrintU16((u16)dx);  textPrint("    ");
    textSetPos(9, 18); textPrintU16((u16)dy);  textPrint("    ");
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

    textSetPos(15, 22); textPrint("0X");
    textPrintHex((u16)cos_v, 4);
    textPrint(" ");
    textSetPos(15, 23); textPrint("0X");
    textPrintHex((u16)sin_v, 4);
    textPrint(" ");
}

int main(void) {
    GameLoopConfig cfg = {
        .init = on_init,
        .update = on_update,
    };
    gameLoopRun(&cfg);
    return 0;
}
