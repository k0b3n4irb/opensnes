/* Loop-carried u32 values stay in their slots (2026-10-25,
 * .claude/notes/chantiers/loop_carried_u32.md): a second game's 16.16 path
 * loop copied four u32 values at the end of every iteration (16 lda/sta,
 * the phi moves), and and'ed the low word with $FFFF. 83 instructions in
 * the loop body before, 62 after; 48 once the loaded table value feeds the
 * adc directly and the index's widening stops writing its high word. */
typedef unsigned short u16;
typedef unsigned long u32;
static const u32 tab[512] = { 1, 2, 3 };
u32 gx, gy, gang, grot, gacc;
void f(u16 n) {
    u32 x = gx, y = gy, ang = gang, rot = grot, acc = gacc;
    u16 i;
    while (n--) {
        i = (u16)(ang >> 16) << 1;
        y += tab[i];
        x += tab[i + 1];
        ang = (ang + rot) & 0x00FFFFFF;
        rot += acc;
    }
    gx = x; gy = y; gang = ang; grot = rot;
}
