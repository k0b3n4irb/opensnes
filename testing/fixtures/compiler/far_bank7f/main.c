/*
 * far_bank7f: C objects in WRAM bank $7F, declared by assembly.
 *
 * data.asm places the objects with `.RAMSECTION "x" BANK $7F SLOT 3`; here
 * they are `extern FAR`. The library puts nothing in $7F, so this is the
 * recipe a game follows for a bulk buffer beyond the $7E band, and this
 * fixture is what proves the compiler follows the bank byte there.
 *
 * Each result global is one cell, in bank 0; the Python side compares it
 * with the sentinel. Forms: (a) direct symbol, (b) symbol[idx] with a
 * volatile index, (c) a run-time `volatile T FAR *`, (d) a parameter,
 * (e) memcpy / memset of <snes/string.h>, (f) a $7E and a $7F object read in
 * one function, (g) a bank-0 control. Indices cross 256 bytes, and one is a
 * 16-bit offset above $8000 inside the bank.
 */
#include <snes.h>
#include <snes/string.h>

extern FAR u16 f7_w[];        /* 0x4400 words, 34 816 bytes */
extern FAR u16 f7_v[];        /* 16 words, placed after f7_w */
extern FAR u8  f7_s8;
extern FAR u16 f7_s16;
extern FAR u32 f7_s32;
extern FAR u8  f7_u8[];       /* 300 bytes */
extern FAR u32 f7_u32[];      /* 100 words */
extern FAR u16 e7_x[];        /* $7E:2000, 8 words */
extern FAR u16 f7_x[];        /* $7F:2000, 8 words */

#define W_HI 0x4200           /* f7_w[W_HI]: offset past $8000 in the bank */

static const u8 rom_tab[16] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xF1, 0xF2
};

u16 near_w[300];              /* bank-0 control */

volatile u16 vlo = 200;       /* opaque indices */
volatile u16 vhi = W_HI;
volatile u16 v8  = 270;
volatile u16 v32 = 70;
volatile u16 vn  = 5;

volatile u8  FAR *vp8;        /* opaque far pointers */
volatile u16 FAR *vp16;
volatile u32 FAR *vp32;

/* results (bank 0) */
u8  r_a8;  u16 r_a16;  u32 r_a32;
u8  r_b8;  u16 r_b16, r_bhi;  u32 r_b32;
u8  r_c8;  u16 r_c16, r_chi;  u32 r_c32;
u8  r_d8;  u16 r_d16, r_dhi;  u32 r_d32;
u8  r_e_c0, r_e_c15, r_e_before, r_e_s0, r_e_s3, r_e_after;
u16 r_e_w0, r_e_w7;
u16 r_f_lo, r_f_hi, r_f_lo2, r_f_hi2;
u16 r_g16;
u16 r_ovl_a, r_ovl_b, r_ovl_c, r_ord, r_bank_w, r_bank_x, r_bank_e;

/* parameter forms: the pointer arrives through a call */
void wr8(u8 FAR *p, u8 v)      { *p = v; }
u8   rd8(u8 FAR *p)            { return *p; }
void wr16(u16 FAR *p, u16 v)   { *p = v; }
u16  rd16(u16 FAR *p)          { return *p; }
void wr32(u32 FAR *p, u32 v)   { *p = v; }
u32  rd32(u32 FAR *p)          { return *p; }

/* a $7E object and a $7F one at the same offset, one function */
void swap_banks(u16 i) {
    u16 t = e7_x[i];
    e7_x[i] = f7_x[i];
    f7_x[i] = t;
}

int main(void) {
    u16 k, bad;

    /* (a) direct symbol */
    f7_s8  = 0x5A;          r_a8  = f7_s8;
    f7_s16 = 0xBEEF;        r_a16 = f7_s16;
    f7_s32 = 0x12345678UL;  r_a32 = f7_s32;

    /* (b) symbol[idx]: 270 bytes, word 200 = byte 400, word 0x4200 = byte
     * 0x8400 of the object, word 70 of 32 bits = byte 280 */
    f7_u8[v8]   = 0xC3;          r_b8  = f7_u8[v8];
    f7_w[vlo]   = 0xCAFE;        r_b16 = f7_w[vlo];
    f7_w[vhi]   = 0xD00D;        r_bhi = f7_w[vhi];
    f7_u32[v32] = 0xDEADBEEFUL;  r_b32 = f7_u32[v32];

    /* (c) run-time pointer */
    vp8  = &f7_u8[271];     *vp8  = 0x77;             r_c8  = *vp8;
    vp16 = &f7_w[201];      *vp16 = 0xA11A;           r_c16 = *vp16;
    vp16 = &f7_w[W_HI + 1]; *vp16 = 0xB22B;           r_chi = *vp16;
    vp32 = &f7_u32[71];     *vp32 = 0xA5A5F00FUL;     r_c32 = *vp32;

    /* (d) parameter: stored by a call and read by symbol, then stored by
     * symbol and read by a call */
    wr8(&f7_u8[272], 0x42);        r_d8  = f7_u8[272];
    f7_w[202] = 0x6969;            r_d16 = rd16(&f7_w[202]);
    wr16(&f7_w[W_HI + 2], 0x3C3C); r_dhi = f7_w[W_HI + 2];
    wr32(&f7_u32[72], 0x0BADF00DUL);
    f7_u32[73] = 0x89ABCDEFUL;
    r_d32 = rd32(&f7_u32[72]) ^ rd32(&f7_u32[73]);   /* 0x82063DE2 */

    /* (e) memcpy from ROM, memset over part of it */
    memcpy(&f7_u8[260], rom_tab, 16);
    r_e_c0 = f7_u8[260];
    r_e_c15 = f7_u8[275];
    memset(&f7_u8[262], 0xEE, 4);              /* bytes 262..265 */
    r_e_before = f7_u8[261];                   /* 0x22 */
    r_e_s0     = f7_u8[262];
    r_e_s3     = f7_u8[265];
    r_e_after  = f7_u8[266];                   /* rom_tab[6] = 0x77 */
    memcpy(&f7_w[W_HI + 8], rom_tab, 16);      /* ROM -> offset past $8000 */
    r_e_w0 = f7_w[W_HI + 8];                   /* 0x2211 */
    r_e_w7 = f7_w[W_HI + 15];                  /* 0xF2F1 */

    /* (f) $7E and $7F at the same offset, same function */
    for (k = 0; k < 8; k++) { e7_x[k] = 0xE700 + k; f7_x[k] = 0xF700 + k; }
    swap_banks(vn);
    r_f_lo  = e7_x[5];                         /* was f7_x[5] = 0xF705 */
    r_f_hi  = f7_x[5];                         /* was e7_x[5] = 0xE705 */
    r_f_lo2 = e7_x[4];                         /* untouched 0xE704 */
    r_f_hi2 = f7_x[6];                         /* untouched 0xF706 */

    /* (g) bank-0 control */
    near_w[vlo] = 0x7E7F;   r_g16 = near_w[vlo];

    /* no overlap: fill f7_w, fill f7_v after it, count the strays */
    for (k = 0; k < 0x4400; k++) f7_w[k] = 0x1111;
    for (k = 0; k < 16; k++)     f7_v[k] = 0x2222;
    bad = 0;
    for (k = 0; k < 0x4400; k++) if (f7_w[k] != 0x1111) bad++;
    r_ovl_a = bad;
    bad = 0;
    for (k = 0; k < 16; k++)     if (f7_v[k] != 0x2222) bad++;
    r_ovl_b = bad;
    /* the neighbours in f7_c / f7_b: u8[300] ends where u32[] starts, u32[]
     * ends where f7_v starts; scalars before them keep their value */
    for (k = 0; k < 100; k++) f7_u32[k] = 0x33333333UL;
    for (k = 0; k < 300; k++) f7_u8[k] = 0x44;
    bad = 0;
    for (k = 0; k < 100; k++) if (f7_u32[k] != 0x33333333UL) bad++;
    for (k = 0; k < 16; k++)  if (f7_v[k] != 0x2222) bad++;
    if (f7_s8 != 0x5A) bad++;
    if (f7_s16 != 0xBEEF) bad++;
    if (f7_s32 != 0x12345678UL) bad++;
    r_ovl_c = bad;
    {
        u32 w = (u32)(u8 *)f7_w, v = (u32)(u8 *)f7_v;
        r_ord = (v >= w + 0x8800UL || w >= v + 32UL) ? 1 : 0;
        r_bank_w = (u16)(w >> 16);
        r_bank_x = (u16)(((u32)(u8 *)f7_x) >> 16);
        r_bank_e = (u16)(((u32)(u8 *)e7_x) >> 16);
    }

    consoleInit();
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
