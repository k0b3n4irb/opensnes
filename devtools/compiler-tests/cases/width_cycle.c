/* `(u8)v` after a loop that shifts a signed char right keeps its mask.
 * QBE proved "v fits 8 bits" around the loop by assuming it, and reused
 * that assumption for the narrower question the sign extension asks
 * ("v fits 7 bits"): the extub went, and a negative v reached the xor
 * with its high byte set (difftest_stmt seed 241214, 2026-10-10;
 * copy.c, defwidthle). */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
u16 out;
u8 amount(u16 k);

void shift_then_mask(s8 v, u16 flag) {
    u16 j = 3;
    while (j--)
        v >>= amount(j) & 15;
    out = flag ^ (u8)v;
}
