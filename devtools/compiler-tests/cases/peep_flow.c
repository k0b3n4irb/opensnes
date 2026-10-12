/* A store to a direct-page slot that is not read after it goes, even when
 * the slot is read elsewhere in the function and a branch stands between
 * the store and the next write (2026-10-10, issue #166; emit.c,
 * peephole_flow). In a large function every slot is shared, so the rule
 * "nobody in the function reads this slot" never fires. */
typedef unsigned short u16;
typedef unsigned char u8;
extern u8 map[16384];
extern u16 tab[32], out[32];

/* the address of each store is computed, stored in a slot and used at
 * once through X: nothing reads the slot again */
void fill(u16 row) {
    u16 x;
    for (x = 0; x < 128; x++) {
        u8 t = (u8)((x ^ row) & 1);
        if ((row == 12 || row == 13) && (x & 4) && (x & 2) == 0)
            t = 2;
        map[row * 128 + x] = t;
    }
}

/* a value stored before a branch and read on one side only: it stays */
u16 keep(u16 i) {
    u16 a = tab[i] + 3;
    if (tab[i + 1] & 1)
        out[i] = a;
    return tab[i + 2];
}
