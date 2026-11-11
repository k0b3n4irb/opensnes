/*
 * memcpy, memmove and memset against a byte loop (stress fixture).
 *
 * Every length 0..40 and 255, 256, 257, 1000, 4096 where the memory holds
 * them, odd and even addresses on both sides, across the kinds of memory a
 * pointer can name: plain RAM (bank $00, below $2000), FAR RAM ($7E:2000+),
 * RAM in bank $7F (a pointer built from its address: the SDK places nothing
 * there), and a const table in ROM (an asset bank). Each destination has
 * eight guard bytes before and after, checked after the call. The return
 * value is checked whole (address and bank), and so is the data bank
 * register on return: the compiler assumes it is 0, and the block move
 * leaves it at the destination's bank unless the routine restores it.
 * memmove moves inside one buffer by 1, 2 and n - 1 bytes both ways, and
 * the result is compared with a snapshot taken before.
 *
 * memset runs from the start of bank $7F and up to its end, where its first
 * turn's index goes back across the bank boundary (string.asm).
 *
 * Before any of that, with the NMI off, the four routines are called on a
 * table of lengths between two calls of sb_mark: the master cycles of each
 * call are the difference of two `luna profile --until-pc sb_mark --hit k`
 * (the commit of the routines has the figures and the command).
 *
 * Read by string_block.toml: r_bad = 0, case counts, r_done.
 */
#include <snes.h>
#include <snes/string.h>

#define GUARD 8

extern const u8 sb_rom[];
extern u16 sb_dbr(void);
extern void sb_quiet(void);
extern void sb_loud(void);
extern void sb_mark(void);

/* Results, read by the manifest. */
u16 r_bad;          /* wrong bytes, guards, return values, DBR       -> 0 */
u16 r_cpy;          /* memcpy cases run */
u16 r_mov;          /* memmove cases run */
u16 r_set;          /* memset cases run */
u16 r_first_case;   /* the first failing case (diagnosis) */
u16 r_first_n;      /* its length */
u16 r_first_i;      /* the byte, or 0xD000 DBR, 0xE000 return, 0xF0xx/0xF1xx guard */
u16 r_done;         /* 0xC0DE at the end */

static u16 case_id;
static u16 dbr0;
static u16 guard_after;
volatile u16 canary = 0x5AC3;

static u8 lo_a[1040 + 2 * GUARD];
static u8 lo_b[1040 + 2 * GUARD];
static FAR u8 fa[4112 + 2 * GUARD];
static FAR u8 fb[4112 + 2 * GUARD];
static FAR u8 snap[4112 + 2 * GUARD];

#define P7F_A ((u8 FAR *)0x7F4000UL)
#define P7F_B ((u8 FAR *)0x7F6000UL)
#define P7F_LO ((u8 FAR *)0x7F0008UL)

static const u16 lens_long[5] = { 255, 256, 257, 1000, 4096 };

static void fail(u16 n, u16 i)
{
    if (!r_bad) {
        r_first_case = case_id;
        r_first_n = n;
        r_first_i = i;
    }
    r_bad++;
}

static u8 gbyte(u16 i) { return (u8)(0xC3 ^ (i * 29)); }

static void prep(u8 FAR *dst, u16 n)
{
    u8 FAR *p = dst - GUARD;
    u16 i, m = n + GUARD + (guard_after ? GUARD : 0);
    for (i = 0; i < m; i++) p[i] = gbyte(i);
}

static void check_after(u8 FAR *dst, u16 n, void FAR *r)
{
    u8 FAR *p = dst - GUARD;
    u16 i;
    if (sb_dbr() != dbr0) fail(n, 0xD000);
    if (canary != 0x5AC3) fail(n, 0xD001);
    if ((u32)r != (u32)dst) fail(n, 0xE000);
    for (i = 0; i < GUARD; i++) {
        if (p[i] != gbyte(i)) fail(n, 0xF000 | i);
        if (guard_after && p[GUARD + n + i] != gbyte(GUARD + n + i)) fail(n, 0xF100 | i);
    }
}

static void one_copy(u8 FAR *dst, const u8 *src, u16 n)
{
    void FAR *r;
    u16 i;
    prep(dst, n);
    r = memcpy(dst, src, n);
    check_after(dst, n, r);
    for (i = 0; i < n; i++) {
        if (dst[i] != src[i]) {
            fail(n, i);
            break;
        }
    }
    case_id++;
    r_cpy++;
}

static void one_set(u8 FAR *dst, u16 c, u16 n)
{
    void FAR *r;
    u16 i;
    prep(dst, n);
    r = memset(dst, c, n);
    check_after(dst, n, r);
    for (i = 0; i < n; i++) {
        if (dst[i] != (u8)c) {
            fail(n, i);
            break;
        }
    }
    case_id++;
    r_set++;
}

/* buf[-GUARD .. span + GUARD) gets a pattern; memmove(buf + d, buf + s, n);
 * every byte of that range is compared with the snapshot moved by hand. */
static void one_move(u8 FAR *buf, u16 s, u16 d, u16 n)
{
    void FAR *r;
    u8 FAR *p = buf - GUARD;
    u16 span = (s > d ? s : d) + n + 2 * GUARD;
    u16 i, j;
    for (i = 0; i < span; i++) {
        p[i] = (u8)(i * 7 + 0x11);
        snap[i] = p[i];
    }
    r = memmove(buf + d, buf + s, n);
    if (sb_dbr() != dbr0) fail(n, 0xD000);
    if ((u32)r != (u32)(buf + d)) fail(n, 0xE000);
    for (i = 0; i < span; i++) {
        u8 want = snap[i];
        j = i - GUARD;              /* index from buf (wraps below: not in [d, d+n)) */
        if (j >= d && j < d + n) want = snap[s + (j - d) + GUARD];
        if (p[i] != want) {
            fail(n, i);
            break;
        }
    }
    case_id++;
    r_mov++;
}

static void fill(u8 FAR *p, u16 n, u8 seed)
{
    u16 i;
    for (i = 0; i < n; i++) p[i] = (u8)(i * 5 + seed);
}

/* The lengths of a run, up to max: 0..40, then the long ones that fit. */
static u16 len_at(u16 k, u16 max)
{
    if (k <= 40) return k;
    k -= 41;
    if (k < 5 && lens_long[k] <= max) return lens_long[k];
    return 0xFFFF;
}

static void copies(u8 FAR *dst, const u8 *src, u16 max)
{
    u16 k, n, a;
    for (a = 0; a < 4; a++) {
        for (k = 0; k < 46; k++) {
            n = len_at(k, max);
            if (n == 0xFFFF) continue;
            one_copy(dst + (a & 1), src + (a >> 1), n);
        }
    }
}

static void sets(u8 FAR *dst, u16 max)
{
    u16 k, n, a;
    for (a = 0; a < 2; a++) {
        for (k = 0; k < 46; k++) {
            n = len_at(k, max);
            if (n == 0xFFFF) continue;
            one_set(dst + a, (k & 1) ? 0x1200 + k * 37 : (u8)(0xFF - k), n);
        }
    }
}

/* 1, 2 and n - 1 apart, both ways, at both parities of the lower end */
static void moves(u8 FAR *buf, u16 max)
{
    u16 k, n, a, o;
    for (a = 0; a < 2; a++) {
        for (k = 0; k < 46; k++) {
            n = len_at(k, max);
            if (n == 0xFFFF || n == 0) continue;
            for (o = 0; o < 3; o++) {
                u16 sh = o == 0 ? 1 : o == 1 ? 2 : n - 1;
                if (sh == 0) continue;
                one_move(buf, a, a + sh, n);     /* dst above: backward */
                one_move(buf, a + sh, a, n);     /* dst below: forward */
            }
        }
    }
    one_move(buf, 5, 5, 40);                     /* same place */
    one_move(buf, 0, 40, 40);                    /* touching, no overlap */
}

/* ---- timing (NMI off): one call between two marks, for each length ---- */
#define NT 13
static const u16 tlen[NT] = { 0, 4, 8, 12, 14, 16, 18, 20, 24, 32, 64, 256, 4096 };

static void timing(void)
{
    u16 i;
    sb_quiet();
    for (i = 0; i < NT; i++) { sb_mark(); }                          /* hits 1..13: the loop alone */
    for (i = 0; i < NT; i++) { sb_mark(); memcpy(fb, fa, tlen[i]); }  /* 14..26 */
    for (i = 0; i < NT; i++) { sb_mark(); memmove(fa, fa + 1, tlen[i]); }   /* 27..39: forward */
    for (i = 0; i < NT; i++) { sb_mark(); memmove(fa + 1, fa, tlen[i]); }   /* 40..52: backward */
    for (i = 0; i < NT; i++) { sb_mark(); memset(fb, 0x55, tlen[i]); }      /* 53..65 */
    sb_mark();                                                       /* 66 */
    sb_loud();
}

int main(void)
{
    consoleInit();
    timing();
    dbr0 = sb_dbr();

    fill(lo_a, sizeof(lo_a), 0x21);
    fill(fa, sizeof(fa), 0x43);
    fill(P7F_A - GUARD, 4112 + 2 * GUARD, 0x65);

    guard_after = 1;
    copies(lo_b + GUARD, lo_a + GUARD, 1000);              /* plain -> plain */
    copies(fb + GUARD, lo_a + GUARD, 1000);                /* plain -> FAR */
    copies(lo_b + GUARD, sb_rom, 1000);                    /* ROM -> plain */
    copies(fb + GUARD, sb_rom, 4096);                      /* ROM -> FAR */
    copies(fb + GUARD, fa + GUARD, 4096);                  /* FAR -> FAR */
    copies(P7F_B, fa + GUARD, 4096);                       /* FAR -> $7F */
    copies(P7F_B, P7F_A, 4096);                            /* $7F -> $7F */
    copies(lo_b + GUARD, P7F_A, 1000);                     /* $7F -> plain */
    copies(P7F_B, sb_rom, 4096);                           /* ROM -> $7F */

    sets(lo_b + GUARD, 1000);
    sets(fb + GUARD, 4096);
    sets(P7F_B, 4096);
    sets(P7F_LO, 40);                                      /* from $7F:0008: the first turn's index crosses back into $7E */
    guard_after = 0;
    {
        u16 n;
        for (n = 1; n <= 40; n++) {                        /* up to $7F:FFFF, the last byte of the bank */
            one_set((u8 FAR *)(0x800000UL - n), 0xA0 + n, n);
            one_copy((u8 FAR *)(0x800000UL - n), sb_rom, n);
        }
    }
    guard_after = 1;

    moves(lo_a + GUARD, 257);
    moves(fa + GUARD, 1000);
    moves(P7F_A, 1000);

    r_done = 0xC0DE;
    while (1) { }
    return 0;
}
