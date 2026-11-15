/* path with the exact idioms I1-I4 said in C (for the current generator) */
void k_pathid_c(void) {
    u16 l0 = pa_lo[0], l1 = pa_lo[1], l2 = pa_lo[2], l3 = pa_lo[3];
    u16 h0 = pa_hi[0], h1 = pa_hi[1], h3 = pa_hi[3];
    u32 p2 = ((u32)pa_hi[2] << 16) | pa_lo[2];
    u16 idx2 = (u16)(pa_idx * 2), step2 = (u16)(pa_step * 2), n;
    (void)l2;
    for (n = NSUB; n; n--) {
        s16 s = *(const s16 *)((const u8 *)path_sin + idx2);
        u16 us = (u16)s, m = (u16)(s >> 15), t, o;
        idx2 = (idx2 + step2) & 126;
        o = l0; l0 += us; h0 += m + (l0 < o);
        o = l1; l1 -= us; h1 -= m + (o < us);
        p2 += (u32)((s32)s * 2);
        t = (u16)(s >> 1); o = l3; l3 += t; h3 += m + (l3 < o);
    }
    pa_hi[0] = h0; pa_lo[0] = l0;
    pa_hi[1] = h1; pa_lo[1] = l1;
    pa_hi[2] = p2 >> 16; pa_lo[2] = p2;
    pa_hi[3] = h3; pa_lo[3] = l3;
    pa_idx = idx2 >> 1;
}
