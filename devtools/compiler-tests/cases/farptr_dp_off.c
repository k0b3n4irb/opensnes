/* farptr_dp with the rule off: staged in tcc__r9, stepped on 32 bits. */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

FAR u8 src[64];
FAR u8 dst[64];

void copy2(u16 n) {
    const u8 FAR *s = src;
    u8 FAR *d = dst;
    do {
        *d++ = *s++;
    } while (--n);
}
