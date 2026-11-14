/*
 * A far pointer read in its direct-page slot and stepped on 16 bits.
 *
 * The compiler keeps a pointer derived from one object this unit defines as
 * (address modulo 2^16, the object's bank): a C object never crosses a bank,
 * so its step adds the low word only. Its slot in the direct page is read as
 * `[tcc__lf+N]` instead of being copied to tcc__r9 first. What may go wrong:
 *
 *   - an object that ENDS at $7E:FFFF: one past its end is $7F:0000, valid
 *     to compare, and a 16-bit step gives $7E:0000. `tail` is placed there
 *     (pad fills the rest of the band; test_farptr_dp.py checks the
 *     placement in the .sym) and cmp_tail() compares a walked pointer with
 *     its end: the pointer must keep its 32-bit step
 *     (QBE_FARPTR_FAULT=2 drops that rule: n_cmp reaches the bound);
 *   - an object defined elsewhere, which may cross a bank: `strad`, defined
 *     in strad.asm at $7E:FFF8, 16 bytes up to $7F:0007. ext_walk() must
 *     step it on 32 bits (QBE_FARPTR_FAULT=1 takes any symbol for a root:
 *     the walk reads $7E:0000-0007 instead of $7F:0000-0007).
 *
 * The other functions are the rule's own cases: two pointers walked in turn
 * (copy2, the LZ unpack shape), a pointer walked by a counter to the last
 * byte of `tail`, a const table and a plain (bank $00) array.
 */
#include <snes.h>

FAR u8 pad[0xDFF0 - 128];
FAR u8 src[64];
FAR u8 dst[64];
FAR u8 tail[16];                /* $7E:FFF0-$FFFF */
extern FAR u8 strad[16];        /* strad.asm: $7E:FFF8-$7F:0007 */
static const u8 rom[16] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9, 3 };
u8 nearbuf[16];

u16 r_copy, r_tail, r_cmp, n_cmp, r_ext, r_rom, r_near, r_back, done;

void copy2(u16 n) {
    const u8 FAR *s = src;
    u8 FAR *d = dst;
    do {
        *d++ = *s++;
        *d++ = *s++ ^ 0x55;
    } while (--n);
}

u16 walk_tail(void) {
    const u8 FAR *p = tail;
    u16 i, s = 0;
    for (i = 0; i < 16; i++)
        s += *p++;
    return s;
}

/* The end is computed at run time: a constant `tail + 16` is `$7F:0000` the
 * assembler cannot write as `#tail+16` (the link stops, "out of 16-bit
 * range"), a loud limit of its own. */
volatile u16 vlen = 16;

u16 cmp_tail(void) {
    const u8 FAR *p;
    const u8 FAR *end = tail + vlen;
    u16 n = 0, s = 0;
    for (p = tail; p < end && n < 40; p++, n++)
        s += *p;
    n_cmp = n;
    return s;
}

u16 ext_walk(void) {
    const u8 FAR *p = strad;
    u16 i, s = 0;
    for (i = 0; i < 16; i++)
        s += *p++;
    return s;
}

u16 rom_walk(void) {
    const u8 *p = rom;
    u16 i, s = 0;
    for (i = 0; i < 16; i++)
        s += *p++ * (i + 1);
    return s;
}

u16 near_walk(void) {
    u8 *p = nearbuf;
    u16 i, s = 0;
    for (i = 0; i < 16; i++)
        *p++ = (u8)(i * 7);
    p = nearbuf;
    for (i = 0; i < 16; i++)
        s += *p++;
    return s;
}

/* a walked pointer stepped back: the bank word stays the root's */
u16 back_walk(void) {
    const u8 FAR *p = tail + 15;
    u16 i, s = 0;
    for (i = 0; i < 16; i++)
        s = s * 3 + *p--;
    return s;
}

int main(void) {
    u8 FAR *hi = (u8 FAR *)0x7F0000UL;
    u16 i;

    pad[0] = 1;                 /* kept by the link (it places tail) */
    for (i = 0; i < 64; i++)
        src[i] = (u8)(i * 5 + 1);
    for (i = 0; i < 16; i++)
        tail[i] = (u8)(i + 1);
    for (i = 0; i < 8; i++)
        hi[i] = (u8)(0x40 + i);
    copy2(32);
    for (i = 0, r_copy = 0; i < 64; i++)
        r_copy = r_copy * 3 + dst[i];
    r_tail = walk_tail();
    r_cmp = cmp_tail();
    r_ext = ext_walk();
    r_rom = rom_walk();
    r_near = near_walk();
    r_back = back_walk();
    done = 0xBEEF;
    while (1) {
    }
    return 0;
}
