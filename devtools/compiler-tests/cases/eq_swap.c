/* An equality compares its operands either way round (eq_swap, emitter). */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

FAR u8 gen[192];
FAR u8 tv[192];
u8 g;
u16 acc, misses;

__attribute__((noinline)) void miss(u16 p) { misses += p; }

/* the operand computed last (g) is in A: `cmp` the other, no reload */
void k(void)
{
    u16 p, s = 0;
    for (p = 0; p < 192; p += 3) {
        if (gen[p] == g)
            s += tv[p];
        else
            miss(p);
    }
    acc = s;
}

/* an unsigned < of the same shape: the operands keep their place */
void lt(void)
{
    u16 p, s = 0;
    for (p = 0; p < 192; p += 3) {
        if ((u16)gen[p] < (u16)g)
            s += tv[p];
        else
            miss(p);
    }
    acc = s;
}
