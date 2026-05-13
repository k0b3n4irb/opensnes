/**
 * @file superfx.h
 * @brief SuperFX (GSU) coprocessor library — registers, config, and API
 *
 * Provides a complete C API for SuperFX cartridges:
 * - GSU detection and configuration
 * - WRAM-safe GSU launch (mandatory — CPU can't read ROM during GSU execution)
 * - DMA helpers for SRAM-to-VRAM framebuffer transfer
 * - HDMA screen blanking for 60 FPS DMA bandwidth
 * - Column-major tilemap setup for PLOT rendering
 *
 * The GSU code itself is written in SuperFX assembly (.sfx files).
 * The C API handles all SNES-side boilerplate.
 *
 * @see docs/tutorials/superfx.md
 */

#ifndef SNES_SUPERFX_H
#define SNES_SUPERFX_H

#include <snes/types.h>

/*============================================================================
 * GSU Register Definitions ($3000-$303F)
 *============================================================================*/

#define REG_GSU_R0     (*(volatile u16*)0x3000)
#define REG_GSU_R1     (*(volatile u16*)0x3002)
#define REG_GSU_R2     (*(volatile u16*)0x3004)
#define REG_GSU_R3     (*(volatile u16*)0x3006)
#define REG_GSU_R4     (*(volatile u16*)0x3008)
#define REG_GSU_R5     (*(volatile u16*)0x300A)
#define REG_GSU_R6     (*(volatile u16*)0x300C)
#define REG_GSU_R7     (*(volatile u16*)0x300E)
#define REG_GSU_R8     (*(volatile u16*)0x3010)
#define REG_GSU_R9     (*(volatile u16*)0x3012)
#define REG_GSU_R10    (*(volatile u16*)0x3014)
#define REG_GSU_R11    (*(volatile u16*)0x3016)
#define REG_GSU_R12    (*(volatile u16*)0x3018)
#define REG_GSU_R13    (*(volatile u16*)0x301A)
#define REG_GSU_R14    (*(volatile u16*)0x301C)
#define REG_GSU_R15    (*(volatile u16*)0x301E)

#define REG_SFR        (*(volatile u16*)0x3030)
#define REG_SFR_L      (*(volatile u8*)0x3030)
#define SFR_GO         0x20

#define REG_BRAMR      (*(volatile u8*)0x3033)
#define REG_PBR        (*(volatile u8*)0x3034)
#define REG_ROMBR      (*(volatile u8*)0x3036)
#define REG_SCBR       (*(volatile u8*)0x3038)
#define REG_CLSR       (*(volatile u8*)0x3039)
#define REG_SCMR       (*(volatile u8*)0x303A)
#define REG_VCR        (*(volatile u8*)0x303B)
#define REG_RAMBR      (*(volatile u8*)0x303C)
#define REG_CBR        (*(volatile u16*)0x303E)

#define CLSR_10MHZ     0x00
#define CLSR_21MHZ     0x01

#define SCMR_2BPP      0x00
#define SCMR_4BPP      0x01
#define SCMR_8BPP      0x03
#define SCMR_RAN       0x08
#define SCMR_RON       0x10
#define SCMR_H128      0x00
#define SCMR_H160      0x04
#define SCMR_H192      0x20

#define GSU_SRAM_BASE  0x700000

/*============================================================================
 * Library Configuration Variables (defined in lib/source/superfx.asm)
 * Set these BEFORE calling gsuLaunch().
 *============================================================================*/

/** @brief GSU program bank byte (set by gsuSetProgram) */
extern u8 gsu_prog_bank;

/** @brief GSU program 16-bit address (set by gsuSetProgram) */
extern u16 gsu_prog_addr;

/** @brief CFGR register value ($80=IRQ mask, $A0=IRQ mask + fast multiply) */
extern u8 gsu_cfgr;

/** @brief SCMR register value ($18=RAN+RON, $19=4bpp+RAN+RON) */
extern u8 gsu_scmr;

/** @brief SCBR screen base ($00=buffer A, $10=buffer B) */
extern u8 gsu_scbr;

/** @brief DMA source high byte ($00=buffer A at $70:0000, $40=buffer B at $70:4000) */
extern u8 gsu_dma_src_hi;

/** @brief SuperFX status from crt0 init (VCR chip version, 0=not detected) */
extern u8 superfx_status;

/*============================================================================
 * API Functions
 *============================================================================*/

/**
 * @brief Initialize SuperFX — detect hardware, set default config
 * @return 1 if GSU detected, 0 if not
 *
 * Sets defaults: gsu_cfgr=$80, gsu_scmr=$19, gsu_scbr=$00, gsu_dma_src_hi=$00.
 * Inlined for zero-call-overhead access.
 */
inline u8 gsuInit(void) {
    gsu_cfgr = 0x80;        /* IRQ mask, no fast multiply */
    gsu_scmr = 0x19;        /* 4bpp + RAN + RON (most common for PLOT) */
    gsu_scbr = 0x00;        /* Buffer A */
    gsu_dma_src_hi = 0x00;  /* DMA from buffer A */
    return superfx_status != 0;
}

/**
 * @brief Launch GSU program and wait for completion (WRAM-safe)
 *
 * Call gsuSetProgram() first to set the GSU binary address.
 * Reads gsu_cfgr, gsu_scmr, gsu_scbr for configuration.
 * Disables NMI during execution (ROM inaccessible).
 */
extern void gsuLaunch(void);

/**
 * @brief Setup column-major tilemap for SuperFX PLOT framebuffer
 * @param vramAddr VRAM word address for tilemap (typically 0x4000)
 */
extern void gsuSetupBitmapTilemap(u16 vramAddr);

/**
 * @brief Scanline-polled 16KB DMA from SRAM to VRAM (60 FPS)
 *
 * Polls V-counter until scanline 184, then DMAs full framebuffer.
 * Requires gsuSetupHdmaBlanking() for sufficient DMA bandwidth.
 */
extern void gsuDmaFullFrame(void);

/**
 * @brief Setup HDMA screen blanking for DMA bandwidth
 * @param topBlank Scanlines of forced blank at top (e.g., 40)
 * @param bottomBlank Scanlines of forced blank at bottom (e.g., 40)
 *
 * Creates black bars like Star Fox. Total blank + VBlank must provide
 * enough bandwidth for the 16KB framebuffer DMA (~6.2ms needed).
 */
extern void gsuSetupHdmaBlanking(u16 topBlank, u16 bottomBlank);

#endif /* SNES_SUPERFX_H */
