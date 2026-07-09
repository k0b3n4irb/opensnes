/* Codegen pin for opensnes#99 (FIXED): a u8 read-modify-write through a
 * pointer, then re-read.
 *
 * The byte STORE (`sta.l $0000,x`) deliberately leaves A in 8-bit mode
 * (Ostoreb doesn't restore, so consecutive byte stores stay cheap). The
 * following byte LOAD's indirect path must therefore emit `rep #$20`
 * before its 16-bit `lda <addr>,s` address reload — otherwise the reload
 * grabs only the pointer's LOW byte and `tax` keeps a stale high byte, so
 * the re-read hits the wrong page. The bug only surfaced for structs
 * outside page zero (a stack local), which is why lib code masked it.
 *
 * Behavioural pin: r_rmw_u8 in devtools/libtests. This is the static
 * codegen shape. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef struct { const u16 *tab; u8 idx; u8 cnt; u8 fl; u8 rsv; } RmwProbe;

u16 rmw_step(RmwProbe *q) {
    u16 out = q->tab[q->idx];
    if (q->fl & 1) return out;
    q->cnt--;
    if (q->cnt == 0) {        /* store cnt-1, then RE-READ cnt through q */
        q->idx = q->idx + 1;
        q->cnt = 2;
    }
    return out;
}
