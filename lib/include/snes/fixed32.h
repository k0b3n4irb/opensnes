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
#include <snes/math.h>   /* for fixSin / fixCos (8.8 LUT) */

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
/* Cast through u32 before shifting to avoid the C UB on left-shifting a
 * negative signed value. Final cast back to fixed32 preserves bit pattern. */
#define FIX32(x) ((fixed32)((u32)(s32)(x) << 16))

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
 * ASM-backed multiply
 *============================================================================*/

/**
 * @brief 16.16 fixed-point multiply
 * @param a First operand
 * @param b Second operand
 * @return (a * b) at 16.16 precision (bits 16-47 of the full 64-bit product)
 *
 * Algorithm: three 16×16→32 unsigned partial products (a_l*b_l, a_l*b_h,
 * a_h*b_l) + one 16×16→16 (low 16 of a_h*b_h) combined as
 *   result = ml1 + ml2 + (ll >> 16) + (hh_lo << 16)
 * Each 16×16→32 uses 4 hardware 8×8 multiplies — ~280 cycles total.
 *
 * Sign-magnitude internally: result = sign(a) XOR sign(b) applied to
 * the unsigned 32-bit magnitude. Overflow wraps modulo 2^32.
 *
 * @code
 * fixed32 area = fix32Mul(FIX32(width), FIX32(height));
 * fixed32 dy = fix32Mul(velocity, FIX32(dt));
 * @endcode
 */
fixed32 fix32Mul(fixed32 a, fixed32 b);

/**
 * @brief 16.16 fixed-point divide
 * @param a Numerator
 * @param b Denominator (must be non-zero; division by zero is undefined)
 * @return (a / b) at 16.16 precision (low 32 bits of (a << 16) / b)
 *
 * Algorithm: 48-iteration bit-by-bit long divide of (|a| << 16) by |b|,
 * with sign-magnitude handling. The 48-bit dividend doesn't fit in a
 * single 32-bit register, so we can't reuse tcc_udivmod32 directly —
 * the custom loop processes one quotient bit per iteration with an
 * 80-bit working register.
 *
 * Cycles: ~1500 (much slower than fix32Mul; use sparingly in hot loops).
 *
 * @code
 * fixed32 velocity = fix32Div(distance, time);
 * fixed32 ratio = fix32Div(width, FIX32(2));  // halve width
 * @endcode
 */
fixed32 fix32Div(fixed32 a, fixed32 b);

/**
 * @brief 16.16 fixed-point linear interpolation
 * @param a Start value (returned when t = 0)
 * @param b End value (returned when t = FIX32(1))
 * @param t Interpolation parameter in 16.16, typically [0, FIX32(1)]
 * @return a + (b - a) * t at 16.16 precision
 *
 * Inline: just `a + fix32Mul(b - a, t)`. No new asm — the multiply
 * carries the cost (~280 cycles), the add is one Kl op.
 *
 * Extrapolation is supported: t > FIX32(1) extends past b, t < 0
 * extends before a. The caller is responsible for clamping if a
 * strict interpolation is needed.
 *
 * Precision caveat: when (b - a) approaches the fix32 range limit
 * (close to ±32768), the intermediate multiply may lose precision
 * at the bottom of the fractional part. For tight cases, prefer
 * direct `a*(1-t) + b*t` formulation (one extra mul, no subtraction
 * round-trip).
 *
 * @code
 * fixed32 midpoint = fix32Lerp(p0, p1, FIX32(1) >> 1);  // (p0 + p1) / 2
 * fixed32 eased = fix32Lerp(start, end, easing_curve_t);
 * @endcode
 */
inline fixed32 fix32Lerp(fixed32 a, fixed32 b, fixed32 t) {
    return a + fix32Mul(b - a, t);
}

/*============================================================================
 * Trigonometry (lifted from 8.8 LUT, 8 fractional bits of precision)
 *============================================================================*/

/**
 * @brief 16.16 fixed-point sine of an 8-bit angle (0..255 = 0..360°)
 * @param angle 0=0°, 64=90°, 128=180°, 192=270°
 * @return sin(angle) in 16.16, range [FIX32(-1), FIX32(1)]
 *
 * Lifted from the existing 8.8 `fixSin` LUT by shifting left 8 bits to
 * fill the upper half of the 16-bit fractional field. The lower 8 bits
 * are always zero (no precision gained beyond what the 8.8 LUT provides).
 * Costs: one LUT lookup + sign-extend + shift — about 30 cycles total.
 *
 * Precision: each LUT step is 1/256 ≈ 0.0039, so for fine animation
 * (sub-pixel motion over many frames) this is adequate. For high-
 * precision physics that compound thousands of operations, the
 * 8-bit-fractional limit may show as drift; a future chantier could
 * add a 16-bit LUT for a 256× precision improvement at 512 bytes ROM.
 *
 * @code
 * fixed32 dx = fix32Mul(speed, fix32Sin(angle));
 * @endcode
 */
/* Implemented in lib/source/fixed32.asm. Two qbe codegen bugs make the
 * one-line C body `(u32)(s32)fixSin(angle) << 8` produce wrong results:
 *
 *   1. Kl shift-by-constant spill (fixed in qbe 2026-05-22): the high
 *      half was computed from an unstored stack slot. Resolved.
 *
 *   2. Kw → Kl widening sign assumption (UNFIXED): ref_is_high_zero()
 *      treats all Kw operands as zero-extended. Wrong for signed
 *      s16 → s32 — produces 0x00FF0000 instead of 0xFFFF0000 for
 *      sin(270°) = -1.
 *
 * The asm form sign-extends explicitly and avoids both. See
 * lib/source/math.c for the full notes. */
fixed32 fix32Sin(u8 angle);
fixed32 fix32Cos(u8 angle);

/**
 * @brief 16.16 fixed-point cosine of an 8-bit angle
 * @see fix32Sin (same precision/cost characteristics)
 *
 * @code
 * fixed32 dy = fix32Mul(speed, fix32Cos(angle));
 * @endcode
 */
/* fix32Cos declared above with fix32Sin — both bodies in math.c. */

#endif /* OPENSNES_FIXED32_H */
