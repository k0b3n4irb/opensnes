/*
 * workloads.c — the same C for both SDKs.
 *
 * Nineteen small workloads of the kind a game runs every frame: loops over
 * arrays, structs of entities, collisions, multiplies and divides, 32-bit
 * arithmetic, byte copies, calls, a switch-driven interpreter, bit work,
 * a linked list, a tilemap, a 2D grid, entities through a pointer, word
 * and byte copies, strings, a state machine with a table of functions.
 * Each leaves a checksum in `res`; the two SDKs must agree on it, or the
 * comparison of their speed means nothing.
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
static void w_sieve(void) {
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
static void w_sort(void) {
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
static void w_physics(void) {
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
static void w_collide(void) {
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
static void w_mul(void) {
    u16 i, j, acc;
    acc = 0;
    for (i = 1; i <= 48; i++)
        for (j = 1; j <= 48; j++)
            acc += i * j + (i ^ j) * 3;
    res = acc;
}

/* 6. numbers to decimal digits: divide and modulo by 10 */
static void w_decimal(void) {
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
static void w_long(void) {
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
static void w_bytes(void) {
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

static void w_calls(void) {
    res = fib(17);
}

/* 10. a small bytecode interpreter: switch, branches, a state machine */
static void w_switch(void) {
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
static void w_crc(void) {
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
static void w_list(void) {
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
static void w_tilemap(void) {
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
static void w_grid(void) {
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
static void w_entities(void) {
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
static void w_copy(void) {
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

static void w_strings(void) {
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

static void w_state(void) {
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

static void w_place(void) {
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
#endif
    done = 0x600D;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
