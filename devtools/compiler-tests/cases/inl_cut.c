/* The label of a copied block, cut where the assembler reads a size suffix
 * (I1 S1). Three levels of `static inline` copies: run() holds copies of f,
 * which hold copies of pair, which hold copies of bs. The block part of a
 * copied label is cut at 40 characters, and at the third level the cut falls
 * right after `.b` (of body), `.w` (of while_*) and `.l` (of logic_*): the
 * names' lengths and this file's stem (`inl_cut`, part of every static's
 * name) are chosen for it. The inliner adds `_` after such a cut. The runtime
 * twin is testing/fixtures/compiler/inline_label_cut (values, NMI, faults). */
static inline unsigned bs(unsigned x) {
    unsigned s = 0;
    while (x) {
        if ((x & 1) && s < 900)
            s += x;
        x >>= 1;
    }
    return s;
}
static inline unsigned pair(unsigned x) { return bs(x) + bs(x ^ 0x55); }
static inline unsigned f(unsigned x) { return pair(x) ^ pair(x + 3); }
unsigned run(unsigned x) { return f(x) + f(x * 7); }
