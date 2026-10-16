/* A pointer staged in tcc__r9 stays staged across the index arithmetic of
 * another access (`i * 2` emitted as `asl a / tax`): the cache was dropped
 * by every op but loads, stores and and/or/xor. And `(d & m) | q[1]` folds
 * the read into the OR (`ora [tcc__r9],y`) instead of keeping d in a slot
 * (2026-10-16). */
typedef unsigned short u16;
static __far u16 buf[1024];

void column(u16 i, const u16 *q, u16 n) {
    u16 m;
    for (; n; n--, i++, q += 9) {
        m = q[0];
        buf[i] = (buf[i] & m) | q[1];
        buf[i + 8] = (buf[i + 8] & m) | q[2];
    }
}
