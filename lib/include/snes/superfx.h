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
#define REG_CFGR       (*(volatile u8*)0x3037)
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

#define CFGR_IRQ_MASK  0x80   /**< CFGR bit 7: 1 = no IRQ on STOP (the gsuInit default) */
#define CFGR_FAST_MUL  0x20   /**< CFGR bit 5: high-speed multiplier */

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

/**
 * @brief GSU IRQs on STOP acknowledged so far (crt0, wraps at 256)
 *
 * With gsu_cfgr's CFGR_IRQ_MASK clear, the GSU raises an IRQ when its
 * program executes STOP. The Super FX IRQ entry (crt0, in WRAM) reads the
 * GSU status to tell it from an H/V-timer IRQ, acknowledges it and adds one
 * here; your irqSet() handler never sees it. Compare with a copy taken before
 * the start: the count moving is the end of the job, without polling the GSU.
 * The CPU's I flag must be clear (irqEnable() clears it) for the IRQ to be
 * taken at all. (Since 2026-09-27: before, an unmasked STOP IRQ with the
 * I flag clear locked the CPU in its IRQ entry.)
 */
extern volatile u8 gsu_stop_irqs;

/**
 * @brief 1 while a GSU job owns the Game Pak ROM (crt0)
 *
 * gsuLaunch() sets it around its job. Code of your own that starts a job
 * with SCMR RON = 1 (from the RAM code window: RAM_CODE, RAM_CODE_SECTION)
 * sets it before the start and clears it once SCMR is back to 0: while it
 * is 1 the NMI and IRQ entries (crt0, in WRAM) do only the ROM-free part of
 * their work — frame_count, OAM upload, IRQ acknowledge — and defer the rest.
 */
extern u8 gsu_owns_cart;

/*============================================================================
 * API Functions
 *============================================================================*/

/**
 * @brief Is a GSU on this cartridge? (crt0 detects it; this reads the outcome)
 * @return 1 if crt0 read a non-zero chip version from the VCR, 0 if not
 *
 * The presence test gsuInit() also returns, without the side effects — use it
 * anywhere after the defaults are set. Inlined. (Added 2026-09-22; docs
 * cited a superfxIsPresent() that never existed.)
 */
inline u8 gsuIsPresent(void) {
    return superfx_status != 0;
}

/**
 * @brief Set the GSU default configuration; returns gsuIsPresent()
 * @return 1 if GSU detected, 0 if not
 *
 * Sets defaults: gsu_cfgr=$80, gsu_scmr=$19, gsu_scbr=$00, gsu_dma_src_hi=$00.
 * Detection itself is crt0's (superfx_status); call this once at boot, and
 * gsuIsPresent() when you only want the answer. Inlined.
 */
inline u8 gsuInit(void) {
    gsu_cfgr = 0x80;        /* IRQ mask, no fast multiply */
    gsu_scmr = 0x19;        /* 4bpp + RAN + RON (most common for PLOT) */
    gsu_scbr = 0x00;        /* Buffer A */
    gsu_dma_src_hi = 0x00;  /* DMA from buffer A */
    return superfx_status != 0;
}

/**
 * @brief Tell the library where the GSU program lives
 * @param program The first byte of the assembled GSU binary (the `.sfx.bin`
 *                the build `.incbin`s), in any ROM bank — the bank comes
 *                from the pointer
 *
 * Sets gsu_prog_bank and gsu_prog_addr, which gsuLaunch() reads. Call it
 * once before the first gsuLaunch(), and again only to switch programs.
 * (Until 2026-09-26 this header asked for it but only the superfx_3d example
 * defined it, in asm.)
 *
 * @code
 * extern const u8 gsu_program[];   // label before the .incbin in your .asm
 * gsuSetProgram(gsu_program);
 * @endcode
 */
void gsuSetProgram(const void *program);

/**
 * @brief Launch GSU program and wait for completion (WRAM-safe)
 *
 * Call gsuSetProgram() first to set the GSU binary address.
 * Reads gsu_cfgr, gsu_scmr, gsu_scbr for configuration.
 * The CPU waits in WRAM while the GSU owns the Game Pak. Interrupts keep
 * working (since 2026-09-25/26): the vectors point into WRAM, the NMI
 * counts the frame and uploads OAM, an H/V-timer or GSU IRQ is acknowledged;
 * the ROM-side work (your NMI callback, your IRQ handler) waits for the end
 * of the job.
 */
extern void gsuLaunch(void);

/**
 * @brief Launch the program at one of its entry points and wait (WRAM-safe)
 * @param entry Offset of the entry point in the program set by
 *              gsuSetProgram() — the build writes one per global label of
 *              the `.sfx` into `<name>.sfx.h` (`gsu_job.sfx`'s `mul_job`
 *              becomes `GSU_JOB_MUL_JOB`)
 *
 * gsuLaunch() starts at offset 0; this starts at `entry`, the rest is the
 * same (interrupts, bus ownership, the wait from the RAM code window). Pass
 * arguments in the GSU registers before the call — `REG_GSU_R1 = x;` — any
 * of R0-R7 and R9-R13: the launcher itself writes R8 (buffer base, SCBR ×
 * 1024) and R15. The program must not cross a 32 KB ROM bank.
 *
 * @code
 * #include "gsu_job.sfx.h"
 * REG_GSU_R1 = 300;
 * REG_GSU_R2 = 7;
 * gsuCall(GSU_JOB_MUL_JOB);
 * @endcode
 */
void gsuCall(u16 entry);

/**
 * @name Running a GSU job while the game keeps running
 *
 * gsuLaunch() parks the CPU in WRAM until the job ends, because a GSU that
 * runs its program from ROM owns the Game Pak ROM (SCMR RON = 1) and the
 * CPU cannot read it. A program that fits in the GSU's 512-byte code cache
 * does not need the ROM: loaded there by the CPU and started from the
 * cache, it keeps running with RON = 0, and the CPU goes on with its own
 * code in ROM (Nintendo manual Book II 6.1.2 and 6.8.4; fullsnes, SCMR).
 *
 * Constraints on such a program: at most 512 bytes; no CACHE and no LJMP
 * (both empty the cache and refetch from ROM); no ROM data reads (GETB,
 * ROM buffer). Game Pak RAM stays the GSU's while the job runs if gsu_scmr
 * grants it (RAN): the CPU must not touch $70:xxxx until gsuWait().
 *
 * @code
 * gsuCacheLoad(gsu_program, gsu_program_end - gsu_program);
 * gsuStartCached(0);              // returns at once
 * while (gsuBusy()) {
 *     game_logic();               // the CPU runs from ROM meanwhile
 * }
 * gsuWait();                      // gives Game Pak RAM back
 * @endcode
 *
 * Or be told when it ends: clear CFGR_IRQ_MASK and watch gsu_stop_irqs.
 *
 * @code
 * gsu_cfgr = 0x00;                // IRQ on STOP (irqEnable() done earlier)
 * u8 stops = gsu_stop_irqs;
 * gsuStartCached(0);
 * while (gsu_stop_irqs == stops) {
 *     game_logic();
 * }
 * gsuWait();
 * @endcode
 * @{
 */

/**
 * @brief Copy a GSU program into the GSU code cache ($3100-$32FF)
 * @param code  The assembled GSU binary (any ROM bank)
 * @param size  Its size in bytes, 1 to 512
 *
 * The GSU must be stopped. Stops it (GO = 0, which sets CBR to 0 and
 * empties the cache), copies the code, and pads the last 16-byte cache line
 * with NOP ($01) so every line it uses is written in full, as krom's cache
 * injection test does. A size above 512 is cut to 512.
 */
void gsuCacheLoad(const void *code, u16 size);

/**
 * @brief Start the program loaded by gsuCacheLoad() and return at once
 * @param pc  Entry offset in the cache (0 for the first byte; the build's
 *            `<name>.sfx.h` names every entry point)
 *
 * Writes gsu_cfgr, the 21 MHz clock, gsu_scbr, then gsu_scmr with RON
 * forced to 0 (the ROM stays the CPU's), then R15, which starts the GSU.
 */
void gsuStartCached(u16 pc);

/**
 * @brief Is the GSU still running? (SFR GO bit)
 * @return 1 while the job runs, 0 once it has executed STOP
 */
u8 gsuBusy(void);

/**
 * @brief Wait for the end of the job, then give the Game Pak back to the CPU
 *
 * Returns at once if the GSU has already stopped. Clears SCMR, so the CPU
 * can read the results the program left in Game Pak RAM.
 */
void gsuWait(void);

/** @} */

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
