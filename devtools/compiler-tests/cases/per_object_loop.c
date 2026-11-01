/* The per-object loop: a structure of word arrays, two indices, a value
 * chosen by a branch, an early exit between two tests. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
s16 px[8], py[8];
u16 ta[8], big[8], slot_big[8];
u8 shadow[40];
u16 size_calls;

void place(u16 n, u16 cx, u16 pr) {
    u16 i, id, w;
    s16 sx, sy;
    for (i = 0, id = 0; i < n; i++, id += 4) {
        w = ta[i];
        if (!(w & 0x3000))
            w |= pr;
        *(u16 *)(shadow + id + 2) = w;
        if ((w = big[i]) != slot_big[i]) {
            slot_big[i] = w;
            size_calls++;
        }
        sx = px[i] - cx;
        sy = py[i];
        if ((u16)(sx + 31) >= 287 || (u16)(sy + 31) >= 255)
            *(u16 *)(shadow + id) = 224 << 8;
        else
            *(u16 *)(shadow + id) = (u8)sx | (sy << 8);
    }
}
