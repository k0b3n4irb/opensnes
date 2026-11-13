// The operand of a compare or an arithmetic operation taken in place
// (2026-11-13): `lda SRC / sta T / lda Y / [clc|sec] / op T`, T a direct-page
// slot dead after, becomes `lda Y / [clc|sec] / op SRC` for an immediate, a
// frame slot or a named symbol, as it already did for an X-indexed symbol
// (fuse_indexed). fuse_src_off is the same source with the rule turned off.
unsigned int g_free, g_ceil, out;
unsigned int tab[16];

/* not static: a static function with one call site is inlined */
unsigned int room(unsigned int need, unsigned int i) {
    unsigned int top = tab[i & 15] + need;
    if (top > g_free)
        return top - g_free;
    return 0;
}

void bump(unsigned int *p) { *p += 1; }

unsigned int after_call(unsigned int a) {
    unsigned int v = a;
    bump(&v);
    return out + v;
}

unsigned int cmp_ceil(unsigned int a, unsigned int b) {
    unsigned int s = a * 3 + b;
    return s < g_ceil ? s : g_ceil;
}
