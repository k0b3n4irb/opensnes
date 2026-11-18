/*
 * The byte swap of a 16-bit word is one `xba` (2026-11-18, mark_bswap in
 * the w65816 emitter): `(u16)((w << 8) | (w >> 8))` reaches the emitter as
 * a shl by 8, a shr by 8 of the same word and an `or`, and is emitted as
 * the swap of the accumulator, without any range fact.
 *
 * Every function below runs on the 65 536 values of w (main's sweep):
 *   sw_lr()    (w << 8) | (w >> 8)                  the form, taken
 *   sw_rl()    (w >> 8) | (w << 8)                  the other order, taken
 *   sw_far()   the word read from a FAR byte array   taken
 *   sw_glob()  a global swapped in place             taken
 *   sw_s16()   ((u16)s << 8) | (s >> 8), s signed   NOT a swap: the shift
 *              of a negative s fills the high byte with ones; left alone
 *   sw_u32()   (x << 8) | (x >> 8), x 32 bits       NOT a swap; left alone
 *   sw_mask()  ((w & 0xFF) << 8) | ((w >> 8) & 0xFF) another idiom, left
 * Every form is compared, value by value, with what C says it is, built
 * without the shape (`(w & 0xFF) * 256 + w / 256` for the swap, its sign
 * fill for s16, both words for u32); the loop's own swap is
 * also tested for zero and for its sign (xba sets N and Z from the low
 * byte only: a branch must not trust them). Each result also feeds a
 * checksum per form, which test_bswap.py computes again on the host.
 */
#include <snes.h>

FAR u8 fbuf[4];
u16 gw;

u16 bad_lr, bad_rl, bad_far, bad_glob, bad_mask, bad_s16, bad_u32, bad_nz, bad_neg;
u16 h_lr, h_rl, h_far, h_glob, h_s16, h_u32lo, h_u32hi, h_mask;
u16 done;

__attribute__((noinline)) u16 sw_lr(u16 w) { return (u16)((w << 8) | (w >> 8)); }
__attribute__((noinline)) u16 sw_rl(u16 w) { return (u16)((w >> 8) | (w << 8)); }
__attribute__((noinline)) u16 sw_far(u16 o)
{
    u16 w = *(u16 FAR *)&fbuf[o];
    return (u16)((w << 8) | (w >> 8));
}
__attribute__((noinline)) void sw_glob(void) { gw = (u16)((gw << 8) | (gw >> 8)); }
__attribute__((noinline)) s16 sw_s16(s16 s) { return (s16)(((u16)s << 8) | (s >> 8)); }
__attribute__((noinline)) u32 sw_u32(u32 x) { return (x << 8) | (x >> 8); }
__attribute__((noinline)) u16 sw_mask(u16 w) { return (u16)(((w & 0xFF) << 8) | ((w >> 8) & 0xFF)); }

/* rotate by 5, xor, add: a checksum the structured faults do not cancel
 * (a sum weighted by powers of 31 summed to 0 over the whole domain) */
static u16 mix(u16 h, u16 r) { return (u16)((((h << 5) | (h >> 11)) ^ r) + 0x9E37u); }

int main(void)
{
    u16 w = 0, ref, r, hi, lo;
    u32 x;

    do {
        ref = (u16)((w & 0xFFu) * 256u + w / 256u);
        r = sw_lr(w);
        bad_lr += r != ref;
        h_lr = mix(h_lr, r);
        r = sw_rl(w);
        bad_rl += r != ref;
        h_rl = mix(h_rl, r);
        fbuf[1] = (u8)w;
        fbuf[2] = (u8)(w / 256u);
        r = sw_far(1);
        bad_far += r != ref;
        h_far = mix(h_far, r);
        gw = w;
        sw_glob();
        bad_glob += gw != ref;
        h_glob = mix(h_glob, gw);
        r = sw_mask(w);
        bad_mask += r != ref;
        h_mask = mix(h_mask, r);
        /* s >> 8 of a negative s fills the high byte with ones */
        r = (u16)sw_s16((s16)w);
        bad_s16 += r != ((w & 0x8000u) ? (u16)(0xFF00u + w / 256u) : ref);
        h_s16 = mix(h_s16, r);
        hi = w ^ 0xA5C3u;
        x = ((u32)hi << 16) + w;
        x = sw_u32(x);
        lo = (u16)(((w & 0xFFu) | (hi & 0xFFu)) * 256u + w / 256u);
        bad_u32 += (u16)x != lo;
        bad_u32 += (u16)(x >> 16) != (u16)(((hi & 0xFFu) * 256u + w / 256u) | (hi / 256u));
        h_u32lo = mix(h_u32lo, (u16)x);
        h_u32hi = mix(h_u32hi, (u16)(x >> 16));
        /* the loop's own swap, then a branch on its value */
        r = (u16)((w << 8) | (w >> 8));
        if (r)
            bad_nz += ref == 0;
        else
            bad_nz += ref != 0;
        if ((s16)r < 0)
            bad_neg += ref < 0x8000u;
        else
            bad_neg += ref >= 0x8000u;
    } while (++w);
    done = 0xBEEF;
    while (1) {
    }
    return 0;
}
