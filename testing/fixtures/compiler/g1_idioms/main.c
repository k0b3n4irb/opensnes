/* G1 idiom I1 (compiler/qbe/idiom.c): a 32-bit value that is the sign
 * extension of a 16-bit one keeps that value's sign mask as its high
 * word, and the extension of `s >> k` is then the 32-bit `sar` of the
 * extension of s. Built with OPENSNES_G1_IDIOMS=1 the compiler rewrites
 * every function below; without it, it does not. Each result is a 32-bit
 * fold over values of s that cover every high byte, both signs and the
 * edges (-32768, -1, 0, 1, 32767), checked by test_g1_idioms.py against
 * the same C computed on the host.
 *
 * Results: r_path0, r_path3 (the path follower's two accumulators),
 * r_k[k] for k = 1..15 ((s32)s ^ (s32)(s >> k) << 1), r_chain (s, s >> 1,
 * (s >> 1) >> 3 widened), r_dom (the extension in a block that
 * dominates a loop which widens s >> 2), done = 0xBEEF at the end.
 *
 * G1 idiom I2 (same file): a loop variable used in the loop only as the
 * table index 2*v is carried scaled, entry scaled with the IR's 32-bit
 * rule, step and wrap mask scaled; its value after the loop is j >> 1.
 * The table is read through a FAR pointer b at b + 2*idx (24 bits), so an
 * index of $8000 or more reaches bank $7F: put() fills the indices read,
 * F(i) at i, and r_i2[c] for each case c of main() folds every value read
 * and the index after the loop (test_g1_idioms.py: model and cases).
 *
 * G1 idiom I3 (same file): a loop whose trip count is a constant of at
 * least 1 is entered at its body, the test at the bottom. r_i3[c] folds
 * what each loop of case c saw (counter used or not, signed, stepped by
 * 7, wrapping past $FFFF, counted down, one trip, nested, a break: not
 * rotated); i3_zero's loop never runs and must not be rotated
 * (OPENSNES_G1_FAULT=3 rotates it: its body runs once).
 *
 * G1 idiom I4 (same file): a counter used only for the trip count is
 * counted down to zero from that count. r_i4[c] folds what each loop of
 * case c did (signed from -5, a step of 3, counted down, wrapping past
 * $FFFF, `<=`, a second exit, a loop that never runs);
 * OPENSNES_G1_FAULT=4 reads the signed compare as unsigned: i4_neg's
 * count becomes 0.
 */
#include <snes.h>

u32 r_path0, r_path3, r_chain, r_dom;
u32 r_k[16];
u32 r_i2[13];
u32 r_i3[10];
u32 r_i4[7];
u16 done;
FAR s16 big[64];
s16 vals[264];

static u32 mix(u32 acc, u32 f) {
    return ((acc << 1) | (acc >> 31)) ^ f;
}

/* the shape of a path follower: one value widened as is and shifted */
__attribute__((noinline)) void path(u16 n) {
    u32 p0 = 0x00010000, p3 = 0x00020000;
    u16 i;
    for (i = 0; i < n; i++) {
        s16 s = vals[i];
        p0 += (u32)(s32)s;
        p3 += (u32)(s32)(s >> 1);
        p3 = mix(p3, p0);
    }
    r_path0 = p0;
    r_path3 = p3;
}

#define KF(K) \
    __attribute__((noinline)) u32 kf##K(s16 s) { \
        return (u32)(s32)s ^ ((u32)(s32)(s >> K) << 1); \
    }
KF(1) KF(2) KF(3) KF(4) KF(5) KF(6) KF(7) KF(8)
KF(9) KF(10) KF(11) KF(12) KF(13) KF(14) KF(15)

__attribute__((noinline)) u32 chain(s16 s) {
    s16 a = s >> 1;
    s16 b = a >> 3;
    s32 x = s, y = a, z = b;
    return (u32)(x + y * 16 - z * 512);
}

/* the plain extension is made before the loop, the shifted one inside */
__attribute__((noinline)) u32 dom(s16 s, u16 n) {
    s32 e = s;
    u32 acc = (u32)e;
    u16 i;
    for (i = 0; i < n; i++) {
        acc = mix(acc, (u32)(s32)(s >> 2));
        acc += (u32)e;
    }
    return acc;
}

/* I2: b[idx] with idx stepped and masked, the path shape */
__attribute__((noinline)) void put(s16 FAR *b, u16 i, s16 v) { b[i] = v; }

static s16 F(u16 i) { return (s16)(u16)(i * 0x9E37u + 0x51u); }

__attribute__((noinline)) u32 i2_walk(s16 FAR *b, u16 idx, u16 step, u16 n) {
    u32 acc = 0x00030000;
    for (; n; n--) {
        acc = mix(acc, (u32)(s32)b[idx]);
        idx = (idx + step) & 63;
    }
    return mix(acc, idx);
}

/* a mask that keeps bit 15: every index is $8000 or more */
__attribute__((noinline)) u32 i2_wide(s16 FAR *b, u16 idx, u16 step, u16 n) {
    u32 acc = 0x00040000;
    for (; n; n--) {
        acc = mix(acc, (u32)(s32)b[idx]);
        idx = (idx + step) & 0x803F;
    }
    return mix(acc, idx);
}

/* no mask: the 16-bit wrap itself */
__attribute__((noinline)) u32 i2_free(s16 FAR *b, u16 idx, u16 step, u16 n) {
    u32 acc = 0x00050000;
    for (; n; n--) {
        acc = mix(acc, (u32)(s32)b[idx]);
        idx += step;
    }
    return mix(acc, idx);
}

__attribute__((noinline)) u32 i2_sub(s16 FAR *b, u16 idx, u16 step, u16 n) {
    u32 acc = 0x00060000;
    for (; n; n--) {
        acc = mix(acc, (u32)(s32)b[idx]);
        idx = (idx - step) & 63;
    }
    return mix(acc, idx);
}

/* two exits: the key found, or n run out */
__attribute__((noinline)) u32 i2_exit(s16 FAR *b, u16 idx, s16 key, u16 n) {
    u32 acc = 0x00070000;
    while (n) {
        s16 s = b[idx];
        if (s == key)
            break;
        acc = mix(acc, (u32)(s32)s);
        idx = (idx + 3) & 63;
        n--;
    }
    return mix(acc, idx) ^ n;
}

/* constant entry and step */
__attribute__((noinline)) u32 i2_const(s16 FAR *b, u16 n) {
    u32 acc = 0x00080000;
    u16 idx = 7;
    for (; n; n--) {
        acc = mix(acc, (u32)(s32)b[idx]);
        idx = (idx + 9) & 63;
    }
    return mix(acc, idx);
}

/* I3: constant trip counts */
__attribute__((noinline)) u32 i3_count(void) {
    u32 acc = 0x00090000;
    u16 n;
    for (n = 0; n < 64; n++)
        acc = acc * 3 + 0x1234;   /* mix by a constant has period 32 */
    return acc;
}

__attribute__((noinline)) u32 i3_idx(void) {
    u32 acc = 0x000A0000;
    u16 i;
    for (i = 0; i < 10; i++)
        acc = mix(acc, (u32)(s32)vals[i]);
    return mix(acc, i);
}

__attribute__((noinline)) u32 i3_neg(void) {
    u32 acc = 0x000B0000;
    s16 i;
    for (i = -5; i < 5; i++)
        acc = mix(acc, (u32)(s32)i);
    return mix(acc, (u32)(s32)i);
}

__attribute__((noinline)) u32 i3_step(void) {
    u32 acc = 0x000C0000;
    u16 i;
    for (i = 3; i < 100; i += 7)
        acc = mix(acc, i);
    return mix(acc, i);
}

__attribute__((noinline)) u32 i3_wrap(void) {
    u32 acc = 0x000D0000;
    u16 i;
    for (i = 0xFFF0; i != 0x10; i++)
        acc = mix(acc, i);
    return mix(acc, i);
}

__attribute__((noinline)) u32 i3_down(void) {
    u32 acc = 0x000E0000;
    u16 i;
    for (i = 20; i > 0; i--)
        acc = mix(acc, i);
    return mix(acc, i);
}

__attribute__((noinline)) u32 i3_one(void) {
    u32 acc = 0x000F0000;
    u16 i;
    for (i = 7; i < 8; i++)
        acc = mix(acc, i);
    return mix(acc, i);
}

__attribute__((noinline)) u32 i3_nest(void) {
    u32 acc = 0x00100000;
    u16 i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 3; j++)
            acc = mix(acc, i * 16 + j);
    return acc;
}

/* a second exit: not rotated */
__attribute__((noinline)) u32 i3_brk(void) {
    u32 acc = 0x00110000;
    u16 i;
    for (i = 0; i < 50; i++) {
        if (vals[i] == vals[12])
            break;
        acc = mix(acc, i);
    }
    return mix(acc, i);
}

/* never runs: not rotated */
__attribute__((noinline)) u32 i3_zero(void) {
    u32 acc = 0x00120000;
    u16 i;
    for (i = 5; i < 3; i++)
        acc = mix(acc, i);
    return mix(acc, i);
}

/* I4: counters used only for the trip count */
__attribute__((noinline)) u32 i4_neg(void) {
    u32 acc = 0x00200000;
    s16 n;
    for (n = -5; n < 5; n++)
        acc = acc * 3 + 0x55;
    return acc;
}

__attribute__((noinline)) u32 i4_step(void) {
    u32 acc = 0x00210000;
    u16 n;
    for (n = 1; n < 100; n += 3)
        acc = acc * 3 + 0x66;
    return acc;
}

__attribute__((noinline)) u32 i4_down(void) {
    u32 acc = 0x00220000;
    u16 n;
    for (n = 10; n > 0; n--)
        acc = acc * 3 + 0x77;
    return acc;
}

__attribute__((noinline)) u32 i4_wrap(void) {
    u32 acc = 0x00230000;
    u16 n;
    for (n = 0xFFF0; n != 0x10; n++)
        acc = acc * 3 + 0x88;
    return acc;
}

__attribute__((noinline)) u32 i4_le(void) {
    u32 acc = 0x00240000;
    u16 n;
    for (n = 0; n <= 7; n++)
        acc = acc * 3 + 0x99;
    return acc;
}

/* a second exit, on the data: reversed, not rotated */
__attribute__((noinline)) u32 i4_brk(void) {
    u32 acc = 0x00250000;
    u16 n;
    for (n = 0; n < 40; n++) {
        if ((acc & 0xFF) == 0x3A)
            break;
        acc = acc * 3 + 0x1D;
    }
    return acc;
}

/* never runs: reversed to 0, not rotated */
__attribute__((noinline)) u32 i4_zero(void) {
    u32 acc = 0x00260000;
    u16 n;
    for (n = 5; n < 3; n++)
        acc = acc * 3 + 0xAA;
    return acc;
}

typedef u32 (*kfn)(s16);
static const kfn kfs[16] = {0, kf1, kf2, kf3, kf4, kf5, kf6, kf7, kf8,
                            kf9, kf10, kf11, kf12, kf13, kf14, kf15};

int main(void) {
    u16 i, k;
    u32 acc;

    for (i = 0; i < 256; i++)
        vals[i] = (s16)(i * 257 + 0x35);
    vals[256] = (s16)0x8000;
    vals[257] = -1;
    vals[258] = 0;
    vals[259] = 1;
    vals[260] = 0x7FFF;
    vals[261] = -2;
    vals[262] = (s16)0x8001;
    vals[263] = 0x4000;

    consoleInit();
    path(264);
    for (k = 1; k < 16; k++) {
        acc = 0;
        for (i = 0; i < 264; i++)
            acc = mix(acc, kfs[k](vals[i]));
        r_k[k] = acc;
    }
    acc = 0;
    for (i = 0; i < 264; i++)
        acc = mix(acc, chain(vals[i]));
    r_chain = acc;
    acc = 0;
    for (i = 0; i < 264; i += 7)
        acc = mix(acc, dom(vals[i], 3));
    r_dom = acc;

    for (i = 0; i < 64; i++) {
        put(big, i, F(i));
        put(big, 0x8000 + i, F(0x8000 + i));
    }
    for (i = 0; i < 16; i++)
        put(big, 0x7FF0 + i, F(0x7FF0 + i));
    r_i2[0] = i2_walk(big, 3, 5, 64);          /* path */
    r_i2[1] = i2_walk(big, 2, 0xFFFF, 70);     /* step -1, masked, across 0 */
    r_i2[2] = i2_walk(big, 0x8003, 5, 20);     /* entry >= $8000 */
    r_i2[3] = i2_walk(big, 0x8005, 1, 0);      /* no trip: the entry back */
    r_i2[4] = i2_wide(big, 0x8010, 7, 40);     /* mask $803F */
    r_i2[5] = i2_free(big, 5, 0x8000, 6);      /* wraps past $FFFF */
    r_i2[6] = i2_free(big, 0x8002, 0xFFFF, 18);/* down across $8000 */
    r_i2[7] = i2_free(big, 0x7FF4, 1, 30);     /* up across $8000 */
    r_i2[8] = i2_sub(big, 1, 3, 50);           /* sub, masked, across 0 */
    r_i2[9] = i2_exit(big, 4, F(19), 64);      /* key found */
    r_i2[10] = i2_exit(big, 4, 0x1234, 30);    /* n run out */
    r_i2[11] = i2_const(big, 40);
    r_i2[12] = i2_exit(big, 0x8001, F(0x8001), 10); /* found at entry */
    r_i3[0] = i3_count();
    r_i3[1] = i3_idx();
    r_i3[2] = i3_neg();
    r_i3[3] = i3_step();
    r_i3[4] = i3_wrap();
    r_i3[5] = i3_down();
    r_i3[6] = i3_one();
    r_i3[7] = i3_nest();
    r_i3[8] = i3_brk();
    r_i3[9] = i3_zero();
    r_i4[0] = i4_neg();
    r_i4[1] = i4_step();
    r_i4[2] = i4_down();
    r_i4[3] = i4_wrap();
    r_i4[4] = i4_le();
    r_i4[5] = i4_brk();
    r_i4[6] = i4_zero();
    done = 0xBEEF;
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
