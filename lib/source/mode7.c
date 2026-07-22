/**
 * @file mode7.c
 * @brief Mode 7 matrix control — C port of mode7.asm (C1 audit, module 1)
 *
 * Faithful migration of the 481-line ASM original (itself based on
 * PVSnesLib's implementation by Alekmaul). Every hardware trick is
 * preserved:
 * - the PPU 16x8 signed multiplier (write M7A twice + M7B once, read
 *   the product's high 16 bits at $2135/$2136) computes the four
 *   matrix terms — no software multiply on the trig path;
 * - PPU double-write registers (M7x, M7HOFS/VOFS, M7X/Y) get their
 *   low byte then high byte through volatile stores (honoured by QBE
 *   since chantier A2 — the two writes cannot be coalesced).
 *
 * Benchmarked against the ASM original with devtools/benchrom under
 * the C1 audit's ±10 % rule; numbers in
 * .claude/notes/chantiers/c1_asm_audit.md.
 *
 * Main-thread only (PPU multiplier + matrix writes; same contract as
 * the ASM had).
 */

#include <snes/mode7.h>

/* PPU registers (double-write: low byte first, then high) */
#define REG_M7SEL  (*(volatile u8 *)0x211A)
#define REG_M7A    (*(volatile u8 *)0x211B)
#define REG_M7B    (*(volatile u8 *)0x211C)
#define REG_M7C    (*(volatile u8 *)0x211D)
#define REG_M7D    (*(volatile u8 *)0x211E)
#define REG_M7X    (*(volatile u8 *)0x211F)
#define REG_M7Y    (*(volatile u8 *)0x2120)
#define REG_M7HOFS (*(volatile u8 *)0x210D)
#define REG_M7VOFS (*(volatile u8 *)0x210E)
#define REG_MPYM   (*(volatile u8 *)0x2135)
#define REG_MPYH   (*(volatile u8 *)0x2136)
/* MPYM+MPYH as one 16-bit read — exactly the ASM's `lda.l $2135` */
#define REG_MPY16  (*(volatile u16 *)0x2135)

/* Module state (WRAM) — mirrors the ASM's .mode7vars */
static u16 m7_scale_x;
static u16 m7_scale_y;
static s8 m7_sin;
static s8 m7_cos;


/* Sin LUT: 256 entries, signed 8-bit, index 64 = cos(0) = 127.
 * Same table bytes as the ASM original. Deliberately NON-const: as
 * ROM const it must sit in bank $00 for direct C indexing, and the
 * tightest mode7 example (mode7_perspective, 12 bytes free) can't
 * host 256 more bytes there — it spilled to bank $01 and the symmap
 * guard caught the garbage-read. As an initialized static it lives
 * in WRAM (copied at boot by the bank-aware data_init loop, the fix
 * check_lib_rodata.py documents), is bank-safe by construction, and
 * indexes faster than any ROM access. Cost: 256 B of WRAM + the
 * 256 B init image, only for examples linking mode7. */
static s8 m7_sincos_table[256] = {
      0,   3,   6,   9,  12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,
     48,  51,  54,  57,  59,  62,  65,  67,  70,  73,  75,  78,  80,  82,  85,  87,
     89,  91,  94,  96,  98, 100, 102, 103, 105, 107, 108, 110, 112, 113, 114, 116,
    117, 118, 119, 120, 121, 122, 123, 123, 124, 125, 125, 126, 126, 126, 126, 126,
    127, 126, 126, 126, 126, 126, 125, 125, 124, 123, 123, 122, 121, 120, 119, 118,
    117, 116, 114, 113, 112, 110, 108, 107, 105, 103, 102, 100,  98,  96,  94,  91,
     89,  87,  85,  82,  80,  78,  75,  73,  70,  67,  65,  62,  59,  57,  54,  51,
     48,  45,  42,  39,  36,  33,  30,  27,  24,  21,  18,  15,  12,   9,   6,   3,
      0,  -3,  -6,  -9, -12, -15, -18, -21, -24, -27, -30, -33, -36, -39, -42, -45,
    -48, -51, -54, -57, -59, -62, -65, -67, -70, -73, -75, -78, -80, -82, -85, -87,
    -89, -91, -94, -96, -98,-100,-102,-103,-105,-107,-108,-110,-112,-113,-114,-116,
   -117,-118,-119,-120,-121,-122,-123,-123,-124,-125,-125,-126,-126,-126,-126,-126,
   -127,-126,-126,-126,-126,-126,-125,-125,-124,-123,-123,-122,-121,-120,-119,-118,
   -117,-116,-114,-113,-112,-110,-108,-107,-105,-103,-102,-100, -98, -96, -94, -91,
    -89, -87, -85, -82, -80, -78, -75, -73, -70, -67, -65, -62, -59, -57, -54, -51,
    -48, -45, -42, -39, -36, -33, -30, -27, -24, -21, -18, -15, -12,  -9,  -6,  -3,
};

/* Helpers as MACROS on purpose: a function version costs ~340
 * cycles/call in call overhead (post-A6 4-byte pointer arg + frame)
 * — measured with benchrom, it swamped every register write. Direct
 * volatile stores to constant addresses compile to sta.l absolute. */

/** Write one 16-bit value to a PPU double-write register (low, high).
 * Direct form measured BEST: u8-temp pre-extraction (tried) adds
 * stack traffic and loses ~45 %; the remaining gap vs ASM is the
 * per-store sep/rep churn (see the chantier note's analysis). */
#define W16(reg, v) do { (reg) = (u8)(v); (reg) = (u8)((u16)(v) >> 8); } while (0)

/** PPU 16x8 signed multiply into `out`: (m16 * m8) >> 8 (M7A/B trick) */
#define PPU_MUL(out, m16, m8) do { \
    REG_M7A = (u8)(m16); \
    REG_M7A = (u8)((u16)(m16) >> 8); \
    REG_M7B = (u8)(m8);              /* triggers the multiply */ \
    (out) = REG_MPY16;               /* one 16-bit read, as the ASM */ \
} while (0)

void mode7Init(void) {
    m7_scale_x = 0x0100;
    m7_scale_y = 0x0100;

    W16(REG_M7A, 0x0100);       /* identity matrix */
    W16(REG_M7B, 0);
    W16(REG_M7C, 0);
    W16(REG_M7D, 0x0100);

    W16(REG_M7X, 128);          /* center of rotation: screen center */
    W16(REG_M7Y, 128);

    W16(REG_M7HOFS, 0);         /* scroll centered on the mode 7 plane */
    W16(REG_M7VOFS, 0x0180);
}

void mode7SetScale(u16 scale_x, u16 scale_y) {
    m7_scale_x = scale_x;
    m7_scale_y = scale_y;
}

void mode7SetAngle(u8 angle) {
    u16 a, b, c, d;
    u16 sx, sy;
    s8 sn, cs;

    sn = m7_sincos_table[angle];
    cs = m7_sincos_table[(u8)(angle + 64)];
    m7_sin = sn;
    m7_cos = cs;

    /* locals on purpose: the byte-pair store fusion needs the two
     * M7A writes to split the SAME temp, and globals reload per use */
    sx = m7_scale_x;
    sy = m7_scale_y;

    PPU_MUL(b, sx, (s8)-sn);
    PPU_MUL(c, sy, sn);
    PPU_MUL(a, sx, cs);
    PPU_MUL(d, sy, cs);

    W16(REG_M7A, (u16)a);
    W16(REG_M7B, (u16)b);
    W16(REG_M7C, (u16)c);
    W16(REG_M7D, (u16)d);
}

void mode7SetCenter(s16 x, s16 y) {
    W16(REG_M7X, (u16)x);
    W16(REG_M7Y, (u16)y);
}

void mode7SetScroll(s16 x, s16 y) {
    W16(REG_M7HOFS, (u16)x);
    W16(REG_M7VOFS, (u16)y);
}

void mode7Rotate(u16 degrees) {
    /* degrees (0-359) -> 0-255: (degrees * 182) >> 8. degrees*182
     * peaks at 65 338 — fits u16, one 16-bit multiply. */
    mode7SetAngle((u8)((u16)(degrees * 182) >> 8));
}

void mode7Transform(u16 degrees, u16 scalePercent) {
    /* scale = percent * 2.5 (the ASM's approximation of * 2.56) */
    u16 s = (u16)(scalePercent << 1) + (scalePercent >> 1);
    m7_scale_x = s;
    m7_scale_y = s;
    mode7Rotate(degrees);
}

void mode7SetPivot(u8 x, u8 y) {
    W16(REG_M7X, x);
    W16(REG_M7Y, y);
}

void mode7SetMatrix(s16 a, s16 b, s16 c, s16 d) {
    W16(REG_M7A, (u16)a);
    W16(REG_M7B, (u16)b);
    W16(REG_M7C, (u16)c);
    W16(REG_M7D, (u16)d);
}

void mode7SetSettings(u8 settings) {
    REG_M7SEL = settings;
}
