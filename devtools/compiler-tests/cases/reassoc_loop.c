/* A constant added to a loop counter goes into the symbol (A6a R4): the
 * counter is bounded by the loop's own test, so i + k cannot wrap. */
typedef unsigned short u16;
#define FAR __far

FAR u16 fbuf[256];
u16 nbuf[64];

/* the fill unrolled by hand: one index, eight offsets */
void fill4(u16 v) {
    u16 i;
    for (i = 0; i < 256; i += 4) {
        fbuf[i] = v;
        fbuf[i + 1] = v;
        fbuf[i + 2] = v;
        fbuf[i + 3] = v;
    }
}

/* a plain array the same way */
void pairs(u16 v) {
    u16 i;
    for (i = 0; i < 64; i += 2) {
        nbuf[i] = v;
        nbuf[i + 1] = v + 1;
    }
}

/* a bound the loop does not prove: i + 1 may wrap, left alone */
void open_bound(u16 n, u16 v) {
    u16 i;
    for (i = 0; i < n; i += 2) {
        nbuf[i] = v;
        nbuf[i + 1] = v;
    }
}
