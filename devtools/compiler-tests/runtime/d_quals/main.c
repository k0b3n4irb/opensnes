/*
 * d_quals — qualifiers and widths the front end dropped (2026-10-03).
 *
 * Four cproc miscompilations found by the pre-1.0 hunting campaign
 * (.claude/notes/reviews/2026-10-03_audit/A_compiler.md), each one a
 * common idiom that compiled without a word and passed check_bank_reads:
 *
 *   1. `++` / `--` on a FAR object read bank $7E and wrote bank $00
 *      (qbe.c, EXPRINCDEC stored with the expression's own empty qualifier);
 *   2. a whole-struct copy with a 4-aligned member copied two of its four
 *      bytes (funccopy's chunk table was upstream's, where `w` is 4 bytes);
 *   3. `= {0}` on a u32 array or a struct with an s32 left the upper halves
 *      unwritten (zero(), same table);
 *   4. a bit-field of a FAR object, or of const data read through a pointer,
 *      was read in bank $00 (expr.c did not carry the qualifier to the
 *      bit-field node).
 *
 * Each result global is one cell; test_d_quals.py compares it with the
 * value a correct compiler gives. The near_* cells are bank-0 controls.
 */
#include <snes.h>

volatile u16 in_one = 1;
volatile u16 in_17 = 17;

/* 1. ++ / -- on FAR objects */
FAR u16 far_count;
FAR u8 far_buf[16];
struct ent { u8 hp; u8 x; };
FAR struct ent far_ents[4];
FAR u32 far_long;
u16 near_count;

u16 r_inc_dir;      /* far_count = 10; far_count++; ++far_count  -> 12 */
u16 r_inc_idx;      /* far_buf[in_one] = 5; far_buf[in_one]++   -> 6 */
u16 r_dec_fld;      /* far_ents[in_one].hp = 3; twice --        -> 1 */
u16 r_inc_long;     /* far_long = 0xFFFF; far_long++; high word -> 1 */
u16 r_inc_near;     /* near_count = 10; near_count++; ++        -> 12 (control) */

/* 2. whole-struct copy with a 4-aligned member */
struct L { u32 v; u16 k; };
struct L ga = { 0x11223344ul, 0x5566 }, gb;
struct P { u16 x, y; };
struct P pa = { 0x1234, 0x5678 }, pb;

u16 r_copy_lo;      /* (u16)gb.v        -> 0x3344 */
u16 r_copy_hi;      /* (u16)(gb.v >> 16) -> 0x1122 */
u16 r_copy_k;       /* gb.k             -> 0x5566 */
u16 r_copy_p;       /* pb.x ^ pb.y (2-aligned struct, was refused) -> 0x444C */

/* 3. = {0} on 4-aligned objects (stack residue first) */
struct Z { s32 x; s32 y; u16 hp; };
u16 r_zero_arr;     /* u32 arr[4] = {0}: arr[1]|arr[2]|arr[3], both halves -> 0 */
u16 r_zero_struct;  /* struct Z z = {0}: every half of x, y and hp -> 0 */

/* 4. bit-fields of FAR and of const data through a pointer */
struct bits { u16 a : 3; u16 b : 5; u16 c : 8; };
FAR struct bits far_bits;
static const struct bits const_bits[2] = { { 5, 17, 200 }, { 1, 2, 3 } };
struct bits near_bits;

u16 r_bits_far;     /* far_bits = {5, 17, 200}: a + b + c -> 222 */
u16 r_bits_const0;  /* rdp(&const_bits[0])  -> 222 */
u16 r_bits_const1;  /* rdp(&const_bits[in_one]) -> 6 */
u16 r_bits_near;    /* near_bits = {5, 17, 200} -> 222 (control) */

u16 r_done;

static void dirty(void) {
    volatile u8 junk[64];
    u16 i;
    for (i = 0; i < 64; i++)
        junk[i] = 0xAA;
}

static u16 zarr(void) {
    u32 arr[4] = { 0 };
    u32 r = arr[1] | arr[2] | arr[3];
    return (u16)r | (u16)(r >> 16);
}

static u16 zstruct(void) {
    struct Z z = { 0 };
    return (u16)z.x | (u16)(z.x >> 16) | (u16)z.y | (u16)(z.y >> 16) | z.hp;
}

static u16 rdp(const struct bits *p) {
    return p->a + p->b + p->c;
}

int main(void) {
    u16 i;

    /* 1 */
    far_count = 10;
    far_count++;
    ++far_count;
    r_inc_dir = far_count;
    far_buf[in_one] = 5;
    far_buf[in_one]++;
    r_inc_idx = far_buf[1];
    far_ents[in_one].hp = 3;
    for (i = 0; i < 2; i++)
        far_ents[in_one].hp--;
    r_dec_fld = far_ents[1].hp;
    far_long = 0xFFFFul;
    far_long++;
    r_inc_long = (u16)(far_long >> 16);
    near_count = 10;
    near_count++;
    ++near_count;
    r_inc_near = near_count;

    /* 2 */
    gb = ga;
    r_copy_lo = (u16)gb.v;
    r_copy_hi = (u16)(gb.v >> 16);
    r_copy_k = gb.k;
    pb = pa;
    r_copy_p = pb.x ^ pb.y;

    /* 3 */
    dirty();
    r_zero_arr = zarr();
    dirty();
    r_zero_struct = zstruct();

    /* 4 */
    far_bits.a = 5;
    far_bits.b = in_17;
    far_bits.c = 200;
    r_bits_far = far_bits.a + far_bits.b + far_bits.c;
    r_bits_const0 = rdp(&const_bits[0]);
    r_bits_const1 = rdp(&const_bits[in_one]);
    near_bits.a = 5;
    near_bits.b = in_17;
    near_bits.c = 200;
    r_bits_near = near_bits.a + near_bits.b + near_bits.c;

    r_done = 0xBEEF;
    consoleInit();
    setScreenOn();
    while (1)
        WaitForVBlank();
    return 0;
}
