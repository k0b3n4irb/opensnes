/* A counted loop advancing a pointer down a column (2026-10-16): the step `q += stride` went through two temporaries and back
 * into q (peephole_forward_store: stored to q directly); the body's last
 * value was stored and read back before a label (the reload rule looks
 * past it); the test at the top was reached by a jmp each turn
 * (peephole_dec_loop: `dec n / bne @body` at the end). */
typedef unsigned short u16;
static __far u16 buf[1024];

const u16 *column(u16 i, const u16 *q, u16 n, u16 stride) {
    u16 m;
    for (; n; n--, i++, q += stride) {
        m = q[0];
        buf[i] = (buf[i] & m) | q[1];
        buf[i + 8] = (buf[i + 8] & m) | q[2];
    }
    return q;
}
