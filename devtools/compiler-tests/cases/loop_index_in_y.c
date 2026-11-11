/* The counter of a rotated loop in Y when X is taken (A6a R2). */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

u16 tab[256];
u16 out[32];
u8 idx[32];
FAR u8 fmap[4096];

/* X carries a table index: the counter goes to Y, `ldx S` becomes tyx */
void lookup(void) {
    u16 c;
    for (c = 0; c < 32; c++)
        out[c] = tab[idx[c]];
}

/* a far pointer indexed by the counter: `lda S / tay` goes */
void walk(u8 FAR *p) {
    u16 c;
    for (c = 0; c < 32; c++)
        out[c] = tab[p[c]];
}

/* base + counter: `lda B / clc / adc S` becomes `tya / clc / adc B` */
void based(u16 o) {
    u16 c;
    for (c = 0; c < 32; c++)
        out[c] = tab[fmap[o + c]];
}

extern void sink(u16 v);

/* a call in the body: neither register */
void calls(void) {
    u16 c;
    for (c = 0; c < 32; c++)
        sink(tab[idx[c]]);
}
