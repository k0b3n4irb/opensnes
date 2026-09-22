/*
 * libtest_dsp1 — third runtime fixture (coverage lot D): the five DSP-1
 * commands no example calls. Expected values come from the arithmetic the
 * header documents; luna runs the real firmware, so a pass means the
 * wrapper, the chip protocol and the documentation agree.
 */
#include <snes.h>
#include <snes/dsp1.h>
/* dsp1Present is the deprecated name of dsp1IsPresent; it keeps its vector. */
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

volatile u16 dsp1_ok;   /* dsp1IsPresent()                              -> 1 */
volatile u16 dsp1_ok_old; /* dsp1Present(), the deprecated alias        -> 1 */
u16 r_mul;       /* dsp1Multiply(0x4000, 0x4000): 0.5 x 0.5 in 1.15     -> 0x2000 */
u16 r_mul_sign;  /* the product is an s16: (-0.5 x 0.5) < 0                -> 1 (always 0 while it was u16) */
u16 r_mul_neg;   /* dsp1Multiply(0xC000, 0x4000): -0.5 x 0.5            -> 0xE000 */
/* Distance reads ONE LOW on exact lengths (measured; see dsp1.h). */
u16 r_dist;      /* dsp1Distance(3, 4, 12): 13 exactly                  -> 12 */
u16 r_dist_mid;  /* dsp1Distance(300, 400, 0): 500                      -> 499 */
u16 r_dist_big;  /* dsp1Distance(0, 0, 10000)                           -> 9999 */
/* Range is ((x2+y2+z2) - r2) >> 15, arithmetic (measured; see dsp1.h). */
u16 r_range_out; /* dsp1Range(3000, 4000, 0, 1000): 24e6 >> 15          -> 732 */
u16 r_range_in;  /* dsp1Range(0, 0, 0, 1000): -1e6 >> 15                -> -31 */
u16 r_range_on;  /* dsp1Range(3000, 4000, 0, 5000): on the surface      -> 0 */
u16 r_range_sm;  /* dsp1Range(30, 40, 0, 5): far outside, small units   -> 0 */
u16 r_rot_x;     /* dsp1Rotate(0x4000, 100, 0)                          -> 0 */
u16 r_rot_y;     /* ... sin 90 deg is 0x7FFF, not 1.0                   -> -99 */
u16 r_tgt_x;     /* dsp1Target(0, 0) == Cx of the last dsp1Parameter    -> 1 */
u16 r_tgt_y;     /* ... and Cy                                          -> 1 */
u16 r_done;      /*                                                     -> 0xBEEF */

int main(void) {
    s16 cx, cy;

    consoleInit();
    dsp1Init();
    dsp1_ok = dsp1IsPresent();
    dsp1_ok_old = dsp1Present();       /* the deprecated name: same routine, same answer */

    r_mul       = (u16)dsp1Multiply(0x4000, 0x4000);
    r_mul_neg   = (u16)dsp1Multiply(-0x4000, 0x4000);
    r_mul_sign  = (dsp1Multiply(-0x4000, 0x4000) < 0) ? 1 : 0;
    r_dist      = dsp1Distance(3, 4, 12);
    r_dist_mid  = dsp1Distance(300, 400, 0);
    r_dist_big  = dsp1Distance(0, 0, 10000);
    r_range_out = (u16)dsp1Range(3000, 4000, 0, 1000);
    r_range_in  = (u16)dsp1Range(0, 0, 0, 1000);
    r_range_on  = (u16)dsp1Range(3000, 4000, 0, 5000);
    r_range_sm  = (u16)dsp1Range(30, 40, 0, 5);
    dsp1Rotate(0x4000, 100, 0);
    r_rot_x = (u16)dsp1_o0;
    r_rot_y = (u16)dsp1_o1;

    /* the dsp1_ground camera: Target(0,0) must give back its Cx / Cy */
    dsp1Parameter(512, 512, 96, 192, 256, 0, 0x1800);
    cx = dsp1_o2;
    cy = dsp1_o3;
    dsp1Target(0, 0);
    r_tgt_x = (dsp1_o0 == cx) ? 1 : 0;
    r_tgt_y = (dsp1_o1 == cy) ? 1 : 0;

    setScreenOn();
    r_done = 0xBEEF;
    while (1) { WaitForVBlank(); }
    return 0;
}
