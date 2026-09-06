/*
 * B2 far-RAM runtime MATRIX (Phase 0 gate).
 *
 * Each result global is one cell: an access to a FAR (bank-$7E) object crossed by
 *   {direct symbol, sym[idx], runtime pointer, param, struct field}
 *   x {byte, word, long} x {store then load}.
 * Every cell writes a sentinel into the far object, reads it back and parks
 * the read-back in bank-0 WRAM; the Python side compares with the sentinel.
 * A red cell names the emit path that dropped the bank (see the B2 plan,
 * .claude/notes/chantiers/b2_far_ram.md §2). Bank-0 control cells (c0_*)
 * exercise the same forms on a bank-0 object and must stay green throughout.
 *
 * Indices and pointers are `volatile` so they stay opaque (no folding to a
 * direct symbol access): they exercise the runtime-address emit paths.
 */
#include <snes.h>

typedef struct { u8 a; u8 b; u16 w; u32 l; } far_rec;

/* Phase 1: the objects are declared FAR (__far) — cproc sections them into
 * BANK $7E SLOT 2 and taints every access; Phase 2 makes the backend honour
 * the taint on the runtime-address forms (the red cells). */
FAR u8  far_u8;
FAR u16 far_u16;
FAR u32 far_u32;
FAR u8  far_arr8[8];
FAR u16 far_arr16[8];
FAR u32 far_arr32[4];
FAR far_rec far_struct;
FAR far_rec far_recs[3];
/* Phase 2: initialised far data — the init record carries the bank byte
 * and crt0's CopyInitData writes through a 24-bit pointer. */
FAR u16 far_init[4] = { 0x1111, 0x2222, 0x3333, 0x4444 };
FAR u8  far_init8 = 0xA7;
FAR u8  far_walk[16];

/* bank-0 twins (controls) */
u8  near_u8;
u16 near_u16;
u8  near_arr8[8];

volatile u8  vi = 3;          /* opaque index */
volatile u8  FAR *vp8;        /* opaque far pointers */
volatile u16 FAR *vp16;
volatile u32 FAR *vp32;

/* results (bank 0) */
u8  r_dir8,  r_idx8,  r_ptr8,  r_par8,  r_parl8, r_fld8;
u16 r_dir16, r_idx16, r_ptr16, r_fld16;
u32 r_dir32, r_ptr32, r_fld32, r_idx32;
u16 r_init, r_init8, r_pfld16, r_walk, r_zero;
u32 r_pfld32;
u8  c0_dir8, c0_idx8, c0_ptr8;
u16 c0_dir16;
u16 r_hi;                     /* bank half of &far_u8 as the C side sees it */

/* non-static -> not inlined -> the pointer arrives through a param (RSlot).
 * The store is verified by a DIRECT symbol read in main (store-to-load
 * forwarding inside the callee would otherwise hide a bank-blind store). */
void wr_param(u8 FAR *p, u8 v) { *p = v; }
u8   rd_param(u8 FAR *p)       { return *p; }

/* struct fields through a far pointer (base + constant offset forms) */
void set_rec(far_rec FAR *r, u16 w, u32 l) { r->w = w; r->l = l; }
u16  get_rec_w(far_rec FAR *r)             { return r->w; }
u32  get_rec_l(far_rec FAR *r)             { return r->l; }

/* pointer walk (base + runtime index form) */
void fill_walk(u8 FAR *p, u8 n) { u8 k; for (k = 0; k < n; k++) p[k] = (u8)(k * 3); }
u16  sum_walk(u8 FAR *p, u8 n)  { u8 k; u16 s = 0; for (k = 0; k < n; k++) s += p[k]; return s; }

int main(void) {
    u8 i;

    /* direct symbol */
    far_u8  = 0x5A;           r_dir8  = far_u8;
    far_u16 = 0xBEEF;         r_dir16 = far_u16;
    far_u32 = 0x12345678UL;   r_dir32 = far_u32;

    /* sym[idx], runtime index */
    i = vi;
    far_arr8[i]  = 0xC3;      r_idx8  = far_arr8[i];
    far_arr16[i] = 0xCAFE;    r_idx16 = far_arr16[i];

    /* runtime pointer deref */
    vp8  = &far_arr8[5];      *vp8  = 0x77;           r_ptr8  = *vp8;
    vp16 = &far_arr16[6];     *vp16 = 0xD00D;         r_ptr16 = *vp16;
    vp32 = &far_u32;          *vp32 = 0xA5A5F00FUL;   r_ptr32 = *vp32;

    /* pointer through a parameter (RSlot path): store, then load */
    wr_param(&far_arr8[1], 0x42);   r_par8  = far_arr8[1];
    far_arr8[2] = 0x69;             r_parl8 = rd_param(&far_arr8[2]);

    /* 32-bit array element, runtime index */
    far_arr32[i] = 0xDEADBEEFUL; r_idx32 = far_arr32[i];

    /* struct fields through a far pointer: base+const forms */
    set_rec(&far_recs[2], 0x7777, 0x89ABCDEFUL);
    r_pfld16 = get_rec_w(&far_recs[2]);
    r_pfld32 = get_rec_l(&far_recs[2]);

    /* pointer walk: base+index forms; 0+3+...+45 = 360 */
    fill_walk(far_walk, 16);
    r_walk = sum_walk(far_walk, 16);

    /* initialised far data (CopyInitData with the bank byte) */
    r_init  = far_init[i];        /* i == 3 → 0x4444 */
    r_init8 = far_init8;

    /* the boot zero-fill: an untouched far byte reads 0 (0x100 marks "checked") */
    r_zero = 0x100 | far_arr8[7];

    /* struct fields via the symbol */
    far_struct.b = 0x21;      r_fld8  = far_struct.b;
    far_struct.w = 0x4321;    r_fld16 = far_struct.w;
    far_struct.l = 0x0BADF00DUL; r_fld32 = far_struct.l;

    /* bank-0 controls */
    near_u8  = 0x5A;          c0_dir8  = near_u8;
    near_u16 = 0xBEEF;        c0_dir16 = near_u16;
    near_arr8[i] = 0xC3;      c0_idx8  = near_arr8[i];
    { volatile u8 *np = &near_arr8[5]; *np = 0x77;    c0_ptr8 = *np; }

    r_hi = (u16)(((u32)(u8 *)&far_u8) >> 16);

    consoleInit();
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
