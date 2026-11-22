/* ---- path: a path follower. Four positions of 32 bits (each kept as a
 * high and a low half, composed as high << 16 | low), moved NSUB sub-steps
 * a call by a sine read from a ROM table at an index that turns round it.
 * A leaf: no call. ------------------------------------------------------ */

#define NSUB 64

u16 pa_hi[4], pa_lo[4];
u16 pa_idx, pa_step;
const s16 path_sin[64] = {0, 294, 585, 871, 1148, 1414, 1667, 1903, 2121, 2319, 2494, 2646, 2772, 2871, 2942, 2986, 3000, 2986, 2942, 2871, 2772, 2646, 2494, 2319, 2121, 1903, 1667, 1414, 1148, 871, 585, 294, 0, -294, -585, -871, -1148, -1414, -1667, -1903, -2121, -2319, -2494, -2646, -2772, -2871, -2942, -2986, -3000, -2986, -2942, -2871, -2772, -2646, -2494, -2319, -2121, -1903, -1667, -1414, -1148, -871, -585, -294};

void s_path(void) {
    u16 i;
    rng_state = 11;
    for (i = 0; i < 4; i++) {
        pa_hi[i] = rng() & 1023;
        pa_lo[i] = rng();
    }
    pa_idx = 3;
    pa_step = 5;
}

void k_path_c(void) {
    u32 p0 = ((u32)pa_hi[0] << 16) | pa_lo[0];
    u32 p1 = ((u32)pa_hi[1] << 16) | pa_lo[1];
    u32 p2 = ((u32)pa_hi[2] << 16) | pa_lo[2];
    u32 p3 = ((u32)pa_hi[3] << 16) | pa_lo[3];
    u16 idx = pa_idx, step = pa_step, n;
    for (n = 0; n < NSUB; n++) {
        s16 s = path_sin[idx];
        idx = (idx + step) & 63;
        p0 += (u32)(s32)s;
        p1 -= (u32)(s32)s;
        p2 += (u32)((s32)s * 2);
        p3 += (u32)(s32)(s >> 1);
    }
    pa_hi[0] = p0 >> 16; pa_lo[0] = p0;
    pa_hi[1] = p1 >> 16; pa_lo[1] = p1;
    pa_hi[2] = p2 >> 16; pa_lo[2] = p2;
    pa_hi[3] = p3 >> 16; pa_lo[3] = p3;
    pa_idx = idx;
}

u16 c_path(void) {
    return sum((const u8 *)pa_hi, sizeof(pa_hi)) + sum((const u8 *)pa_lo, sizeof(pa_lo)) + pa_idx;
}
