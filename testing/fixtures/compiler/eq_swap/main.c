/*
 * An equality compares its operands either way round (2026-11-18, eq_swap
 * in the w65816 emitter): `a == b` / `a != b` whose b the line before left
 * in A is emitted `cmp a`, not `sta T0 / lda a / cmp T0`, and T0's store
 * goes when nothing reads it.
 *
 * Every function below runs for the 256 values of g against the 256 bytes
 * of gen[] (a permutation: each g meets its byte once):
 *   eq_loop()   gen[p] == g, else a call             taken (fused branch)
 *   ne_loop()   gen[p] != g, a leaf                  taken (fused branch)
 *   eq_val()    the 0 / 1 of gen[p] == g             taken (materialised)
 *   eq_reuse()  gen[p] == g, then g read again       taken; g's store stays
 *   lt_loop()   (u16)gen[p] < (u16)g                 NOT taken: an ordered
 *   ge_loop()   (u16)gen[p] >= (u16)g                compare keeps its place
 * Each result is compared with C written without the shape (an xor tested
 * for zero, the sign of a difference) and feeds a checksum per function,
 * which test_eq_swap.py computes again on the host.
 */
#include <snes.h>

FAR u8 gen[256];
FAR u8 tv[256];
u8 g;
u16 misses;

u16 bad_eq, bad_ne, bad_val, bad_reuse, bad_lt, bad_ge;
u16 h_eq, h_ne, h_val, h_reuse, h_lt, h_ge, h_miss;
u16 done;

__attribute__((noinline)) void miss(u16 p) { misses += p; }

__attribute__((noinline)) u16 eq_loop(void)
{
    u16 p, s = 0;
    for (p = 0; p < 256; p++) {
        if (gen[p] == g)
            s += tv[p];
        else
            miss(p);
    }
    return s;
}

__attribute__((noinline)) u16 ne_loop(void)
{
    u16 p, s = 0;
    for (p = 0; p < 256; p++) {
        if (gen[p] != g)
            s += tv[p];
        else
            s ^= p;
    }
    return s;
}

__attribute__((noinline)) u16 eq_val(u16 p) { return gen[p] == g; }

__attribute__((noinline)) u16 eq_reuse(u16 p)
{
    u16 a = gen[p], b = g;
    if (a == b)
        return b * 3u + 1u;
    return a + b;
}

__attribute__((noinline)) u16 lt_loop(void)
{
    u16 p, s = 0;
    for (p = 0; p < 256; p++) {
        if ((u16)gen[p] < (u16)g)
            s += tv[p];
        else
            s ^= p;
    }
    return s;
}

__attribute__((noinline)) u16 ge_loop(void)
{
    u16 p, s = 0;
    for (p = 0; p < 256; p++) {
        if ((u16)gen[p] >= (u16)g)
            s += tv[p];
        else
            s ^= p;
    }
    return s;
}

/* rotate by 5, xor, add (the checksum of the bswap fixture) */
static u16 mix(u16 h, u16 r) { return (u16)((((h << 5) | (h >> 11)) ^ r) + 0x9E37u); }

int main(void)
{
    u16 p, v, x, eq, lt, r_eq, r_ne, r_lt, r_ge, r_val, r_reuse;

    for (p = 0; p < 256; p++) {
        gen[p] = (u8)(p * 37u + 11u);
        tv[p] = (u8)(p * 13u + 5u);
    }
    v = 0;
    do {
        g = (u8)v;
        misses = 0;
        r_eq = eq_loop();
        r_ne = ne_loop();
        r_lt = lt_loop();
        r_ge = ge_loop();
        r_val = 0;
        r_reuse = 0;
        for (p = 0; p < 256; p++) {
            r_val = (u16)(r_val * 3u + eq_val(p));
            r_reuse = mix(r_reuse, eq_reuse(p));
        }
        h_eq = mix(h_eq, r_eq);
        h_miss = mix(h_miss, misses);
        h_ne = mix(h_ne, r_ne);
        h_lt = mix(h_lt, r_lt);
        h_ge = mix(h_ge, r_ge);
        h_val = mix(h_val, r_val);
        h_reuse = mix(h_reuse, r_reuse);
        /* the same, without the shape */
        {
            u16 s_eq = 0, s_ne = 0, s_lt = 0, s_ge = 0, s_val = 0, s_reuse = 0;
            for (p = 0; p < 256; p++) {
                x = (u16)gen[p];
                eq = !(x ^ v);
                lt = (u16)(x - v) >> 15;
                s_eq += eq ? tv[p] : 0;
                if (eq)
                    s_ne ^= p;
                else
                    s_ne += tv[p];
                if (lt)
                    s_lt += tv[p];
                else
                    s_lt ^= p;
                if (lt)
                    s_ge ^= p;
                else
                    s_ge += tv[p];
                s_val = (u16)(s_val * 3u + eq);
                s_reuse = mix(s_reuse, eq ? (u16)(v * 3u + 1u) : (u16)(x + v));
            }
            bad_eq += s_eq != r_eq;
            bad_ne += s_ne != r_ne;
            bad_lt += s_lt != r_lt;
            bad_ge += s_ge != r_ge;
            bad_val += s_val != r_val;
            bad_reuse += s_reuse != r_reuse;
        }
    } while (++v < 256);
    done = 0xBEEF;
    while (1) {
    }
    return 0;
}
