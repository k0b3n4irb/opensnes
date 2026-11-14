// The same for a source named l.c (`g.l`); the guard also refuses the upper
// case `.B`, `.W`, `.L` (a source B.c), not pinned here: b.c and B.c would
// be one file on a case-insensitive file system.
static unsigned g(unsigned x) { return x ^ 0x55; }
unsigned l_sum(unsigned x) { return g(x) + g(x >> 1); }
