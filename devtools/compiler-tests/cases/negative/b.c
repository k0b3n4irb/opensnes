// A file-scope static of a source named b.c is emitted `g.b`, and the
// assembler reads `jsl g.b` as `jsl g` with a size suffix. Until 2026-11-14
// this failed the link ("unknown label g") or, with a global g in another
// file, linked and called that g. The emitter now refuses it (I1 S1).
static unsigned g(unsigned x) { return x * 3; }
unsigned b_sum(unsigned x) { return g(x) + g(x + 1); }
