/* reassoc_loop with the loop bound unused: each access computes its index. */
typedef unsigned short u16;
#define FAR __far

FAR u16 fbuf[256];

void fill4(u16 v) {
    u16 i;
    for (i = 0; i < 256; i += 4) {
        fbuf[i] = v;
        fbuf[i + 1] = v;
        fbuf[i + 2] = v;
        fbuf[i + 3] = v;
    }
}
