// The peephole over the emitted text (issue #166, patterns 1, 2, 4).
// An index is `lda / asl a / sta / tax` once, and X serves the following
// accesses while nothing writes it; a value just stored is not reloaded.
unsigned int size_tab[19];
int xs[19], ys[19];

unsigned int three(unsigned int id, unsigned int cam) {
    unsigned int size = size_tab[id];
    unsigned int x = xs[id] - cam;
    unsigned int y = ys[id] - cam;
    return size + x + y;
}

// A store through a pointer may write an addressable local: the reload of
// `v` after it must stay.
unsigned int g;
static void bump(unsigned int *p) { *p += 1; }
unsigned int alias(unsigned int a) {
    unsigned int v = a;
    bump(&v);
    return v + g;
}
