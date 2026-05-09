/**
 * @file math.c
 * @brief OpenSNES Fixed-Point Math Implementation
 *
 * Fixed-point arithmetic and trigonometry.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/math.h>

/*============================================================================
 * Sine Table (256 entries, 8.8 fixed-point)
 *
 * Values from 0 to 255 represent angles 0° to 360°.
 * Sine values range from -256 to 256 (-1.0 to 1.0 in 8.8).
 *============================================================================*/

static const s16 sine_table[256] = {
    /* 0-15 (0° to 21°) */
       0,    6,   13,   19,   25,   31,   37,   44,
      50,   56,   62,   68,   74,   80,   86,   92,
    /* 16-31 (22° to 44°) */
      97,  103,  109,  115,  120,  126,  131,  136,
     142,  147,  152,  157,  162,  167,  171,  176,
    /* 32-47 (45° to 66°) */
     181,  185,  189,  193,  197,  201,  205,  209,
     212,  216,  219,  222,  225,  228,  231,  234,
    /* 48-63 (67° to 89°) */
     236,  238,  241,  243,  245,  247,  248,  250,
     251,  252,  253,  254,  255,  255,  256,  256,
    /* 64-79 (90° to 111°) */
     256,  256,  256,  255,  255,  254,  253,  252,
     251,  250,  248,  247,  245,  243,  241,  238,
    /* 80-95 (112° to 134°) */
     236,  234,  231,  228,  225,  222,  219,  216,
     212,  209,  205,  201,  197,  193,  189,  185,
    /* 96-111 (135° to 156°) */
     181,  176,  171,  167,  162,  157,  152,  147,
     142,  136,  131,  126,  120,  115,  109,  103,
    /* 112-127 (157° to 179°) */
      97,   92,   86,   80,   74,   68,   62,   56,
      50,   44,   37,   31,   25,   19,   13,    6,
    /* 128-143 (180° to 201°) */
       0,   -6,  -13,  -19,  -25,  -31,  -37,  -44,
     -50,  -56,  -62,  -68,  -74,  -80,  -86,  -92,
    /* 144-159 (202° to 224°) */
     -97, -103, -109, -115, -120, -126, -131, -136,
    -142, -147, -152, -157, -162, -167, -171, -176,
    /* 160-175 (225° to 246°) */
    -181, -185, -189, -193, -197, -201, -205, -209,
    -212, -216, -219, -222, -225, -228, -231, -234,
    /* 176-191 (247° to 269°) */
    -236, -238, -241, -243, -245, -247, -248, -250,
    -251, -252, -253, -254, -255, -255, -256, -256,
    /* 192-207 (270° to 291°) */
    -256, -256, -256, -255, -255, -254, -253, -252,
    -251, -250, -248, -247, -245, -243, -241, -238,
    /* 208-223 (292° to 314°) */
    -236, -234, -231, -228, -225, -222, -219, -216,
    -212, -209, -205, -201, -197, -193, -189, -185,
    /* 224-239 (315° to 336°) */
    -181, -176, -171, -167, -162, -157, -152, -147,
    -142, -136, -131, -126, -120, -115, -109, -103,
    /* 240-255 (337° to 359°) */
     -97,  -92,  -86,  -80,  -74,  -68,  -62,  -56,
     -50,  -44,  -37,  -31,  -25,  -19,  -13,   -6,
};

/*============================================================================
 * Trigonometry Functions
 *============================================================================*/

fixed fixSin(u8 angle) {
    return sine_table[angle];
}

fixed fixCos(u8 angle) {
    /* cos(x) = sin(x + 90°) = sin(x + 64) */
    return sine_table[(u8)(angle + 64)];
}

/*============================================================================
 * Fixed-Point Arithmetic
 *============================================================================*/

/* fixMul is implemented in math_fixmul.asm using SNES hardware multiplier.
 * The C version overflows because the compiler reduces (s32)a*(s32)b to __mul16. */

fixed fixDiv(fixed a, fixed b) {
    s32 dividend;

    if (b == 0) return 0;

    /* Shift up dividend to get 8.8 result */
    dividend = (s32)a << 8;
    return (fixed)(dividend / b);
}

/*============================================================================
 * Integer Math (Safe Alternatives)
 *============================================================================*/

u16 mul16(u16 a, u16 b) {
    u16 result = 0;

    /* Use repeated addition for reliable multiplication */
    while (b > 0) {
        if (b & 1) {
            result = result + a;
        }
        a = a << 1;
        b = b >> 1;
    }

    return result;
}

u16 div16(u16 dividend, u16 divisor) {
    u16 quotient = 0;

    if (divisor == 0) return 0;

    /* Use repeated subtraction */
    while (dividend >= divisor) {
        dividend = dividend - divisor;
        quotient = quotient + 1;
    }

    return quotient;
}

u16 mod16(u16 dividend, u16 divisor) {
    if (divisor == 0) return 0;

    /* Use repeated subtraction */
    while (dividend >= divisor) {
        dividend = dividend - divisor;
    }

    return dividend;
}

/*============================================================================
 * Utility Functions
 *============================================================================*/

fixed fixAbs(fixed x) {
    if (x < 0) {
        return 0 - x;
    }
    return x;
}

fixed fixClamp(fixed x, fixed min, fixed max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/* fixLerp is implemented in math_fixmul.asm using SNES hardware multiplier.
 * The C version overflows because the compiler reduces (s32)diff*t to __mul16. */

/*============================================================================
 * Square Root and Inverse Trigonometry (chantier B6, 2026-05-09)
 *
 * sqrt16 / fixSqrt live in their own translation unit (math_sqrt.c) so
 * the hdma module can link just the sqrt family without dragging the
 * sine LUT and arithmetic helpers below into hdma-using examples.
 * See math_sqrt.c for the rationale.
 *============================================================================*/

/* atan(i / 64) for i = 0..64, scaled to the 8-bit angle convention
 * (full circle = 256). Computed once at design time (the SNES has no
 * floating-point unit). Each entry is `round(atan(i/64) / (π/2) * 64)`.
 *
 * The table covers the first octant (slope 0 → slope 1, angle 0 → 32);
 * the other 7 octants are reached by mirror / quadrant logic in
 * atan2_8 below. Total ROM cost: 65 bytes.
 */
static const u8 atan_lut[65] = {
     0,  1,  1,  2,  3,  4,  5,  6,
     6,  7,  8,  9, 10, 10, 11, 12,
    13, 14, 14, 15, 16, 17, 17, 18,
    19, 19, 20, 21, 21, 22, 23, 23,
    24, 24, 25, 25, 26, 26, 27, 27,
    27, 28, 28, 28, 29, 29, 29, 30,
    30, 30, 30, 31, 31, 31, 31, 31,
    31, 32, 32, 32, 32, 32, 32, 32,
    32,
};

u8 atan2_8(s16 dy, s16 dx) {
    u8 quadrant = 0;
    u16 udx, udy;
    u8 swap;
    u16 scaled_u16;
    u8 scaled, base;

    /* Sign extraction: bit 1 = dx<0, bit 0 = dy<0. */
    if (dx < 0) { quadrant |= 2; dx = (s16)(0 - dx); }
    if (dy < 0) { quadrant |= 1; dy = (s16)(0 - dy); }

    udx = (u16)dx;
    udy = (u16)dy;

    if (udx == 0 && udy == 0) return 0;  /* origin: angle undefined */

    /* atan2 is scale-invariant: dividing both inputs by the same
     * factor preserves the angle. Reduce magnitudes into u8 range
     * so the (udy << 6) intermediate below fits in u14 and the
     * subsequent /udx fits in 16-bit divide. */
    while (udx > 255 || udy > 255) {
        udx >>= 1;
        udy >>= 1;
    }

    /* Fold into the first octant: udy ≤ udx (slope ≤ 1). If we
     * had to swap, we'll mirror the LUT result around 32 (the
     * 45° boundary). */
    swap = 0;
    if (udy > udx) {
        u16 t = udx;
        udx = udy;
        udy = t;
        swap = 1;
    }

    /* udx is non-zero here (else the origin check above would have
     * fired), and udx ≤ 255, so the divisor fits in u8. The dividend
     * udy << 6 fits in u14 (udy ≤ 255 → udy*64 ≤ 16320). */
    scaled_u16 = (u16)(udy << 6);
    scaled = (u8)(scaled_u16 / udx);

    /* Look up the angle in the first octant. */
    base = atan_lut[scaled];

    /* Mirror around the 45° boundary if we swapped axes. */
    if (swap) base = (u8)(64 - base);

    /* Fold back into the correct quadrant. The 8-bit angle wraps
     * mod 256 naturally — using u8 arithmetic, `0 - base` is the
     * 8-bit two's complement = 256 - base, exactly what we want. */
    switch (quadrant) {
        case 0: return base;                  /* +dx, +dy */
        case 1: return (u8)(0 - base);        /* +dx, -dy → 256 - base */
        case 2: return (u8)(128 - base);      /* -dx, +dy */
        case 3: return (u8)(128 + base);      /* -dx, -dy */
    }
    return 0;
}
