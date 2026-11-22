/* a_across_blocks with the rule off: the loads stay. */
typedef unsigned short u16;
#define FAR __far

FAR u16 fbuf[256];
u16 fval;

void fill(void) {
    u16 i, v = fval;
    for (i = 0; i < 256; i++)
        fbuf[i] = v;
    fval = v + 0x0101;
}
