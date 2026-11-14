/* Lot E1: a far pointer read in its direct-page slot, stepped on 16 bits. */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

FAR u8 src[64];
FAR u8 dst[64];
extern FAR u8 ext[64];

/* two pointers walked in turn from objects of this unit: both read in place,
 * both stepped on their low word */
void copy2(u16 n) {
    const u8 FAR *s = src;
    u8 FAR *d = dst;
    do {
        *d++ = *s++;
    } while (--n);
}

/* compared with its end: the 32-bit step stays */
u16 cmp_walk(const u8 FAR *end) {
    const u8 FAR *p;
    u16 s = 0;
    for (p = src; p < end; p++)
        s += *p;
    return s;
}

/* an extern object (assembly may place it across a bank): 32 bits */
u16 ext_walk(void) {
    const u8 FAR *p = ext;
    u16 i, s = 0;
    for (i = 0; i < 16; i++)
        s += *p++;
    return s;
}

/* a parameter: 32 bits */
u16 par_walk(const u8 FAR *p) {
    u16 i, s = 0;
    for (i = 0; i < 16; i++)
        s += *p++;
    return s;
}
