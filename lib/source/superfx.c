/**
 * @file superfx.c
 * @brief SuperFX (GSU) library — C initialization
 */

#include <snes/superfx.h>

/* gsuInit() and gsuIsPresent() are `inline` in superfx.h. Force-emit canonical here. */
u8 (*const __opensnes_force_emit_gsuInit)(void) = gsuInit;
u8 (*const __opensnes_force_emit_gsuIsPresent)(void) = gsuIsPresent;

void gsuSetProgram(const void *program) {
    /* A 4-byte far pointer (chantier A6): bank in bits 16-23. */
    u32 far_address = (u32)program;
    gsu_prog_addr = (u16)far_address;
    gsu_prog_bank = (u8)(far_address >> 16);
}

/* Cache-resident jobs (2026-09-27). GSU I/O is at $3000-$34FF of banks
 * $00-$3F: plain C pointers reach it, bank $00. */
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

void gsuCacheLoad(const void *code, u16 size) {
    const u8 *src = (const u8 *)code;
    u16 i, end;

    if (size > 512)
        size = 512;
    /* GO = 0: the GSU stops, CBR becomes 0, every cache line is empty */
    GSU_SFRL = 0;
    GSU_SFRH = 0;
    for (i = 0; i < size; i++)
        GSU_CACHE[i] = src[i];
    /* fill the last line: NOP is $01 */
    end = (u16)((size + 15) & ~15u);
    for (; i < end; i++)
        GSU_CACHE[i] = 0x01;
}

void gsuStartCached(u16 pc) {
    GSU_CFGR = gsu_cfgr;
    GSU_CLSR = 1;                                   /* 21.47 MHz */
    GSU_SCBR = gsu_scbr;
    GSU_SCMR = (u8)(gsu_scmr & ~GSU_SCMR_RON);      /* the ROM stays the CPU's */
    GSU_R15L = (u8)pc;
    GSU_R15H = (u8)(pc >> 8);                       /* writing R15 high starts it */
}

u8 gsuBusy(void) {
    return (GSU_SFRL & GSU_SFR_GO) ? 1 : 0;
}

void gsuWait(void) {
    while (GSU_SFRL & GSU_SFR_GO) {
    }
    GSU_SCMR = 0;
}
