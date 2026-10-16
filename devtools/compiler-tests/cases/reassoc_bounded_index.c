/* `buf[i + 8]` next to `buf[i]`: when i is proved bounded (a u8, an index
 * masked by a constant), (i + 8) * 2 is i * 2 + 16 exactly and the access
 * reads `buf+16,x` on the first access's X (reassoc.c, 2026-10-16). An
 * unbounded u16 i is left alone: i + 8 may wrap at 16 bits, and the
 * compiler does not reason from an out-of-bounds access (owner decision). */
typedef unsigned short u16; typedef unsigned char u8;
static __far u16 buf[1024];
void f8(u8 i, u16 m) { buf[i] = (buf[i] & m) | 1; buf[i + 8] = (buf[i + 8] & m) | 2; }
void fm(u16 j, u16 m) { u16 i = j & 511; buf[i] = (buf[i] & m) | 1; buf[i + 8] = (buf[i + 8] & m) | 2; }
void fu(u16 i, u16 m) { buf[i] = (buf[i] & m) | 1; buf[i + 8] = (buf[i + 8] & m) | 2; }
/* the index masked in the body of a loop, the array a union member: the
 * symbol arrives as `add $u, 0` (2026-10-16) */
static __far union { u8 b[2048]; u16 w[1024]; } u;
void fl(u16 i, u16 n, u16 m) {
    for (; n; n--, i++) { u16 k = i & 1023; u.w[k] = (u.w[k] & m) | 1; u.w[k + 8] = (u.w[k + 8] & m) | 2; }
}
