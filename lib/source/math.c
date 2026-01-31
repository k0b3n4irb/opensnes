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

fixed fixMul(fixed a, fixed b) {
    s32 result;

    /* 8.8 * 8.8 = 16.16, then shift down to 8.8 */
    result = (s32)a * (s32)b;
    return (fixed)(result >> 8);
}

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

fixed fixLerp(fixed a, fixed b, u8 t) {
    fixed diff;
    s32 scaled;

    diff = b - a;
    /* Scale diff by t/256 */
    scaled = (s32)diff * t;
    return a + (fixed)(scaled >> 8);
}
