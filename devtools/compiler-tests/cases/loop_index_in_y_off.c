/* loop_index_in_y with the Y pass off: the counter stays in its slot. */
typedef unsigned char u8;
typedef unsigned short u16;

u16 tab[256];
u16 out[32];
u8 idx[32];

void lookup(void) {
    u16 c;
    for (c = 0; c < 32; c++)
        out[c] = tab[idx[c]];
}
