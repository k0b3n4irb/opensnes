/**
 * @file math.h
 * @brief SNES Fixed-Point Math
 *
 * Fixed-point arithmetic for smooth movement and physics.
 *
 * ## Overview
 *
 * This module provides:
 * - 8.8 fixed-point type and macros
 * - Sine/cosine lookup tables (256 angles = full circle)
 * - Fixed-point multiplication
 * - Integer multiplication (safe alternative to compiler's *)
 *
 * ## Fixed-Point Format
 *
 * The `fixed` type is 16-bit signed (s16) in 8.8 format:
 * - High byte: integer part (-128 to 127)
 * - Low byte: fractional part (0-255, representing 0.0 to 0.996)
 *
 * @code
 * fixed pos_x = FIX(100);        // 100.0
 * fixed velocity = FIX(1) / 4;   // 0.25 (64 in fixed)
 * pos_x = pos_x + velocity;      // 100.25
 * s16 screen_x = UNFIX(pos_x);   // 100 (truncated)
 * @endcode
 *
 * ## Angles
 *
 * Angles are 8-bit values (0-255) representing 0-360 degrees:
 * - 0 = 0°, 64 = 90°, 128 = 180°, 192 = 270°
 *
 * @code
 * u8 angle = 64;                       // 90 degrees
 * fixed dx = fixSin(angle);            // 1.0 (256)
 * fixed dy = fixCos(angle);            // 0.0 (0)
 * player_x = player_x + fixMul(speed, dx);
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_MATH_H
#define OPENSNES_MATH_H

#include <snes/types.h>

/*============================================================================
 * Fixed-Point Type and Macros
 *============================================================================*/

/**
 * @brief 8.8 signed fixed-point type
 *
 * Range: -128.0 to 127.996 (approximately)
 * Precision: 1/256 = 0.00390625
 */
typedef s16 fixed;

/**
 * @brief Convert integer to fixed-point
 * @param x Integer value (-128 to 127)
 * @return Fixed-point representation
 *
 * @code
 * fixed pos = FIX(50);   // 50.0 in fixed-point
 * fixed half = FIX(1) / 2;  // 0.5
 * @endcode
 */
#define FIX(x) ((fixed)((x) << 8))

/**
 * @brief Convert fixed-point to integer (truncate)
 * @param x Fixed-point value
 * @return Integer part (truncated toward zero)
 *
 * @code
 * fixed pos = FIX(50) + 128;  // 50.5
 * s16 screen_pos = UNFIX(pos);  // 50
 * @endcode
 */
#define UNFIX(x) ((s16)((x) >> 8))

/**
 * @brief Convert fixed-point to integer (rounded)
 * @param x Fixed-point value
 * @return Integer part (rounded to nearest)
 *
 * @code
 * fixed pos = FIX(50) + 128;  // 50.5
 * s16 screen_pos = UNFIX_ROUND(pos);  // 51
 * @endcode
 */
#define UNFIX_ROUND(x) ((s16)(((x) + 128) >> 8))

/**
 * @brief Get fractional part of fixed-point
 * @param x Fixed-point value
 * @return Fractional part (0-255)
 */
#define FIX_FRAC(x) ((u8)((x) & 0xFF))

/**
 * @brief Create fixed-point from integer and fraction
 * @param i Integer part
 * @param f Fractional part (0-255)
 * @return Fixed-point value
 *
 * @code
 * fixed half = FIX_MAKE(0, 128);  // 0.5
 * fixed quarter = FIX_MAKE(0, 64);  // 0.25
 * @endcode
 */
#define FIX_MAKE(i, f) ((fixed)(((i) << 8) | (f)))

/*============================================================================
 * Fixed-Point Arithmetic
 *============================================================================*/

/**
 * @brief Multiply two fixed-point values
 *
 * @param a First fixed-point operand
 * @param b Second fixed-point operand
 * @return Product in fixed-point format
 *
 * @code
 * fixed speed = FIX(2);
 * fixed scale = FIX(1) / 2;  // 0.5
 * fixed result = fixMul(speed, scale);  // 1.0
 * @endcode
 *
 * @note Uses 32-bit intermediate for accuracy
 */
fixed fixMul(fixed a, fixed b);

/**
 * @brief Divide two fixed-point values
 *
 * @param a Dividend (fixed-point)
 * @param b Divisor (fixed-point, must not be zero)
 * @return Quotient in fixed-point format
 *
 * @code
 * fixed distance = FIX(100);
 * fixed time = FIX(5);
 * fixed speed = fixDiv(distance, time);  // 20.0
 * @endcode
 *
 * @warning Division by zero returns 0
 */
fixed fixDiv(fixed a, fixed b);

/*============================================================================
 * Trigonometry (Lookup Tables)
 *============================================================================*/

/**
 * @brief Get sine value for angle
 *
 * @param angle 8-bit angle (0-255 = 0°-360°)
 * @return Sine value in 8.8 fixed-point (-256 to 256, i.e., -1.0 to 1.0)
 *
 * @code
 * u8 angle = 64;  // 90 degrees
 * fixed sin_val = fixSin(angle);  // 256 = 1.0
 * @endcode
 *
 * @note Table-based lookup, very fast
 */
extern const s16 sine_table[256];

inline fixed fixSin(u8 angle) {
    return sine_table[angle];
}

/**
 * @brief Get cosine value for angle
 *
 * @param angle 8-bit angle (0-255 = 0°-360°)
 * @return Cosine value in 8.8 fixed-point (-256 to 256)
 *
 * @code
 * u8 angle = 0;  // 0 degrees
 * fixed cos_val = fixCos(angle);  // 256 = 1.0
 * @endcode
 * Inlined for zero-call-overhead access (wave 4 retrofit).
 */
inline fixed fixCos(u8 angle) {
    /* cos(x) = sin(x + 90°) = sin(x + 64) */
    return sine_table[(u8)(angle + 64)];
}

/*============================================================================
 * Integer Math (Safe Alternatives)
 *============================================================================*/

/**
 * @brief Safe 16-bit multiplication
 *
 * Multiplies two 16-bit values safely. Use this instead of the
 * compiler's * operator for important calculations, as the compiler's
 * runtime multiplication can have bugs.
 *
 * @param a First operand
 * @param b Second operand
 * @return Product (may overflow for large values)
 *
 * @code
 * u16 result = mul16(width, height);
 * @endcode
 */
u16 mul16(u16 a, u16 b);

/**
 * @brief Safe 16-bit division
 *
 * @param dividend Number to divide
 * @param divisor Number to divide by (must not be zero)
 * @return Quotient
 *
 * @warning Returns 0 if divisor is 0
 */
u16 div16(u16 dividend, u16 divisor);

/**
 * @brief Get remainder of division
 *
 * @param dividend Number to divide
 * @param divisor Number to divide by (must not be zero)
 * @return Remainder
 */
u16 mod16(u16 dividend, u16 divisor);

/*============================================================================
 * Utility Functions
 *============================================================================*/

/**
 * @brief Absolute value of fixed-point
 *
 * @param x Fixed-point value
 * @return Absolute value
 */
fixed fixAbs(fixed x);

/**
 * @brief Clamp value to range
 *
 * @param x Value to clamp
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
fixed fixClamp(fixed x, fixed min, fixed max);

/**
 * @brief Linear interpolation
 *
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor (0-256 = 0.0-1.0)
 * @return Interpolated value
 *
 * @code
 * fixed start = FIX(0);
 * fixed end = FIX(100);
 * fixed mid = fixLerp(start, end, 128);  // 50.0
 * @endcode
 */
fixed fixLerp(fixed a, fixed b, u8 t);

/*============================================================================
 * Square Root and Inverse Trigonometry
 *============================================================================*/

/**
 * @brief Integer square root
 *
 * Computes `floor(sqrt(n))` for any 16-bit unsigned input. Uses the
 * canonical bit-by-bit (digit-by-digit) algorithm — no LUT, no
 * floating point, fully deterministic in cycle count (~80 cycles
 * worst case on the 65816).
 *
 * @param n Input value (0 to 65535)
 * @return Largest integer whose square is ≤ n. Range: 0 to 255.
 *
 * @code
 * u16 dx = enemy_x - player_x;
 * u16 dy = enemy_y - player_y;
 * u16 dist = sqrt16(dx * dx + dy * dy);  // pixel distance
 * @endcode
 *
 * @note Range guarantee: `sqrt16(n)` is always ≤ 255, so the result
 *       fits in `u8`. Useful when distance is bounded by screen size.
 *
 * @see fixSqrt for the 8.8 fixed-point variant.
 */
u16 sqrt16(u16 n);

/**
 * @brief Square root in 8.8 fixed-point
 *
 * Computes the square root of an 8.8 fixed-point value. Internally
 * delegates to `sqrt16(x)` and shifts the result by 4 bits to
 * recover 5 bits of fractional precision in the answer (the
 * remaining 3 bits are zero — this is a lib-side limit, not a
 * mathematical one; precision can be raised once the QBE 32-bit
 * codegen lands — see chantier A7 in the structural-defects
 * catalogue).
 *
 * @param x Input value in 8.8 fixed-point. Must be ≥ 0; negative
 *          inputs return 0.
 * @return `sqrt(x)` in 8.8 fixed-point.
 *
 * @code
 * fixed area = FIX(64);     // 64.0
 * fixed side = fixSqrt(area);  // 8.0 (FIX(8))
 * @endcode
 *
 * @note For `x < 0`, returns 0 (no error signalling — sqrt of
 *       negative is undefined; the lib chooses defined-but-zero
 *       over throwing through the simple API).
 */
fixed fixSqrt(fixed x);

/**
 * @brief 8-bit two-argument arctangent
 *
 * Returns the angle of the vector (dx, dy) measured from the
 * positive X axis, in the SDK's 8-bit angle convention (0–255 =
 * 0°–360°, same scale as `fixSin` / `fixCos`).
 *
 * - 0 → +X (east, dy=0, dx>0)
 * - 64 → +Y (south on a screen, dy>0, dx=0)
 * - 128 → −X (west)
 * - 192 → −Y (north)
 *
 * Implementation: 65-entry LUT covering the first octant
 * (`atan(t)` for `t ∈ [0, 1]` mapped to angle `[0, 32]`), with
 * symmetry handling for the other 7 octants. Inputs are reduced
 * by power-of-two right shifts when their magnitudes exceed 255,
 * so the function works for any 16-bit signed input without
 * overflowing the internal 16-bit divide.
 *
 * @param dy Y component of the vector
 * @param dx X component of the vector
 * @return 8-bit angle of `(dx, dy)`. Returns 0 if both inputs
 *         are zero (the angle is mathematically undefined; the lib
 *         chooses defined-but-zero).
 *
 * @code
 * s16 dy = enemy_y - player_y;
 * s16 dx = enemy_x - player_x;
 * u8 aim_angle = atan2_8(dy, dx);
 * fire_projectile(aim_angle);
 * @endcode
 *
 * @note Precision is ≤ 1 angle unit (≈ 1.4°) for inputs whose
 *       magnitudes fit in 8 bits; coarser for larger inputs after
 *       reduction (still useful for game-side aiming logic).
 */
u8 atan2_8(s16 dy, s16 dx);

#endif /* OPENSNES_MATH_H */
