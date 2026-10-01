/**
 * @file superfx.c
 * @brief SuperFX (GSU) library — C initialization
 */

#include <snes/superfx.h>
#include <snes/console.h>
#include <snes/registers.h>

/* gsuInit() and gsuIsPresent() are `inline` in superfx.h. Force-emit canonical here. */
u8 (*const __opensnes_force_emit_gsuInit)(void) = gsuInit;
u8 (*const __opensnes_force_emit_gsuIsPresent)(void) = gsuIsPresent;

void gsuSetProgram(const void *program) {
    /* A 4-byte far pointer (chantier A6): bank in bits 16-23. */
    u32 far_address = (u32)program;
    gsu_prog_addr = (u16)far_address;
    gsu_prog_bank = (u8)(far_address >> 16);
}

void gsuCall(u16 entry) {
    /* the program's own base, as gsuSetProgram() left it */
    u16 base = gsu_prog_addr;

    gsu_prog_addr = (u16)(base + entry);
    gsuLaunch();
    gsu_prog_addr = base;
}

/* Cache-resident jobs (2026-09-27). GSU I/O is at $3000-$34FF of banks
 * $00-$3F: plain C pointers reach it, bank $00. */
#define GSU_R8    (*(volatile u16 *)0x3010)
#define GSU_R15L  (*(volatile u8 *)0x301E)
#define GSU_R15H  (*(volatile u8 *)0x301F)
#define GSU_SFRL  (*(volatile u8 *)0x3030)
#define GSU_SFRH  (*(volatile u8 *)0x3031)
#define GSU_CFGR  (*(volatile u8 *)0x3037)
#define GSU_SCBR  (*(volatile u8 *)0x3038)
#define GSU_CLSR  (*(volatile u8 *)0x3039)
#define GSU_SCMR  (*(volatile u8 *)0x303A)
#define GSU_CACHE ((volatile u8 *)0x3100)
#define GSU_SFR_GO 0x20
#define GSU_SCMR_RON 0x10

/* gsuCacheLoad is in superfx.asm since 2026-09-29: the C loop (a far read
 * and a volatile write per byte) took up to 105 000 master cycles, a third
 * of a frame, for the 224 bytes of a small renderer. */

void gsuStartCached(u16 pc) {
    GSU_CFGR = gsu_cfgr;
    GSU_CLSR = 1;                                   /* 21.47 MHz */
    GSU_SCBR = gsu_scbr;
    GSU_R8 = (u16)((u16)gsu_scbr << 10);            /* buffer base, as gsuLaunch() */
    gsu_scmr_live = (u8)(gsu_scmr & ~GSU_SCMR_RON); /* the shadow first */
    GSU_SCMR = gsu_scmr_live;                       /* the ROM stays the CPU's */
    GSU_R15L = (u8)pc;
    GSU_R15H = (u8)(pc >> 8);                       /* writing R15 high starts it */
}

u8 gsuBusy(void) {
    return (GSU_SFRL & GSU_SFR_GO) ? 1 : 0;
}

void gsuWait(void) {
    while (GSU_SFRL & GSU_SFR_GO) {
    }
    gsu_scmr_live = 0;      /* the shadow first: see superfx.asm */
    GSU_SCMR = 0;
}

/*============================================================================
 * Presentation (phase D, 2026-09-29): the NMI step is gsu_present_step in
 * superfx.asm; these set it up and feed it.
 *============================================================================*/

extern u8 gsu_pres_busy, gsu_pres_flags, gsu_pres_nba_back, gsu_pres_nba_front;
extern u8 gsu_pres_scbr_a, gsu_pres_scbr_b;
extern u16 gsu_pres_src, gsu_pres_off, gsu_pres_size;
extern u16 gsu_pres_vram_back, gsu_pres_vram_front, gsu_pres_vtotal;
extern u8 bg12nba_shadow;   /* background.c */

u16 gsuFrameBytes(void) {
    u8 m = gsu_scmr;
    u16 rows, tile;

    switch (m & 0x24) {      /* HT0 = bit 2, HT1 = bit 5 (fullsnes SCMR) */
    case 0x00: rows = 16; break;
    case 0x04: rows = 20; break;
    case 0x20: rows = 24; break;
    default:   return 0;    /* OBJ mode */
    }
    switch (m & 0x03) {      /* MD: 4, 16, reserved, 256 colours */
    case 0x00: tile = 16; break;
    case 0x01: tile = 32; break;
    case 0x03: tile = 64; break;
    default:   return 0;
    }
    return (u16)((rows * tile) << 5);   /* 32 columns */
}

u8 gsuPresentInit(u16 vram_a, u16 vram_b, u8 flags) {
    u16 size = gsuFrameBytes();
    u16 words, ram_kb, need_kb;

    gsu_pres_busy = 0;
    gsu_pres_size = 0;
    if (size == 0)
        return 0;
    /* BG12NBA counts BG1's char base in 4K-word steps */
    words = size >> 1;
    if ((vram_a & 0x0FFF) || (vram_b & 0x0FFF))
        return 0;
    if (vram_a > (u16)(0x8000 - words) || vram_b > (u16)(0x8000 - words))
        return 0;
    /* both buffers in Game Pak RAM: header $FFBD, 1 KB << n */
    ram_kb = (u16)(1u << *(const u8 *)0xFFBD);
    need_kb = (u16)(gsu_scbr + ((size >> 10) << 1));
    if (need_kb > ram_kb)
        return 0;

    gsu_pres_scbr_a = gsu_scbr;
    gsu_pres_scbr_b = (u8)(gsu_scbr + (size >> 10));
    gsu_pres_vram_front = vram_a;
    gsu_pres_vram_back = vram_b;
    gsu_pres_nba_front = (u8)(vram_a >> 12);
    gsu_pres_nba_back = (u8)(vram_b >> 12);
    gsu_pres_vtotal = isPAL() ? 312 : 262;
    gsu_pres_frames = 0;
    gsu_pres_last = 0;
    gsu_pres_flags = flags;
    bg12nba_shadow = (u8)((bg12nba_shadow & 0xF0) | gsu_pres_nba_front);
    REG_BG12NBA = bg12nba_shadow;
    gsu_pres_size = size;
    return 1;
}

void gsuPresent(void) {
    if (gsu_pres_size == 0)
        return;
    while (gsu_pres_busy)
        WaitForVBlank();
    gsu_pres_src = (u16)((u16)gsu_scbr << 10);
    gsu_pres_off = 0;
    gsu_pres_busy = 1;      /* last: the NMI reads it */
    gsu_scbr = (gsu_scbr == gsu_pres_scbr_a) ? gsu_pres_scbr_b : gsu_pres_scbr_a;
}

u8 gsuPresentBusy(void) {
    return gsu_pres_busy;
}

void gsuPresentWait(void) {
    while (gsu_pres_busy)
        WaitForVBlank();
}
