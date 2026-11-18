/* eq_swap with the rule off: the old shape, A saved and the other
 * operand reloaded. */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

FAR u8 gen[192];
FAR u8 tv[192];
u8 g;
u16 acc, misses;

__attribute__((noinline)) void miss(u16 p) { misses += p; }

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
