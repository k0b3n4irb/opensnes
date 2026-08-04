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
 * `dsp1b.rom` firmware installed (`~/.config/luna/firmware/` or `--dsp1-rom`).
 * It is copyrighted and not shipped; DSP-1 examples are firmware-gated in CI.
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

#endif /* SNES_DSP1_H */
