/**
 * @file dsp1.h
 * @brief DSP-1 coprocessor (NEC µPD77C25) — fixed-point 3D math from C.
 *
 * The DSP-1 is a fixed-function math coprocessor found in Pilotwings, Super
 * Mario Kart and other pseudo-3D games. It runs Sony's fixed firmware; you do
 * not program it — you invoke its ~30 built-in commands (matrix, vector,
 * projection, trig) over a two-register port. This module wraps that command
 * interface as ordinary C calls.
 *
 * @par Cartridge requirement
 * Build with `USE_DSP1 := 1` (sets the ROM header cartridge type to $03 and
 * maps the DSP registers). The DSP-1 registers live at $30:8000 (data) /
 * $30:C000 (status) on the LoROM board this SDK targets.
 *
 * @par Fixed-point
 * The DSP-1 types its operands per slot:
 * - **T** — signed 1.15 fraction (−1.0 … +0.99997); `0x7FFF`≈+1, `0x4000`=0.5.
 * - **I** — signed 16-bit integer (coordinates).
 * - **A** — signed 16-bit angle; a full turn is 2^16, so `0x4000`=+90°,
 *   `0x8000`=±180°, and angle arithmetic wraps for free.
 *
 * @par Multi-word results
 * Commands that return more than one word write their outputs to the module's
 * result globals (`dsp1_o0`, `dsp1_o1`, `dsp1_o2`); read them after the call.
 * Single-word commands return their value directly.
 *
 * @warning luna emulates the DSP-1 at low level and needs Sony's
 * `dsp1b.rom` firmware installed (`~/.config/luna/firmware/` or via
 * `luna state --dsp1-rom <path> <rom>` once — the setting persists).
 * It is copyrighted and not shipped; DSP-1 examples are firmware-gated in CI.
 *
 * @warning DSP-1 calls are NOT safe inside nmiSet() callbacks. A command is a
 * multi-byte DR transaction; if NMI interrupts one mid-sequence and the
 * callback issues its own DSP-1 command, both transactions are corrupted
 * (same class as the fixMul() restriction in KNOWN_LIMITATIONS.md). Keep all
 * DSP-1 work in the main loop.
 *
 * @code
 * // rotate the point (100, 0) by 90 degrees -> (0, 100)
 * dsp1Rotate(0x4000, 100, 0);
 * s16 x = dsp1_o0;   // ~0
 * s16 y = dsp1_o1;   // ~100
 * @endcode
 *
 * @see .claude/notes/tech/dsp1_reference.md — the full command reference.
 */
#ifndef SNES_DSP1_H
#define SNES_DSP1_H

#include <snes/types.h>

/**
 * @brief Multi-word DSP-1 result registers (written by multi-output commands).
 *
 * Meaning is per command: e.g. after dsp1Triangle, `dsp1_o0` = radius·sin and
 * `dsp1_o1` = radius·cos; after dsp1Rotate, `dsp1_o0`/`dsp1_o1` = x'/y'.
 */
extern volatile s16 dsp1_o0;
extern volatile s16 dsp1_o1;  /**< @see dsp1_o0 */
extern volatile s16 dsp1_o2;  /**< @see dsp1_o0 */
extern volatile s16 dsp1_o3;  /**< @see dsp1_o0 (4th word — dsp1Parameter only) */

/**
 * @name Bridges to the SDK fixed-point types
 * The SDK's `fixed` (snes/math.h) is signed 8.8 and its angles are 8-bit
 * (256 = full turn); the DSP-1 uses 1.15 fractions (T) and 16-bit angles (A).
 * These macros convert. Note the T range is −1..+1: converting a `fixed`
 * outside −1.0..+0.999 overflows — clamp first if your value can exceed it.
 * @{
 */
/** @brief SDK 8.8 `fixed` → DSP-1 T (1.15). Valid for −1.0 ≤ f < 1.0. */
#define DSP1_T_FROM_FIX(f)   ((s16)((s16)(f) << 7))
/** @brief DSP-1 T (1.15) → SDK 8.8 `fixed` (truncating). */
#define DSP1_FIX_FROM_T(t)   ((s16)((s16)(t) >> 7))
/** @brief SDK 8-bit angle (256 = full turn) → DSP-1 A (2^16 = full turn). */
#define DSP1_A_FROM_FIX8(a)  ((u16)((u16)(u8)(a) << 8))
/** @} */

/**
 * @brief Resynchronise the DSP-1 to a known command-wait state.
 *
 * Issues the `$80` Sync/Reset byte repeatedly (128×, matching the stock-game
 * boot handshake). `$80` flushes any pending command, so this recovers the
 * chip even if a previous command was interrupted mid-parameter. Call once at
 * startup before your first DSP-1 command; harmless to call again to recover.
 */
void dsp1Init(void);

/**
 * @brief Signed 1.15 × 1.15 multiply (DSP-1 command $00).
 * @param a first factor (T, signed 1.15)
 * @param b second factor (T, signed 1.15)
 * @return the product in signed 1.15 (rounded to 15 bits)
 */
u16 dsp1Multiply(u16 a, u16 b);

/**
 * @brief Scaled sine and cosine of an angle (DSP-1 command $04, "Triangle").
 * @param angle 16-bit angle (A; full turn = 2^16)
 * @param radius integer magnitude (I) to scale the result by
 *
 * Writes @ref dsp1_o0 = radius·sin(angle) and @ref dsp1_o1 = radius·cos(angle).
 */
void dsp1Triangle(u16 angle, s16 radius);

/**
 * @brief Rotate a 2D point about the origin (DSP-1 command $0C, "Rotate").
 * @param angle 16-bit rotation angle (A)
 * @param x point X (I)
 * @param y point Y (I)
 *
 * Writes @ref dsp1_o0 = x' and @ref dsp1_o1 = y'. Rotation is clockwise in the
 * SNES screen convention (Y down): (100,0) at 90° gives (0,-100).
 */
void dsp1Rotate(u16 angle, s16 x, s16 y);

/**
 * @brief Build a 3D rotation matrix into slot A (DSP-1 command $01, "Attitude").
 * @param scale matrix scale (T, signed 1.15; `0x7FFF` ≈ 1.0)
 * @param az rotation about Z (A)
 * @param ay rotation about Y (A)
 * @param ax rotation about X (A)
 *
 * Produces no output — it loads the matrix the transform commands consume.
 * Call once per frame (or whenever the orientation changes), then transform
 * your points with dsp1Objective.
 */
void dsp1Attitude(u16 scale, u16 az, u16 ay, u16 ax);

/**
 * @brief Transform a point by matrix A: local → world (DSP-1 command $0D,
 *        "Objective").
 * @param x local X (I)
 * @param y local Y (I)
 * @param z local Z (I)
 *
 * Writes @ref dsp1_o0 = x', @ref dsp1_o1 = y', @ref dsp1_o2 = z' (world space).
 * Run dsp1Attitude first to set the matrix.
 */
void dsp1Objective(s16 x, s16 y, s16 z);

/**
 * @brief Set up the projection plane (DSP-1 command $02, "Parameter").
 * @param fx projection-base X (I)
 * @param fy projection-base Y (I)
 * @param fz projection-base Z (I)
 * @param lfe distance eye → screen plane (I) — smaller = wider FOV
 * @param les distance screen plane → ground reference (I)
 * @param aas screen-plane azimuth angle (A)
 * @param azs screen-plane zenith angle (A)
 *
 * Writes @ref dsp1_o0 = Cx and @ref dsp1_o1 = Cy (the raster coefficients);
 * `dsp1_o2`/`dsp1_o3` receive two further words whose meaning is not yet
 * confirmed by references. Call once (and again whenever the camera moves),
 * then project points with dsp1Project. `lfe`/`les` are best tuned
 * empirically — see the values used by examples/chips/dsp1_cube.
 */
void dsp1Parameter(s16 fx, s16 fy, s16 fz, s16 lfe, s16 les, u16 aas, u16 azs);

/**
 * @brief Project a world point to the screen (DSP-1 command $06, "Project").
 * @param x world X (I)
 * @param y world Y (I)
 * @param z world Z (I)
 *
 * Writes @ref dsp1_o0 = H (screen X), @ref dsp1_o1 = V (screen Y),
 * @ref dsp1_o2 = M (scale/depth — use it to size sprites with distance).
 * Requires a prior dsp1Parameter. Takes ~627 DSP cycles (~83 µs); the driver
 * polls, so timing is handled for you.
 */
void dsp1Project(s16 x, s16 y, s16 z);

/**
 * @brief Euclidean length of a 3D vector (DSP-1 command $28, "Distance").
 * @param x vector X (I)
 * @param y vector Y (I)
 * @param z vector Z (I)
 * @return sqrt(x²+y²+z²), rounded (u16)
 *
 * Hardware square root — handy for homing missiles, audio attenuation,
 * anything that needs a true distance rather than a compare.
 */
u16 dsp1Distance(s16 x, s16 y, s16 z);

/**
 * @brief Sphere test (DSP-1 command $18, "Range").
 * @param x offset X from the sphere centre (I)
 * @param y offset Y (I)
 * @param z offset Z (I)
 * @param r sphere radius (I)
 * @return (x²+y²+z²) − r² : ≤ 0 means inside the sphere
 *
 * One call replaces three multiplies and two adds — cheap 3D proximity /
 * LOD tests without ever leaving 16-bit C.
 */
s16 dsp1Range(s16 x, s16 y, s16 z, u16 r);

/**
 * @brief Is a DSP-1 responding? Known-answer self-test.
 * @return 1 if the chip answered dsp1Multiply(0x4000, 0x4000) == 0x2000,
 *         0 if the status port never raised RQM or the product was wrong.
 *
 * Unlike the other calls this one cannot hang on a missing chip (bounded
 * poll). Use it at boot the way sa1IsReady()/superfxIsPresent() are used —
 * e.g. to fall back to software math when running on the wrong board or on
 * an emulator without the firmware.
 */
u16 dsp1Present(void);

#endif /* SNES_DSP1_H */
