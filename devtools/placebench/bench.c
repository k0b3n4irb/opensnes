/* placebench — the C side: data, setup and checksum of each row.
 *
 * oamPlaceWorld() (the library's) against place_old (place_old.asm, the
 * library's body before LIB place prod), on the same batches. run.py times
 * both on luna and refuses a row whose two checksums differ: the checksum
 * covers the whole OAM shadow (544 bytes), oam_max_id, oam_update_flag and
 * visible[].
 *
 * Neutral data, generated here. Two families of rows:
 *   - the M0 rows: 32 sprites (sizes 8, 16, 32), positions drawn so that
 *     about half are on screen, some across the left and top edges (the
 *     ninth x bit), some hidden by x, some by y; all placed; all hidden;
 *     one sprite; 29 sprites from id 3;
 *   - the rows of a consumer's pass (`p_` rows), the shapes one game calls
 *     every frame: two batches of 18 and 20 sprites 32x32 with order and
 *     visible[], about a quarter of them on screen; a one-sprite batch
 *     16x16 with visible[], placed or hidden; batches of 5 and 1 sprites
 *     32x32 and of 4 and 7 sprites 16x16 with visible[], half placed;
 *   - odd sizes (0, 1, 24) and a batch of two: the checksums only. */
#include <snes.h>

#define N 32

extern u8 oam_max_id;   /* sprite module state (sprite_oamset.asm) */

s16 wx[N], wy[N];
u8 wt[N], wa[N], wv[N], wo[N];
OamWorldBatch bt;
u16 cur_camx, cur_camy;
u32 cur_batch;

static u16 rng_state;
static u16 rng(void) {
    rng_state = rng_state * 25173 + 13849;
    return rng_state >> 4;
}

/* placement: how each sprite is drawn */
enum { P_MIX, P_VIS, P_HID, P_QUARTER, P_HALF };
enum { S_PLAIN, S_ORDER, S_VIS, S_ORDVIS };

static void setup(u16 seed, u16 size, u16 count, u16 first, u16 place, u16 shape, u16 badorder) {
    u16 i, mode;
    rng_state = 0x5EED + seed;
    cur_camx = 1000;
    cur_camy = 500;
    for (i = 0; i < N; i++) {
        s16 sx, sy;
        u16 r = rng();
        u16 sz = size ? size : 1;
        if (place == P_VIS) mode = 0;
        else if (place == P_HID) mode = 2 + (i & 1);
        else if (place == P_QUARTER) mode = (i % 9 == 1 || i % 9 == 5) ? (i & 2 ? 1 : 0) : 2 + ((i >> 1) & 1);
        else if (place == P_HALF) mode = (i & 1) ? 2 + ((i >> 1) & 1) : 0;
        else mode = r & 3;                    /* mix: half placed (0, 1), half hidden (2, 3) */
        if (mode == 0) {                      /* fully on screen */
            sx = (s16)(rng() % (256 - sz));
            sy = (s16)(rng() % (224 - sz));
        } else if (mode == 1) {               /* across the left or the top edge */
            sx = (i & 2) ? -(s16)(1 + rng() % sz) + 1 : (s16)(rng() % 200);
            sy = (i & 2) ? (s16)(rng() % 200) : -(s16)(1 + rng() % sz) + 1;
        } else if (mode == 2) {               /* hidden by x */
            sx = (i & 4) ? (s16)(256 + rng() % 300) : -(s16)(sz + rng() % 300);
            sy = (s16)(rng() % 200);
        } else {                              /* hidden by y (x on screen) */
            sx = (s16)(rng() % 240);
            sy = (i & 4) ? (s16)(224 + rng() % 300) : -(s16)(sz + rng() % 300);
        }
        wx[i] = (s16)cur_camx + sx;
        wy[i] = (s16)cur_camy + sy;
        wt[i] = (u8)rng();
        wa[i] = (u8)rng();
        wv[i] = 0x55;
        wo[i] = (u8)((i * 7 + 3) % count);    /* a permutation of 0..count-1 (count prime to 7) */
    }
    if (badorder)
        wo[7] = 200;                          /* one bad order byte: its slot hidden */
    if (count == 1)
        wo[0] = 0;
    for (i = 0; i < 544; i++)
        oamMemory[i] = (u8)(i * 7 + 3);       /* size bits and all: kept or not */
    oam_max_id = 5;
    oam_update_flag = 0;
    bt.x = wx;
    bt.y = wy;
    bt.tile = wt;
    bt.attr = wa;
    bt.visible = (shape == S_VIS || shape == S_ORDVIS) ? wv : 0;
    bt.first_id = (u8)first;
    bt.count = (u8)count;
    bt.size = (u8)size;
    bt.order = (shape == S_ORDER || shape == S_ORDVIS) ? wo : 0;
    cur_batch = (u32)&bt;
}

#define ROW(name, seed, size, count, first, place, shape, bad) \
    void s_##name(void) { setup(seed, size, count, first, place, shape, bad); }
#define ROWS4(k, seed, size, count, first, place, bad) \
    ROW(k##_plain, seed, size, count, first, place, S_PLAIN, bad) \
    ROW(k##_order, seed, size, count, first, place, S_ORDER, bad) \
    ROW(k##_vis, seed, size, count, first, place, S_VIS, bad) \
    ROW(k##_ordvis, seed, size, count, first, place, S_ORDVIS, bad)
/* the M0 rows (count 31 is prime to 7: the order byte stays a permutation) */
ROWS4(mix16, 0, 16, 32, 0, P_MIX, 1)
ROWS4(mix8, 1, 8, 32, 0, P_MIX, 1)
ROWS4(mix32, 7, 32, 32, 0, P_MIX, 1)
ROWS4(vis32, 2, 16, 32, 0, P_VIS, 0)
ROWS4(hid32, 3, 16, 32, 0, P_HID, 0)
ROWS4(vis1, 4, 16, 1, 0, P_VIS, 0)
ROWS4(hid1, 5, 16, 1, 0, P_HID, 0)
ROWS4(odd, 6, 16, 29, 3, P_MIX, 1)
/* the consumer's pass */
ROW(p18, 10, 32, 18, 0, P_QUARTER, S_ORDVIS, 0)
ROW(p20, 11, 32, 20, 18, P_QUARTER, S_ORDVIS, 0)
ROW(pball_in, 12, 16, 1, 38, P_VIS, S_VIS, 0)
ROW(pball_out, 13, 16, 1, 38, P_HID, S_VIS, 0)
ROW(p5, 14, 32, 5, 39, P_HALF, S_VIS, 0)
ROW(p1, 15, 32, 1, 44, P_HALF, S_VIS, 0)
ROW(p4a, 16, 16, 4, 45, P_HALF, S_VIS, 0)
ROW(p7, 17, 16, 7, 49, P_HALF, S_VIS, 0)
ROW(p4b, 18, 16, 4, 56, P_HALF, S_VIS, 0)
/* odd sizes and small counts: checksums */
ROWS4(size0, 20, 0, 23, 5, P_MIX, 1)
ROWS4(size1, 21, 1, 23, 5, P_MIX, 1)
ROWS4(size24, 22, 24, 23, 5, P_MIX, 1)
ROWS4(two, 23, 16, 2, 7, P_MIX, 0)
ROWS4(one0, 24, 0, 1, 9, P_MIX, 0)

void s_nop(void) { setup(0, 16, 32, 0, P_MIX, S_PLAIN, 0); }
u16 c_nop(void) { return 0; }

static u16 hash(const u8 *p, u16 n, u16 s) {
    u16 i;
    for (i = 0; i < n; i++)
        s = ((s << 3) | (s >> 13)) + p[i] + 1;
    return s;
}

/* The tile and attribute bytes of a hidden entry (x 1, y 240) are not part
 * of the contract: the old body's y store, a word, spills into the tile byte
 * of a sprite it then hides by x; the new one leaves them untouched. They
 * are left out. */
u16 c_sum(void) {
    u16 s = 0, i;
    for (i = 0; i < 512; i += 4) {
        s = hash(oamMemory + i, 2, s);
        if (oamMemory[i] != 1 || oamMemory[i + 1] != 240)
            s = hash(oamMemory + i + 2, 2, s);
    }
    s = hash(oamMemory + 512, 32, s);
    s = hash(wv, N, s);
    s = ((s << 3) | (s >> 13)) + oam_max_id;
    s = ((s << 3) | (s >> 13)) + oam_update_flag;
    return s;
}

u16 bench_sum[160];
u16 bench_i, bench_k, bench_n;
u16 bench_ptr[2];

void bench_run(void);

int main(void) {
    bench_run();
    return 0;
}
