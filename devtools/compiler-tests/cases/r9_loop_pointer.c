/* A far pointer walked by a loop lives in tcc__r9 for the whole loop
 * (peephole_r9_loop, 2026-10-16): staged once before the header, its step
 * written to tcc__r9, no staging left in the body. walk_live and walk_ret
 * read the pointer after the loop: tcc__r9 is copied back to its slot on
 * the loop's way out only (walk_ret's n == 0 path skips the copy).
 * walk_call calls a function in the loop, which clobbers tcc__r9: the
 * pointer is staged every turn, as before. */
typedef unsigned short u16; typedef unsigned char u8;
static __far u16 dst[256];
void tick(void);
u16 walk(const u16 *q, u8 n) {
    u16 acc = 0; u8 k = 0;
    for (; n; n--, q += 3) { acc += q[0] ^ q[2]; dst[k++] = q[1]; }
    return acc;
}
u16 walk_live(const u16 *q, u8 n) {
    u16 acc = 0;
    for (; n; n--, q += 3) acc += q[0] ^ q[2];
    return acc + *q;
}
const u16 *walk_ret(u16 i, const u16 *q, u16 n, u16 stride) {
    for (; n; n--, i++, q += stride) dst[i & 255] = (dst[i & 255] & q[0]) | q[1];
    return q;
}
u16 walk_call(const u16 *q, u8 n) {
    u16 acc = 0;
    for (; n; n--, q += 3) { acc += q[0] ^ q[2]; tick(); }
    return acc;
}
