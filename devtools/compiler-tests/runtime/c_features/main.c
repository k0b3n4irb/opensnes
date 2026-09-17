/*
 * C-feature runtime fixture (gaps review C2, 2026-09-13).
 *
 * The static C->ASM checks prove patterns; this ROM proves RESULTS. Every C
 * feature that had no runtime assertion before — switch (dense, sparse,
 * fall-through, negative labels, 32-bit selector), function pointers (const
 * table, RAM table, callback parameter, struct member, equality), bit-fields,
 * enum, goto (forward, backward, out of nested loops), recursion (single,
 * double, mutual), 32-bit multiply / divide / modulo by 32-bit operands,
 * variable-count 32-bit shifts, runtime 32-bit compares, promotions and
 * truncations, signed division rounding, mixed-sign comparison, short-circuit
 * side effects, sizeof of the target's types, const far reads — writes its
 * result into a WRAM global that test_c_features.py asserts through
 * `luna state --assert`.
 *
 * Inputs come from `volatile` globals so QBE cannot fold the computation at
 * compile time: the codegen, not the constant folder, is under test.
 *
 * Deliberately absent: variadic functions, struct parameters / returns /
 * assignment by value, inline assembly. The toolchain REFUSES them with a
 * message naming the feature; devtools/compiler-tests/cases/negative/ pins
 * that refusal. See KNOWN_LIMITATIONS.md "Type & ABI gotchas".
 *
 * Globals sit in bank $00 WRAM (< $2000). u16 is little-endian: value V ->
 * bytes V&FF, V>>8; u32 adds V>>16, V>>24.
 */
#include <snes.h>

/* ---- inputs (volatile: runtime values, not constants) -------------------- */
volatile u16 in_one = 1;
volatile u16 in_seven = 7;
volatile u16 in_100 = 100;
volatile u16 in_1000 = 1000;
volatile u16 in_30000 = 30000;
volatile u16 in_30000b = 30000;
volatile s16 in_neg7 = -7;
volatile s16 in_neg3 = -3;
volatile s16 in_neg1 = -1;
volatile s16 in_neg30000 = -30000;
volatile u16 in_sh1 = 1;
volatile u16 in_sh17 = 17;
volatile u16 in_sh31 = 31;
volatile u32 in_big = 0x00012345ul;
volatile u32 in_max = 0xFFFFFFFFul;
volatile u32 in_max2 = 0xFFFFFFFFul;   /* equal to in_max, a different object (the lint rejects self-comparison) */
volatile u32 in_65536 = 0x00010000ul;
volatile u32 in_131072 = 0x00020000ul;
volatile s32 in_negbig = -100000l;
volatile u8 in_200 = 200;
volatile u8 in_100b = 100;
volatile s8 in_neg5 = -5;

/* ---- switch --------------------------------------------------------------- */
u16 r_sw_dense;     /* dense 0..7 on in_seven      -> 70 */
u16 r_sw_sparse;    /* 1/100/1000/30000 on in_1000 -> 3 */
u16 r_sw_default;   /* no label matches in_100     -> 99 */
u16 r_sw_fall;      /* fall-through from 1 to 2    -> 12 */
u16 r_sw_neg;       /* negative label on in_neg3   -> 33 */
u16 r_sw_32;        /* 32-bit selector in_65536    -> 5 */
u16 r_sw_loop;      /* switch in a loop with continue/break -> 6 */

static u16 dense(u16 v) {
    switch (v) {
    case 0: return 0;
    case 1: return 10;
    case 2: return 20;
    case 3: return 30;
    case 4: return 40;
    case 5: return 50;
    case 6: return 60;
    case 7: return 70;
    default: return 99;
    }
}

static u16 sparse(u16 v) {
    switch (v) {
    case 1:     return 1;
    case 100:   return 2;
    case 1000:  return 3;
    case 30000: return 4;
    default:    return 99;
    }
}

static u16 fall(u16 v) {
    u16 r = 0;
    switch (v) {
    case 1: r += 10;   /* fall through */
    case 2: r += 2; break;
    case 3: r += 3; break;
    }
    return r;
}

static u16 negsw(s16 v) {
    switch (v) {
    case -3: return 33;
    case -1: return 11;
    case 3:  return 3;
    default: return 0;
    }
}

static u16 sw32(u32 v) {
    switch (v) {
    case 0x00000001ul: return 1;
    case 0x00010000ul: return 5;
    case 0x00020000ul: return 6;
    default:           return 0;
    }
}

/* ---- function pointers ---------------------------------------------------- */
typedef u16 (*fn_t)(u16);
static u16 dbl(u16 v) { return v * 2; }
static u16 inc(u16 v) { return v + 1; }
static u16 sq(u16 v)  { return v * v; }
static const fn_t const_tab[3] = { dbl, inc, sq };
static fn_t ram_tab[3];
struct ops { fn_t f; u16 k; };
static const struct ops op_rec = { sq, 4 };

u16 r_fp_const;     /* const_tab[in_one](10) -> 11 */
u16 r_fp_ram;       /* ram_tab[2](7) = sq(7) -> 49 */
u16 r_fp_cb;        /* apply(dbl, 21)        -> 42 */
u16 r_fp_struct;    /* op_rec.f(op_rec.k)    -> 16 */
u16 r_fp_eq;        /* (p == dbl) + 2*(p != inc) -> 3 */

static u16 apply(fn_t f, u16 v) { return f(v); }

/* ---- bit-fields, enum ----------------------------------------------------- */
struct bits { u16 a : 3; u16 b : 5; u16 c : 8; };
enum colour { RED, GREEN = 5, BLUE, MAGENTA = 200 };

u16 r_bf_sum;       /* a=5, b=17, c=200 -> 222 */
u16 r_bf_ovf;       /* a = 9 stored in 3 bits -> 1 */
u16 r_bf_c;         /* c after b changed stays 200 -> 200 */
u16 r_enum;         /* BLUE + MAGENTA -> 206 */
u16 r_enum_sw;      /* switch on enum GREEN -> 2 */

static u16 enumsw(enum colour c) {
    switch (c) {
    case RED:     return 1;
    case GREEN:   return 2;
    case BLUE:    return 3;
    case MAGENTA: return 4;
    }
    return 0;
}

/* ---- goto ----------------------------------------------------------------- */
u16 r_goto_fwd;     /* skip an assignment -> 1 */
u16 r_goto_back;    /* loop by backward goto -> 4 */
u16 r_goto_nested;  /* break out of two loops at i=2,j=3 -> 23 */

/* ---- recursion ------------------------------------------------------------ */
u16 r_fact;         /* fact(6)  -> 720 */
u16 r_fib;          /* fib(10)  -> 55 */
u16 r_mutual;       /* is_even(7)*10 + is_odd(7) -> 1 */

static u16 fact(u16 n) { return n < 2 ? 1 : n * fact(n - 1); }
static u16 fib(u16 n)  { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
static u16 is_odd(u16 n);
static u16 is_even(u16 n) { return n == 0 ? 1 : is_odd(n - 1); }
static u16 is_odd(u16 n)  { return n == 0 ? 0 : is_even(n - 1); }

/* ---- 32-bit arithmetic with runtime operands ------------------------------ */
u32 r_mul32;        /* 0x12345 * 0x100          -> 0x01234500 */
u32 r_mul32_wrap;   /* 0xFFFFFFFF * 2           -> 0xFFFFFFFE */
u32 r_div32;        /* 0xFFFFFFFF / 0x10000     -> 0x0000FFFF */
u32 r_mod32;        /* 0x12345 % 0x10000        -> 0x00002345 */
u32 r_smul32;       /* -100000 * 3              -> 0xFFFB6C20 (-300000) */
u32 r_sdiv32;       /* -100000 / 7              -> 0xFFFFC833 (-14285, truncated) */
u32 r_smod32;       /* -100000 % 7              -> 0xFFFFFFFB (-5) */
u32 r_shl_var;      /* 0x12345 << 17            -> 0x468A0000 */
u32 r_shr_var;      /* 0xFFFFFFFF >> 31         -> 1 */
u32 r_sar_var;      /* -100000 >> 17 (arith)    -> 0xFFFFFFFF (-1) */
u32 r_shl1;         /* 0xFFFFFFFF << 1          -> 0xFFFFFFFE */
u16 r_cmp32;        /* (in_big < in_max) + 2*(in_65536 > in_big) + 4*(in_big == 0x12345) -> 5 */
u16 r_scmp32;       /* (in_negbig < 0) + 2*(in_negbig < (s32)in_big) + 4*((s32)in_max < 0) -> 7 */
u16 r_jnz32;        /* if (in_65536) with only the high half set -> 1 */
u16 r_eq32;         /* values that differ only in the high half: (in_65536 == in_131072) + 2*(in_65536 != in_131072) + 4*(in_max == in_max2) -> 6 */
u16 r_lnot32;       /* !in_65536 + 2*(in_65536 && in_one) + 4*(in_65536 || 0) -> 6 */

/* ---- widths, promotions, truncations -------------------------------------- */
u32 r_widen_u;      /* (u32)in_30000 * 4        -> 120000 */
u32 r_widen_s;      /* (s32)in_neg7             -> 0xFFFFFFF9 */
u32 r_mul16to32;    /* (s32)in_neg3 * in_30000  -> -90000 = 0xFFFEA070 */
u16 r_trunc16;      /* (u16)in_big              -> 0x2345 */
u8  r_trunc8;       /* (u8)in_big               -> 0x45 */
u16 r_promote8;     /* in_200 + in_100b (u8+u8 in int) -> 300 */
u16 r_s8ext;        /* (s16)in_neg5 * 3         -> -15 = 0xFFF1 */
u16 r_u16wrap;      /* 0xFFFF + in_one          -> 0 */
u16 r_sdiv16;       /* -7 / 2                   -> -3 = 0xFFFD */
u16 r_smod16;       /* -7 % 2                   -> -1 = 0xFFFF */
u16 r_mixcmp;       /* (in_neg1 < (u16)in_one)  -> 0 (usual arithmetic conversions) */
u16 r_scmp16;       /* signed 16-bit compares whose subtraction overflows: (-30000 < 30000) + 2*(30000 > -30000) + 4*(-30000 <= -1) + 8*(-1 >= -30000) -> 15 (every u16 operand cast to s16: an s16/u16 pair compares UNSIGNED in C) */
u16 r_sbr16;        /* the same compare as a branch condition: if (-30000 < 30000) -> 1 */
u16 r_ucmp16;       /* unsigned 16-bit: (in_30000 < 0xFFFF) + 2*(in_one > 0) + 4*(in_30000 <= in_30000b) + 8*(0 >= in_one) -> 7 */
u16 r_sizes;        /* sizeof(int)==2, long==4, void*==4, struct bits==2 -> 0x2442 */

/* ---- control flow, side effects, memory ----------------------------------- */
u16 r_shortcirc;    /* (0 && bump()) || (1 || bump()) ; bumps -> 0, result 1 -> 0x0100 | 1 = 0x0101 */
u16 r_ternary;      /* chain on in_seven -> 3 */
u16 r_dowhile;      /* do { } while (--n) from 5 -> 5 iterations */
u16 r_comma;        /* (a = 3, b = 4, a + b) -> 7 */
u16 r_ptr_rmw;      /* *p += 3 on a global via pointer -> 10 */
u16 r_postinc;      /* i = 4; r = i++; -> 4, then i -> 5 => 0x0504 */
u16 r_str;          /* "SNES"[2] far read -> 'E' = 0x45 */
u16 r_2d;           /* grid[2][3] far read -> 23 */
u16 r_struct_arr;   /* recs[1].v via pointer -> 200 */
u16 r_unary;        /* !in_one*100 + (~in_one & 0xF) + (-in_neg7) -> 0 + 14 + 7 = 21 */

static u16 bumps;
static u16 bump(void) { bumps++; return 1; }
static const char msg[] = "SNES";
static const u8 grid[3][4] = { {0, 1, 2, 3}, {10, 11, 12, 13}, {20, 21, 22, 23} };
struct rec { u16 id; u16 v; };
static struct rec recs[2];
static u16 rmw_target = 7;

int main(void) {
    u16 i, j, n, a, b, *p;
    fn_t f;
    struct bits bf;
    struct rec *rp;
    s32 s;

    /* switch */
    r_sw_dense = dense(in_seven);
    r_sw_sparse = sparse(in_1000);
    r_sw_default = sparse(in_100 + 1);
    r_sw_fall = fall(in_one);
    r_sw_neg = negsw(in_neg3);
    r_sw_32 = sw32(in_65536);
    n = 0;
    for (i = 0; i < 10; i++) {
        switch (i & 3) {
        case 0: continue;
        case 1: n += 1; break;
        case 2: n += 2; break;
        default: if (i > 6) goto sw_done; n += 0;
        }
    }
sw_done:
    r_sw_loop = n;   /* i=1:+1 2:+2 3:0 5:+1 6:+2 7:break -> 6 */

    /* function pointers */
    r_fp_const = const_tab[in_one](10);
    ram_tab[0] = inc; ram_tab[1] = dbl; ram_tab[2] = sq;
    r_fp_ram = ram_tab[in_one + 1](in_seven);
    r_fp_cb = apply(dbl, 21);
    r_fp_struct = op_rec.f(op_rec.k);
    f = in_one ? dbl : inc;
    r_fp_eq = (f == dbl ? 1 : 0) + (f != inc ? 2 : 0);

    /* bit-fields, enum */
    bf.a = 5; bf.b = 17; bf.c = (u16)in_200;
    r_bf_sum = bf.a + bf.b + bf.c;
    bf.a = (u16)(in_seven + 2); /* 9 = 1001b in 3 bits -> 001b (runtime value: the lint rejects a constant) */
    r_bf_ovf = bf.a;
    bf.b = 3;
    r_bf_c = bf.c;
    r_enum = BLUE + MAGENTA;
    r_enum_sw = enumsw(GREEN);

    /* goto */
    r_goto_fwd = 1;
    if (in_one) goto fwd;
    r_goto_fwd = 2;
fwd:
    i = 0;
back:
    if (++i < 4) goto back;
    r_goto_back = i;
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            if (i == 2 && j == 3) goto nested_out;
nested_out:
    r_goto_nested = i * 10 + j;

    /* recursion */
    r_fact = fact(6);
    r_fib = fib(10);
    r_mutual = is_even(in_seven) * 10 + is_odd(in_seven);

    /* 32-bit */
    r_mul32 = in_big * 0x100ul;
    r_mul32_wrap = in_max * 2ul;
    r_div32 = in_max / in_65536;
    r_mod32 = in_big % in_65536;
    s = in_negbig;
    r_smul32 = (u32)(s * 3l);
    r_sdiv32 = (u32)(s / 7l);
    r_smod32 = (u32)(s % 7l);
    r_shl_var = in_big << in_sh17;
    r_shr_var = in_max >> in_sh31;
    r_sar_var = (u32)(s >> in_sh17);
    r_shl1 = in_max << in_sh1;
    r_cmp32 = (in_big < in_max ? 1 : 0) + (in_65536 > in_big ? 2 : 0) + (in_big == 0x12345ul ? 4 : 0);
    r_scmp32 = (in_negbig < 0 ? 1 : 0) + (in_negbig < (s32)in_big ? 2 : 0) + ((s32)in_max < 0 ? 4 : 0);
    r_eq32 = (u16)((in_65536 == in_131072 ? 1 : 0) + (in_65536 != in_131072 ? 2 : 0) + (in_max == in_max2 ? 4 : 0));
    r_jnz32 = 0;
    if (in_65536) r_jnz32 = 1;
    r_lnot32 = (u16)((!in_65536 ? 1 : 0) + ((in_65536 && in_one) ? 2 : 0) + ((in_65536 || 0) ? 4 : 0));

    /* widths */
    r_widen_u = (u32)in_30000 * 4ul;
    r_widen_s = (u32)(s32)in_neg7;
    r_mul16to32 = (u32)((s32)in_neg3 * (s32)in_30000);
    r_trunc16 = (u16)in_big;
    r_trunc8 = (u8)in_big;
    r_promote8 = in_200 + in_100b;
    r_s8ext = (u16)((s16)in_neg5 * 3);
    r_u16wrap = (u16)(0xFFFFu + in_one);
    r_sdiv16 = (u16)(in_neg7 / 2);
    r_smod16 = (u16)(in_neg7 % 2);
    r_mixcmp = (in_neg1 < (u16)in_one) ? 1 : 0;
    r_scmp16 = (u16)((in_neg30000 < (s16)in_30000 ? 1 : 0) + ((s16)in_30000 > in_neg30000 ? 2 : 0) + (in_neg30000 <= in_neg1 ? 4 : 0) + (in_neg1 >= in_neg30000 ? 8 : 0));
    r_sbr16 = 0;
    if (in_neg30000 < (s16)in_30000) r_sbr16 = 1;
    r_ucmp16 = (u16)((in_30000 < 0xFFFFu ? 1 : 0) + (in_one > 0 ? 2 : 0) + (in_30000 <= in_30000b ? 4 : 0) + (0 >= in_one ? 8 : 0));
    r_sizes = (u16)((sizeof(int) << 12) | (sizeof(long) << 8) | (sizeof(void *) << 4) | sizeof(struct bits));

    /* control flow, side effects, memory */
    bumps = 0;
    r_shortcirc = (u16)((((0 && bump()) || (1 || bump())) ? 1 : 0) | (bumps << 8));
    r_ternary = in_seven > 10 ? 1 : in_seven > 5 ? 3 : 2;
    n = 5; i = 0;
    do { i++; } while (--n);
    r_dowhile = i;
    r_comma = (a = 3, b = 4, a + b);
    p = &rmw_target; *p += 3;
    r_ptr_rmw = rmw_target;
    i = 4; a = i++;
    r_postinc = (u16)((i << 8) | a);
    r_str = (u8)msg[in_one + 1];
    r_2d = grid[in_one + 1][3];
    recs[0].id = 1; recs[0].v = 100; recs[1].id = 2; recs[1].v = 200;
    rp = &recs[in_one];
    r_struct_arr = rp->v;
    r_unary = (!in_one) * 100 + (~in_one & 0xF) + (u16)(-in_neg7);

    consoleInit();
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
