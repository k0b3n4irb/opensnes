/* The same fill with the loop rule off: the counter stays in its slot. */
typedef unsigned short u16;
extern u16 words[256];
void fill(u16 v) {
    u16 k;
    for (k = 0; k < 256; k++)
        words[k] = v;
}
