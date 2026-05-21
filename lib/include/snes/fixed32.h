/**
 * @file fixed32.h
 * @brief SNES 16.16 Fixed-Point Math
 *
 * 32-bit signed fixed-point arithmetic for world-space coordinates,
 * physics velocity accumulators, and any quantity that needs more range
 * or precision than the lib's 8.8 `fixed` type from `<snes/math.h>`.
 *
 * ## Overview
 *
 * - 16-bit integer part: -32768 to +32767 (enough for a 32k-tile world)
 * - 16-bit fractional part: 1/65536 ≈ 0.0000153 precision
 *
 * The lib's existing 8.8 `fixed` type (range ±128, precision 1/256) is
 * fine for screen-space coords and velocities. Use `fixed32` when:
 * - The world is larger than 128 tiles in any dimension.
 * - Accumulator drift over many frames matters (e.g. sub-pixel motion
 *   that must integrate without bias over thousands of frames).
 * - Two values multiplied risk overflowing 8.8 (e.g. velocity × time).
 *
 * @code
 * fixed32 angle = 0;
 * fixed32 omega = FIX32(1) / 360;       // 1°/frame in radians (~0.0028)
 * angle += omega;                        // free — just s32 += s32
 * fixed32 vx = fix32Mul(FIX32(2), fix32Sin(angle));
 * @endcode
 *
 * ## Operations available
 *
 * | Operation         | Helper           | Cost (cycles, approx) |
 * |-------------------|------------------|----------------------|
 * | Add/sub           | `+` / `-` on s32 | inline, 8-10         |
 * | Negate            | unary `-`        | inline               |
 * | Abs               | `fix32Abs`       | inline               |
 * | Clamp             | `fix32Clamp`     | inline               |
 * | Multiply          | `fix32Mul`       | ~280 (16 × 8x8 hw)   |
 *
 * Division, sin/cos, and lerp are deferred to follow-up chantiers
 * (see `.claude/notes/chantiers/b5_fix32_orbit_sketch.md`).
 *
 * ## Sign convention
 *
 * `fixed32` is signed two's complement s32. Negative values are stored
 * with the sign bit at position 31. Use `FIX32(-5)` for negative
 * integers; the bit pattern handles itself. `fix32Mul` handles signs
 * internally (XOR-then-negate-result).
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_FIXED32_H
#define OPENSNES_FIXED32_H

#include <snes/types.h>

/*============================================================================
 * Type and macros
 *============================================================================*/

/**
 * @brief 16.16 signed fixed-point type
 *
 * Range: -32768.0 to +32767.99998
 * Precision: 1/65536 ≈ 0.0000153
 */
typedef s32 fixed32;

/**
 * @brief Convert integer to fixed32 (shift left 16)
 * @param x Integer value (-32768 to +32767)
 * @return fixed32 representation
 *
 * @code
 * fixed32 pos = FIX32(1000);   // 1000.0
 * fixed32 half = FIX32(1) >> 1; // 0.5 (32768 in raw)
 * @endcode
 */
#define FIX32(x) ((fixed32)((s32)(x) << 16))

/**
 * @brief Convert fixed32 to integer (truncate toward zero)
 * @param x fixed32 value
 * @return Integer part as s16
 *
 * Truncation rounds toward zero for both signs (C99 semantics for
 * arithmetic right-shift on signed types is implementation-defined,
 * but the cc65816 backend implements arithmetic shift, so this works
 * as expected on this target).
 *
 * @code
 * fixed32 pos = FIX32(1000) + 32768;  // 1000.5
 * s16 screen = UNFIX32(pos);          // 1000
 * @endcode
 */
#define UNFIX32(x) ((s16)((fixed32)(x) >> 16))

/**
 * @brief Get fractional part of a fixed32 as a u16 (0..65535)
 * @param x fixed32 value
 * @return Low 16 bits (the fractional part)
 *
 * @code
 * fixed32 q = FIX32(1) >> 2;       // 0.25
 * u16 frac = FIX32_FRAC(q);        // 16384
 * @endcode
 */
#define FIX32_FRAC(x) ((u16)((fixed32)(x) & 0xFFFF))

/**
 * @brief Build a fixed32 from integer + fractional parts
 * @param i Integer part (s16)
 * @param f Fractional part (u16)
 * @return Combined fixed32
 *
 * @code
 * fixed32 v = FIX32_MAKE(100, 32768);  // 100.5
 * @endcode
 */
#define FIX32_MAKE(i, f) (((fixed32)(s16)(i) << 16) | (fixed32)(u16)(f))

/*============================================================================
 * Inline operations (no asm needed — compiler emits Kl arithmetic)
 *============================================================================*/

/**
 * @brief Absolute value of a fixed32
 * @param x fixed32
 * @return |x| (or INT32_MIN for INT32_MIN — the one-value undefined edge)
 *
 * The "undefined" edge is for x = -2^31 exactly, which has no positive
 * representation in s32. The function returns INT32_MIN unchanged in
 * that case, matching standard library `abs` behavior on overflow.
 */
inline fixed32 fix32Abs(fixed32 x) {
    return x < 0 ? -x : x;
}

/**
 * @brief Clamp x to [min, max]
 * @return min if x < min, max if x > max, else x
 *
 * Caller's responsibility to ensure min <= max.
 */
inline fixed32 fix32Clamp(fixed32 x, fixed32 min, fixed32 max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/**
 * @brief Minimum of two fixed32
 */
inline fixed32 fix32Min(fixed32 a, fixed32 b) {
    return a < b ? a : b;
}

/**
 * @brief Maximum of two fixed32
 */
inline fixed32 fix32Max(fixed32 a, fixed32 b) {
    return a > b ? a : b;
}

/*============================================================================
 * Multiply / divide — deferred
 *============================================================================*/

/* fix32Mul, fix32Div, fix32Sin, fix32Lerp are NOT YET AVAILABLE.
 *
 * They are blocked on a cc65816 compiler issue: the Kl (32-bit) return
 * convention is incomplete — `s32 foo() { return ...; }` only returns
 * the low 16 bits, with the high 16 reading uninitialised memory at the
 * caller. See `.claude/notes/chantiers/b5_fix32_orbit_sketch.md` and the
 * archived asm draft alongside it (b5_fix32mul_asm_draft.asm) for the
 * implementation plan once the compiler blocker is resolved.
 *
 * In the meantime, all the inline ops above (Add/Sub via `+` / `-`,
 * Abs, Clamp, Min, Max) work because the C compiler handles them
 * directly via stack slots / globals — no Kl return value crosses a
 * function boundary.
 */

#endif /* OPENSNES_FIXED32_H */
