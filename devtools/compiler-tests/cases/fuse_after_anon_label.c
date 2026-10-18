/* A function holding a variable shift (`1 << n`, emitted as the loop
 * `-  asl a / dex / bne -`) kept no fusion at all: the liveness of the
 * peephole stage gave up on the whole function at the anonymous `-` label,
 * so the slot the fusion must prove dead was taken as live (2026-10-15, the
 * OR inlined into a large function). */
typedef unsigned short u16;
static __far u16 dst[64 * 16];
u16 mask, bits[16];

void orWithShift(u16 t, u16 n, const u16 *p) {
    u16 __far *d = dst + (t << 4);
    mask = 1 << n;
    d[0] |= p[0]; d[1] |= p[1]; d[2] |= p[2]; d[3] |= p[3];
}
