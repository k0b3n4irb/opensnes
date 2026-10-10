// The frame of a leaf function is in the direct page: a function that calls
// nothing, has no object on the stack and needs at most 16 words keeps its
// temps at tcc__lf instead of reserving a stack frame. No prologue, no
// epilogue, `lda.b` for `lda n,s`, nothing below the return address.
//
// It cannot be re-entered: it calls nothing, the NMI handler runs on its own
// direct page (which mirrors tcc__lf), and an IRQ handler is assembly.
unsigned short tab[16];
unsigned short other(unsigned short);

// a leaf: direct-page frame
unsigned short leaf(unsigned short a, unsigned short b) {
    unsigned short i, s = 0;
    for (i = a; i < b; i++) s += tab[i & 15];
    return s;
}

// calls something: a stack frame (the callee may be a leaf using tcc__lf)
unsigned short caller(unsigned short a, unsigned short b) {
    unsigned short s = other(a);
    s += other(b);
    return s + a;
}

// a local array has an address: it lives on the stack, and so does the rest
unsigned short with_array(unsigned short a) {
    unsigned short buf[4];
    unsigned short i;
    for (i = 0; i < 4; i++) buf[i] = a + i;
    return buf[a & 3];
}
