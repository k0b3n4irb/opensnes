/* A load of what A already holds goes, across blocks (A6a R3). */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

FAR u16 fbuf[256];
u16 fval;
u16 tab[256], out[32];
u8 idx[32];

/* the value stays in A for the whole loop: the body is the store alone */
void fill(void) {
    u16 i, v = fval;
    for (i = 0; i < 256; i++)
        fbuf[i] = v;
    fval = v + 0x0101;
}

/* `sta S / tyx / lda S`: the reload across the transfer goes, its flags
 * set again by the step after the block label */
FAR u8 map[64 * 64];
void column(u16 o) {
    u16 r;
    u8 FAR *p = &map[o];
    for (r = 0; r < 32; r++) {
        out[r] = tab[*p];
        p += 64;
    }
}
