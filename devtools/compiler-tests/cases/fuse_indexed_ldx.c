/* An indexed load stays the operand of what it feeds when the index
 * registers are loaded in between (peephole_fuse_indexed, 2026-10-15): an
 * ldy keeps X; an ldx is undone before the operation when X is dead after
 * it. And a 16-bit and/or/xor keeps the pointer staged in tcc__r9.
 * `d[k] |= p[k]` from ROM into far RAM: 9 or 10 instructions a word before,
 * 5 and 4 since. */
typedef unsigned short u16;
static __far u16 dst[64 * 16];
static const u16 src[256 * 16] = { 1, 2, 3 };

void orIndexed(u16 t, u16 i) {
    u16 __far *d = dst + (t << 4);
    const u16 *p = src + i;
    d[0] |= p[0]; d[1] |= p[1]; d[2] |= p[2]; d[3] |= p[3];
}

void orPointer(u16 t, const u16 *p) {
    u16 __far *d = dst + (t << 4);
    d[0] |= p[0]; d[1] |= p[1]; d[2] |= p[2]; d[3] |= p[3];
}
