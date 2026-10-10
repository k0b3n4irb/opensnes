/*
 * workloads.c — the same C for both SDKs.
 *
 * Twenty small workloads of the kind a game runs every frame: loops over
 * arrays, structs of entities, collisions, multiplies and divides, 32-bit
 * arithmetic, byte copies, calls, a switch-driven interpreter, bit work,
 * a linked list, a tilemap, a 2D grid, entities through a pointer, word
 * and byte copies, strings, a state machine with a table of functions.
 * Each leaves a checksum in `res`; the two SDKs must agree on it, or the
 * comparison of their speed means nothing.
 *
 * The workloads themselves (w_*) are not static: each must remain a
 * function of its own to be measured, and since 2026-10-10 cc65816 gives a
 * static function with one call site to its caller. Their helpers stay
 * static, as a game would write them.
 *
 * Plain C89 so that 816-tcc and cc65816 both take it unchanged. WORKLOAD
 * (from which.h) selects the one this ROM runs; 0 runs none (the baseline
 * the runner subtracts).
 */
#include <snes.h>
#include "which.h"

u16 res;
u16 done;

#define NENT 32

struct Ent { s16 x; s16 y; s16 vx; s16 vy; };
struct Node { u16 val; struct Node *next; };

static u8 sieve[1024];
static u16 arr[64];
static struct Ent ent[NENT];
static u8 bufa[512];
static u8 bufb[512];
static struct Node nodes[64];
static u16 tmap[32 * 16];
static u8 grid[16][32];
static u16 wsrc[128];
static u16 wdst[128];
static u16 rnd;

static u16 rnd16(void) {
    rnd = rnd * 25173 + 13849;
    return rnd;
}

static void ent_init(void) {
    u16 i;
    rnd = 12345;
    for (i = 0; i < NENT; i++) {
        ent[i].x = (s16)(16 + (rnd16() & 127));
        ent[i].y = (s16)(24 + (rnd16() & 127));
        ent[i].vx = (s16)((rnd16() & 3) + 1);
        ent[i].vy = (s16)((rnd16() & 3) + 1);
        if (i & 1) ent[i].vx = -ent[i].vx;
        if (i & 2) ent[i].vy = -ent[i].vy;
    }
}

/* 1. sieve of Eratosthenes: byte array writes, nested loops */
void w_sieve(void) {
    u16 i, j, count;
    count = 0;
    for (i = 0; i < 1024; i++) sieve[i] = 1;
    for (i = 2; i < 1024; i++) {
        if (sieve[i]) {
            count++;
            for (j = i + i; j < 1024; j += i) sieve[j] = 0;
        }
    }
    res = count;
}

/* 2. insertion sort of 64 words: compares and moves in an array */
void w_sort(void) {
    u16 i, j, key, sum;
    rnd = 777;
    for (i = 0; i < 64; i++) arr[i] = rnd16();
    for (i = 1; i < 64; i++) {
        key = arr[i];
        j = i;
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
    sum = 0;
    for (i = 0; i < 64; i++) sum = (sum << 1) ^ arr[i] ^ (sum >> 15);
    res = sum;
}

/* 3. entities bouncing in a box, 60 frames of it: struct fields, signed math */
void w_physics(void) {
    u16 step, i, sum;
    ent_init();
    for (step = 0; step < 60; step++) {
        for (i = 0; i < NENT; i++) {
            ent[i].x += ent[i].vx;
            ent[i].y += ent[i].vy;
            if (ent[i].x < 8 || ent[i].x > 248) ent[i].vx = -ent[i].vx;
            if (ent[i].y < 16 || ent[i].y > 208) ent[i].vy = -ent[i].vy;
        }
    }
    sum = 0;
    for (i = 0; i < NENT; i++) sum += (u16)ent[i].x * 3 + (u16)ent[i].y;
    res = sum;
}

/* 4. box against box for every pair, 8 rounds: struct reads through pointers */
void w_collide(void) {
    u16 round, i, j, hits;
    s16 dx, dy;
    struct Ent *a;
    struct Ent *b;
    ent_init();
    hits = 0;
    for (round = 0; round < 8; round++) {
        for (i = 0; i < NENT; i++) {
            a = &ent[i];
            for (j = i + 1; j < NENT; j++) {
                b = &ent[j];
                dx = a->x - b->x;
                dy = a->y - b->y;
                if (dx < 0) dx = -dx;
                if (dy < 0) dy = -dy;
                if (dx < 16 && dy < 16) hits++;
            }
            a->x += a->vx;
            a->y += a->vy;
        }
    }
    res = hits;
}

/* 5. multiply two variables, 48 x 48 times */
void w_mul(void) {
    u16 i, j, acc;
    acc = 0;
    for (i = 1; i <= 48; i++)
        for (j = 1; j <= 48; j++)
            acc += i * j + (i ^ j) * 3;
    res = acc;
}

/* 6. numbers to decimal digits: divide and modulo by 10 */
void w_decimal(void) {
    u16 n, v, sum;
    sum = 0;
    for (n = 0; n < 200; n++) {
        v = n * 327 + 1;
        while (v) {
            sum += v % 10;
            v = v / 10;
        }
    }
    res = sum;
}

/* 7. a 32-bit generator and hash: long multiply, add, shifts */
void w_long(void) {
    u32 x, h;
    u16 i;
    x = 1;
    h = 0;
    for (i = 0; i < 300; i++) {
        x = x * 1664525 + 1013904223;
        h ^= x >> 11;
        h += x << 3;
    }
    res = (u16)(h ^ (h >> 16));
}

/* 8. byte buffers: fill, copy, compare, scan */
void w_bytes(void) {
    u16 i, pass, sum;
    u8 *s;
    u8 *d;
    sum = 0;
    for (i = 0; i < 512; i++) bufa[i] = (u8)(i * 7 + 3);
    for (pass = 0; pass < 4; pass++) {
        s = bufa;
        d = bufb;
        for (i = 0; i < 512; i++) *d++ = *s++;
        bufa[pass * 100] ^= 0x55;
        for (i = 0; i < 512; i++) if (bufa[i] != bufb[i]) sum += i;
    }
    bufb[400] = 0;
    for (i = 0; bufb[i]; i++) sum++;
    res = sum;
}

/* 9. calls: a recursive Fibonacci */
static u16 fib(u16 n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

void w_calls(void) {
    res = fib(17);
}

/* 10. a small bytecode interpreter: switch, branches, a state machine */
void w_switch(void) {
    u16 pc, acc, x, round;
    u8 op;
    for (pc = 0; pc < 64; pc++) bufa[pc] = (u8)((pc * 5 + (pc >> 2)) & 7);
    acc = 1;
    x = 3;
    for (round = 0; round < 20; round++) {
        for (pc = 0; pc < 64; pc++) {
            op = bufa[pc];
            switch (op) {
            case 0: acc += x; break;
            case 1: acc ^= 0x5A5A; break;
            case 2: x++; break;
            case 3: acc = (acc << 1) | (acc >> 15); break;
            case 4: if (acc & 1) x += 2; break;
            case 5: acc -= pc; break;
            case 6: x = acc & 15; break;
            default: acc += 7; break;
            }
        }
    }
    res = acc ^ x;
}

/* 11. CRC-16 bit by bit over 256 bytes: shifts and xors */
void w_crc(void) {
    u16 i, crc;
    u8 b, k;
    crc = 0xFFFF;
    for (i = 0; i < 256; i++) {
        b = (u8)(i * 13 + 5);
        crc ^= (u16)b << 8;
        for (k = 0; k < 8; k++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    res = crc;
}

/* 12. a linked list walked 40 times: pointer chasing */
void w_list(void) {
    u16 i, round, sum;
    struct Node *p;
    for (i = 0; i < 64; i++) {
        nodes[i].val = i * 3 + 1;
        nodes[i].next = &nodes[(i * 37 + 11) & 63];
    }
    nodes[63].next = 0;
    sum = 0;
    for (round = 0; round < 40; round++) {
        p = &nodes[round & 63];
        i = 0;
        while (p && i < 64) {
            sum += p->val;
            p = p->next;
            i++;
        }
    }
    res = sum;
}

/* 13. a 32 x 16 tilemap: written from coordinates, then looked up */
void w_tilemap(void) {
    u16 x, y, n, sum;
    for (y = 0; y < 16; y++)
        for (x = 0; x < 32; x++)
            tmap[(y << 5) | x] = (x ^ y) + ((y & 3) << 10);
    sum = 0;
    x = 3;
    y = 5;
    for (n = 0; n < 600; n++) {
        sum += tmap[(y << 5) | x];
        x = (x + 7) & 31;
        y = (y + 3) & 15;
        if (tmap[(y << 5) | x] & 0x0400) sum ^= n;
    }
    res = sum;
}

/* 14. a 16 x 32 byte grid: the four neighbours of every inner cell */
void w_grid(void) {
    u8 x, y;
    u16 sum;
    rnd = 4242;
    for (y = 0; y < 16; y++)
        for (x = 0; x < 32; x++)
            grid[y][x] = (u8)(rnd16() >> 9);
    sum = 0;
    for (y = 1; y < 15; y++)
        for (x = 1; x < 31; x++)
            sum += grid[y - 1][x] + grid[y + 1][x] + grid[y][x - 1] + grid[y][x + 1];
    res = sum;
}

/* 15. the entities again, walked with a pointer instead of an index */
void w_entities(void) {
    u16 step, sum;
    struct Ent *e;
    ent_init();
    for (step = 0; step < 60; step++) {
        for (e = ent; e != ent + NENT; e++) {
            e->x += e->vx;
            e->y += e->vy;
            if (e->x < 8 || e->x > 248) e->vx = -e->vx;
            if (e->y < 16 || e->y > 208) e->vy = -e->vy;
        }
    }
    sum = 0;
    for (e = ent; e != ent + NENT; e++) sum += (u16)e->x * 3 + (u16)e->y;
    res = sum;
}

/* 16. word and byte copies written as index loops */
void w_copy(void) {
    u16 i, pass, sum;
    for (i = 0; i < 128; i++) wsrc[i] = i * 517 + 9;
    for (i = 0; i < 512; i++) bufa[i] = (u8)(i + (i >> 3));
    sum = 0;
    for (pass = 0; pass < 8; pass++) {
        for (i = 0; i < 128; i++) wdst[i] = wsrc[i];
        wsrc[pass * 9] += pass;
        sum += wdst[pass * 15];
    }
    for (pass = 0; pass < 3; pass++) {
        for (i = 0; i < 512; i++) bufb[i] = bufa[i];
        bufa[pass * 150] ^= 0x0F;
        sum += bufb[pass * 170];
    }
    res = sum;
}

/* 17. strings: length, compare, copy, by hand */
static u16 slen(u8 *s) {
    u16 n;
    n = 0;
    while (s[n]) n++;
    return n;
}

static u16 scmp(u8 *a, u8 *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (u16)*a - (u16)*b;
}

static void scpy(u8 *d, u8 *s) {
    while ((*d++ = *s++) != 0) {
    }
}

void w_strings(void) {
    u16 i, k, sum;
    for (k = 0; k < 8; k++) {
        for (i = 0; i < 40 + k * 2; i++) bufa[k * 64 + i] = (u8)('A' + ((i * (k + 3)) & 15));
        bufa[k * 64 + i] = 0;
    }
    sum = 0;
    for (k = 0; k < 8; k++) sum += slen(bufa + k * 64);
    for (k = 0; k < 7; k++) sum ^= scmp(bufa + k * 64, bufa + (k + 1) * 64);
    for (k = 0; k < 8; k++) scpy(bufb + k * 64, bufa + (7 - k) * 64);
    for (k = 0; k < 8; k++) sum += scmp(bufb + k * 64, bufa + (7 - k) * 64) + slen(bufb + k * 64);
    res = sum;
}

/* 18. a state machine: a switch on the state, and a table of functions */
static u16 op_add(u16 v) { return v + 3; }
static u16 op_xor(u16 v) { return v ^ 0x1234; }
static u16 op_rot(u16 v) { return (v << 3) | (v >> 13); }
static u16 op_dec(u16 v) { return v - 1; }

static u16 (*ops[4])(u16) = { op_add, op_xor, op_rot, op_dec };

void w_state(void) {
    u16 n, acc;
    u8 state;
    acc = 1;
    state = 0;
    for (n = 0; n < 600; n++) {
        switch (state) {
        case 0: state = (acc & 1) ? 2 : 1; break;
        case 1: acc += n; state = 3; break;
        case 2: acc ^= n << 2; state = (acc & 4) ? 0 : 3; break;
        default: state = (u8)(n & 3) == 0 ? 0 : 1; break;
        }
        acc = ops[n & 3](acc);
    }
    res = acc;
}

/* A per-entity loop over parallel tables (issue #166, from a real game):
 * world position to screen position for 19 sprites, with an on-screen test.
 * place() is the issue's function, unchanged. */
#define PN 19
static u16 size_tab[PN];
static s16 pxs[PN];
static s16 pys[PN];
static u16 pslot[PN];
static u16 pout[2 * PN];

static void place(u16 cam_x, u16 cam_y) {
    u16 id;

    for (id = 0; id < PN; id++) {
        u16 size = size_tab[id];
        u16 x = pxs[id] - cam_x;
        u16 y = pys[id] - cam_y;

        if ((u16)(x + size) < 256 + size && (u16)(y + size) < 224 + size)
            pout[pslot[id]] = (x & 0xFF) | (y << 8);
    }
}

void w_place(void) {
    u16 i, n, acc;
    for (i = 0; i < PN; i++) {
        size_tab[i] = (i == 18) ? 16 : 32;
        pxs[i] = (s16)(i * 23) - 40;
        pys[i] = (s16)(i * 17) - 20;
        pslot[i] = i << 1;
    }
    for (i = 0; i < 2 * PN; i++) pout[i] = 0;
    acc = 0;
    for (n = 0; n < 200; n++) {
        place(n, n >> 1);
        acc += pout[(n % PN) << 1];
    }
    for (i = 0; i < 2 * PN; i++) acc ^= pout[i] + i;
    res = acc;
}

/* Every player of one team against every player of the other (issue #166,
 * from the same game): a nested loop, the index rebuilt in each block, two
 * absolute values and a static function with one call site. distances() and
 * vectorLength() are the issue's functions, unchanged but for the names of
 * the tables. */
static s16 dpx[18];
static s16 dpy[18];
static s16 dpoff[18];
static u16 dist_flat[81];

/* length of (a, b) without a square root */
static u16 vectorLength(u16 a, u16 b) {
    u16 r;

    if (a < b) {
        r = a;
        a = b;
        b = r;
    }
    r = a + (b >> 1);
    if (r < (b << 1)) return r - (b >> 3);
    if (r < (b << 2)) return r - (b >> 2);
    return r - ((b >> 2) + (b >> 3));
}

static void distances(void) {
    u16 i, j;

    for (i = 0; i < 9; i++) {
        for (j = 0; j < 9; j++) {
            s16 dx = dpx[i] - dpx[j + 9];
            s16 dy = dpy[i] - dpy[j + 9] - dpoff[j + 9];

            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            dist_flat[i * 9 + j] = vectorLength(dx, dy);
        }
    }
}

void w_dist(void) {
    u16 i, n, acc;
    for (i = 0; i < 18; i++) {
        dpx[i] = (s16)(i * 37) - 200;
        dpy[i] = (s16)(i * 53) - 300;
        dpoff[i] = i & 7;
    }
    acc = 0;
    for (n = 0; n < 40; n++) {
        dpx[n % 18] += 7;
        dpy[(n * 5) % 18] -= 3;
        distances();
        acc += dist_flat[n % 81];
    }
    for (i = 0; i < 81; i++) acc ^= dist_flat[i] + i;
    res = acc;
}

/* ---- depot: a burst of decisions by eighteen agents -----------------------
 * Two crews of nine carts in a depot of 512 x 960; every few ticks the
 * sixteen that are not leaders decide where to go, all on the same tick.
 * That burst is the workload: 30 of them, 480 decisions.
 *
 *   decide -> crowd       a rival within arm's length? sidestep or chase
 *          -> intruder    sometimes: shadow the rival nearest my bay's middle
 *          -> slotFor     else a slot relative to my leader, from a table of
 *                         6 rows x 9 columns
 *          -> depart -> bearing, steer
 *
 * Written for this bench by the first game built on the SDK, to have the
 * shape its compiled C spends its time on without any of its rules: one
 * global array per field indexed by the agent, small helpers called from
 * loops, values live across a call, signed compares, a table indexed by
 * row * 9 + column, 32-bit sums kept in two 16-bit halves with the carry
 * rebuilt by a compare. Expected: res = 0xD31F (the same file built
 * natively). */
#define NA          18
#define CREW        9
#define NONE        0xFFFF
#define ROWS        6
#define COLS        9
#define STILL       8       /* no heading: already there */

#define ARM         24      /* a rival this close is in the way */
#define SNAP        4
#define DEPOT_W     512
#define DEPOT_H     960

#define TASK_IDLE   0
#define TASK_ROLL   1
#define TASK_CHASE  2
#define TASK_DODGE  3

static s16 px[NA];
static s16 py[NA];
static s16 vx[NA];
static s16 vy[NA];
static s16 wx[NA];              /* waypoint */
static s16 wy[NA];
static u16 heading[NA];         /* 0 = north, then clockwise */
static u16 wait[NA];            /* ticks before the next decision */
static u16 task[NA];
static u16 busy[NA];
static u16 dice[NA];            /* byte drawn at each decision */

static s16 bay_x0[NA];
static s16 bay_x1[NA];
static s16 bay_y0[NA];
static s16 bay_y1[NA];
static u16 rank[NA];            /* row of the slot table */
static u16 nerve[NA];           /* 0 to 255: how readily it leaves its slot */
static u16 range[NA];           /* how far it looks for an intruder */
static u16 lead[NA];            /* look-ahead shift on the other's velocity */
static u16 pace[NA];            /* 0 to 7 */

static s16 slot_x[ROWS * COLS];
static s16 slot_y[ROWS * COLS];
static s16 step_x[64];          /* pace * 8 + heading */
static s16 step_y[64];

/* (sign of dy + 1) * 3 + sign of dx + 1 -> heading */
static const u16 compass[9] = { 7, 0, 1, 6, STILL, 2, 5, 4, 3 };

static u16 leader[2];
static u16 load;                /* the cart that carries the load, or NONE */
static s16 aim_x, aim_y;        /* point being worked out */
static u16 mix_hi, mix_lo;      /* 32-bit state of the dice */
static u16 odo_hi, odo_lo;      /* 32-bit sum of the distances decided */

/* Distance between two offsets, without a square root: the longer side plus
 * half the shorter. */
static u16 gap(s16 dx, s16 dy) {
    u16 a = (dx < 0) ? -dx : dx;
    u16 b = (dy < 0) ? -dy : dy;

    if (a < b) return b + (a >> 1);
    return a + (b >> 1);
}

/* One byte of dice: a 32-bit add in two halves, the carry rebuilt by a
 * compare, then the halves stirred into each other. */
static u16 shake(void) {
    u16 lo = mix_lo + 0x9E37;
    u16 carry = lo < mix_lo;
    u16 hi = mix_hi + 0x79B9 + carry;

    mix_lo = lo ^ (hi >> 3);
    mix_hi = hi ^ (u16)(lo << 5);
    return (mix_hi ^ mix_lo) & 0xFF;
}

static void tally(u16 d) {
    u16 sum = odo_lo + d;

    if (sum < odo_lo) odo_hi++;
    odo_lo = sum;
}

static void clampX(u16 id) {
    if (aim_x < bay_x0[id]) aim_x = bay_x0[id];
    else if (aim_x > bay_x1[id]) aim_x = bay_x1[id];
}

static void clampY(u16 id) {
    if (aim_y < bay_y0[id]) aim_y = bay_y0[id];
    else if (aim_y > bay_y1[id]) aim_y = bay_y1[id];
}

/* Heading from a cart to a point: an axis counts only if its offset is more
 * than a quarter of the other one. */
static u16 bearing(u16 id, s16 x, s16 y) {
    s16 dx = x - px[id];
    s16 dy = y - py[id];
    s16 ax = (dx < 0) ? -dx : dx;
    s16 ay = (dy < 0) ? -dy : dy;
    u16 at = 4;

    if (ax < SNAP && ay < SNAP) return STILL;
    if (ay > (ax >> 2)) at = (dy < 0) ? 1 : 7;
    if (ax > (ay >> 2)) at += (dx < 0) ? -1 : 1;
    return compass[at];
}

static void halt(u16 id) {
    vx[id] = 0;
    vy[id] = 0;
    task[id] = TASK_IDLE;
}

/* Take a heading: velocity from the table, and an axis already within reach
 * of the waypoint is put on it. */
static u16 steer(u16 id, u16 h) {
    u16 at;
    s16 d;

    if (h == STILL) {
        halt(id);
        return 0;
    }
    heading[id] = h;
    at = (pace[id] << 3) + h;
    vx[id] = step_x[at];
    vy[id] = step_y[at];
    d = wx[id] - px[id];
    if (d < 0) d = -d;
    if (d < SNAP) {
        px[id] = wx[id];
        vx[id] = 0;
    }
    d = wy[id] - py[id];
    if (d < 0) d = -d;
    if (d < SNAP) {
        py[id] = wy[id];
        vy[id] = 0;
    }
    return 1;
}

/* Go to the point worked out, kept inside the bay. */
static void depart(u16 id, u16 what) {
    clampX(id);
    clampY(id);
    wx[id] = aim_x;
    wy[id] = aim_y;
    if (steer(id, bearing(id, aim_x, aim_y))) task[id] = what;
    tally(gap(aim_x - px[id], aim_y - py[id]));
}

/* Where the other will be, as far ahead as this cart can foresee. */
static void forecast(u16 id, u16 other) {
    aim_x = px[other] + (vx[other] << lead[id]);
    aim_y = py[other] + (vy[other] << lead[id]);
}

/* Step aside: at right angles to the line between the two carts, on the
 * side the other is not heading to. */
static void sidestep(u16 id, u16 other) {
    s16 dx = px[other] - px[id];
    s16 dy = py[other] - py[id];

    if (vx[other] + vy[other] < 0) {
        aim_x = px[id] + dy;
        aim_y = py[id] - dx;
    } else {
        aim_x = px[id] - dy;
        aim_y = py[id] + dx;
    }
}

/* A rival within arm's length: chase it if it carries the load and this cart
 * has the nerve, otherwise step aside. */
static u16 crowd(u16 id) {
    u16 other = (id < CREW) ? CREW : 0;
    u16 end = other + CREW;
    s16 dx, dy;

    for (; other < end; other++) {
        dx = px[other] - px[id];
        dy = py[other] - py[id];
        if (dx > ARM || dx < -ARM || dy > ARM || dy < -ARM) continue;
        if (gap(dx, dy) > ARM) continue;
        if (other == load && nerve[id] > dice[id]) {
            forecast(id, other);
            depart(id, TASK_CHASE);
        } else {
            sidestep(id, other);
            depart(id, TASK_DODGE);
        }
        return 1;
    }
    return 0;
}

/* The rival inside this cart's bay that is nearest to the middle of it, if
 * it is within twice the cart's range. */
static u16 intruder(u16 id) {
    u16 other = (id < CREW) ? CREW : 0;
    u16 end = other + CREW;
    u16 best = NONE;
    u16 nearest = range[id] << 1;
    s16 mx = (bay_x0[id] + bay_x1[id]) >> 1;
    s16 my = (bay_y0[id] + bay_y1[id]) >> 1;
    u16 d;

    for (; other < end; other++) {
        if (busy[other]) continue;
        if (px[other] < bay_x0[id] || px[other] > bay_x1[id]) continue;
        if (py[other] < bay_y0[id] || py[other] > bay_y1[id]) continue;
        d = gap(px[other] - mx, py[other] - my);
        if (d >= nearest) continue;
        nearest = d;
        best = other;
    }
    return best;
}

/* A slot relative to the leader: the row is the cart's rank, the column is
 * the ninth of the depot the leader stands in. The second crew uses the
 * table turned half a turn. */
static void slotFor(u16 id, u16 ref) {
    s16 x = px[ref];
    s16 y = py[ref];
    u16 col = 0;
    u16 at;

    if (x >= 342) col = 2;
    else if (x >= 171) col = 1;
    if (y >= 640) col += 6;
    else if (y >= 320) col += 3;
    if (id >= CREW) col = COLS - 1 - col;
    at = rank[id] * COLS + col;
    if (id < CREW) {
        aim_x = x + slot_x[at];
        aim_y = y + slot_y[at];
    } else {
        aim_x = x - slot_x[at];
        aim_y = y - slot_y[at];
    }
    aim_x += vx[ref] << lead[id];
    aim_y += vy[ref] << lead[id];
}

static void decide(u16 id) {
    u16 crew = id >= CREW;
    u16 ref, other;

    dice[id] = shake();
    wait[id] = 4 + (pace[id] >> 1);
    if (busy[id]) return;
    if (crowd(id)) return;
    if (dice[id] < nerve[id]) {
        other = intruder(id);
        if (other != NONE) {
            forecast(id, other);
            depart(id, TASK_CHASE);
            return;
        }
    }
    ref = leader[crew];
    slotFor(id, ref);
    if (rank[id] >= 3 && load != NONE && (load >= CREW) == crew) {
        /* the front ranks run ahead when their crew carries the load */
        if (crew) aim_y += 48;
        else aim_y -= 48;
    }
    depart(id, TASK_ROLL);
}

static void setup(void) {
    u16 id, n, h;
    s16 v, w;

    for (id = 0; id < NA; id++) {
        u16 crew = id >= CREW;
        u16 k = crew ? id - CREW : id;

        rank[id] = k % ROWS;
        bay_x0[id] = 16 + (k % 3) * 120;
        bay_x1[id] = bay_x0[id] + 250;
        bay_y0[id] = 16 + (k / 3) * 190 + (crew ? 150 : 0);
        bay_y1[id] = bay_y0[id] + 400;
        px[id] = bay_x0[id] + 40 + ((id * 37) & 127);
        py[id] = bay_y0[id] + 60 + ((id * 53) & 255);
        vx[id] = (s16)(id % 5) - 2;
        vy[id] = (s16)(id % 7) - 3;
        wx[id] = px[id];
        wy[id] = py[id];
        heading[id] = crew ? 4 : 0;
        wait[id] = 0;
        task[id] = TASK_IDLE;
        busy[id] = 0;
        dice[id] = 0;
        nerve[id] = (id * 29 + 40) & 0xFF;
        range[id] = 60 + (id & 3) * 30;
        lead[id] = 1 + (id & 1);
        pace[id] = 3 + (id % 4);
    }
    for (n = 0; n < ROWS * COLS; n++) {
        slot_x[n] = (s16)((n * 23) % 181) - 90;
        slot_y[n] = (s16)((n * 41) % 221) - 110;
    }
    for (n = 0; n < 8; n++) {
        for (h = 0; h < 8; h++) {
            v = (h == 1 || h == 2 || h == 3) ? (s16)n : (h == 5 || h == 6 || h == 7) ? -(s16)n : 0;
            w = (h == 3 || h == 4 || h == 5) ? (s16)n : (h == 7 || h == 0 || h == 1) ? -(s16)n : 0;
            step_x[(n << 3) + h] = v;
            step_y[(n << 3) + h] = w;
        }
    }
    mix_hi = 0x2545;
    mix_lo = 0xF491;
    odo_hi = 0;
    odo_lo = 0;
}

void w_depot(void) {
    u16 n, id, acc;

    setup();
    acc = 0;
    for (n = 0; n < 30; n++) {
        /* the tick before: everybody rolls, kept inside the depot */
        for (id = 0; id < NA; id++) {
            px[id] += vx[id];
            py[id] += vy[id];
            if (px[id] < 8) px[id] = 8;
            else if (px[id] > DEPOT_W - 8) px[id] = DEPOT_W - 8;
            if (py[id] < 8) py[id] = 8;
            else if (py[id] > DEPOT_H - 8) py[id] = DEPOT_H - 8;
            busy[id] = ((id + n * 3) & 15) == 5;
        }
        leader[0] = (n & 1) ? 6 : 3;
        leader[1] = (n & 4) ? 15 : 11;
        load = (n & 2) ? (n * 7 + 2) % NA : NONE;
        /* the burst: the sixteen others decide, the crews interleaved */
        for (id = 0; id < NA; id++) {
            u16 who = (id & 1) ? (id >> 1) : (id >> 1) + CREW;

            if (who != leader[0] && who != leader[1]) decide(who);
        }
        acc += (u16)wx[n % NA] ^ (u16)wy[(n * 5) % NA];
    }
    for (id = 0; id < NA; id++)
        acc ^= (u16)(wx[id] + wy[id]) + (heading[id] << 8) + task[id] + (u16)(vx[id] * 3) + (u16)vy[id] + dice[id] + wait[id] + id;
    res = acc ^ odo_lo ^ (u16)(odo_hi << 9) ^ mix_hi;
}

int main(void) {
    consoleInit();
#if WORKLOAD == 1
    w_sieve();
#elif WORKLOAD == 2
    w_sort();
#elif WORKLOAD == 3
    w_physics();
#elif WORKLOAD == 4
    w_collide();
#elif WORKLOAD == 5
    w_mul();
#elif WORKLOAD == 6
    w_decimal();
#elif WORKLOAD == 7
    w_long();
#elif WORKLOAD == 8
    w_bytes();
#elif WORKLOAD == 9
    w_calls();
#elif WORKLOAD == 10
    w_switch();
#elif WORKLOAD == 11
    w_crc();
#elif WORKLOAD == 12
    w_list();
#elif WORKLOAD == 13
    w_tilemap();
#elif WORKLOAD == 14
    w_grid();
#elif WORKLOAD == 15
    w_entities();
#elif WORKLOAD == 16
    w_copy();
#elif WORKLOAD == 17
    w_strings();
#elif WORKLOAD == 18
    w_state();
#elif WORKLOAD == 19
    w_place();
#elif WORKLOAD == 20
    w_dist();
#elif WORKLOAD == 21
    w_depot();
#endif
    done = 0x600D;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
