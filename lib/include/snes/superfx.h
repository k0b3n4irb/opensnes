/**
 * @file superfx.h
 * @brief SuperFX (GSU) coprocessor library — registers, config, and API
 *
 * Provides a complete C API for SuperFX cartridges:
 * - GSU detection and configuration
 * - WRAM-safe GSU launch (mandatory — CPU can't read ROM during GSU execution)
 * - DMA helpers for SRAM-to-VRAM framebuffer transfer
 * - HDMA screen blanking (letterbox) that lengthens the VRAM transfer window
 * - Double-buffered presentation moved by the NMI (gsuPresent)
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

/**
 * @brief The value last written to SCMR (write-only), kept by the library
 *
 * gsuLaunch(), gsuCall(), gsuStartCached() and gsuWait() keep it. Code of
 * your own that writes REG_SCMR while gsuPresent() is in use must keep it
 * too — write it BEFORE giving the GSU the buses, clear it BEFORE taking
 * them back: the presentation NMI takes Game Pak RAM from the GSU for its
 * transfer and restores this value after.
 */
extern u8 gsu_scmr_live;

/** @brief Frames gsuPresent() has put on screen so far */
extern u16 gsu_pres_frames;

/** @brief Bytes the last transferring NMI moved (a diagnostic: the window) */
extern u16 gsu_pres_last;

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
 * Writes gsu_cfgr, the 21 MHz clock, gsu_scbr, R8 = the buffer base
 * (gsu_scbr x 1024, as gsuLaunch() does; since 2026-09-29), then gsu_scmr
 * with RON forced to 0 (the ROM stays the CPU's), then R15, which starts
 * the GSU.
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
 * @name Presenting frames: double buffer, moved by the NMI
 *
 * The GSU draws in one Game Pak RAM framebuffer while the NMI moves the
 * previous one to VRAM, a piece per VBlank, and shows it only once all of
 * it has landed. The NMI takes Game Pak RAM from the GSU for each piece
 * (Nintendo manual Book II 5.3: the GSU waits on its next RAM access and
 * resumes after); nothing polls, the CPU runs the game meanwhile.
 *
 * How much lands per VBlank is the blank window: the VBlank alone moves
 * about 4.7 KB (measured on luna), and a top letterbox from
 * gsuSetupHdmaBlanking() (with a bottom band of at least one line) adds
 * about 150 bytes per blanked line: a 16 KB frame lands in two VBlanks
 * with a 40 + 40 letterbox, in one with 84 + 4.
 * The NMI measures the window itself (V counter) at every frame.
 *
 * @code
 * gsu_scmr = SCMR_4BPP | SCMR_H128 | SCMR_RAN | SCMR_RON;
 * gsu_scbr = 0x00;                        // buffer A at $70:0000
 * gsuSetupBitmapTilemap(0x4000);
 * gsuPresentInit(0x0000, 0x2000, 0);      // two 16 KB char blocks
 * while (1) {
 *     game_logic();
 *     gsuLaunch();                        // draws in gsu_scbr's buffer
 *     gsuPresent();                       // queue it, draw the next in the other
 * }
 * @endcode
 *
 * The NMI callback gets what is left of the VBlank after the transfer:
 * it must not write VRAM while a frame is in flight.
 * @{
 */

/**
 * @brief Bytes of one framebuffer for gsu_scmr's height and depth
 * @return 32 columns x H/8 x 8*bpp bytes (16384 at 256x128x4bpp), 0 for OBJ
 *         mode or the reserved depth
 */
u16 gsuFrameBytes(void);

/**
 * @brief gsuPresentInit flag: move frames on lag frames too
 *
 * By default the NMI moves a frame only while the main thread is parked —
 * in WaitForVBlank, or waiting for a job — the rule the tilemap and OAM
 * uploads follow, because a main thread in the middle of its own VRAM
 * access would be corrupted. A game that never touches VRAM, VMAIN, VMADD,
 * DMA channel 7 or the H/V counter latch outside the NMI while a frame is
 * in flight can set this flag: the frame then moves at every VBlank, even
 * one that falls in the game's logic.
 */
#define GSU_PRESENT_ON_LAG_FRAMES 0x01

/**
 * @brief Set up presentation for gsu_scmr's geometry
 * @param vram_a VRAM word address of the char block shown first
 * @param vram_b VRAM word address of the other one
 * @param flags  0, or GSU_PRESENT_ON_LAG_FRAMES
 * @return 1, or 0 if a block is not a multiple of $1000 words, runs past
 *         VRAM, or two framebuffers from gsu_scbr on do not fit in the Game
 *         Pak RAM the header declares
 *
 * The two Game Pak RAM framebuffers start at gsu_scbr and right after it.
 * Sets BG1's char base to `vram_a`: call it with the screen off. The
 * tilemap (gsuSetupBitmapTilemap) serves both blocks.
 */
u8 gsuPresentInit(u16 vram_a, u16 vram_b, u8 flags);

/**
 * @brief Queue the frame the last job drew; the next job draws in the other
 *
 * Waits (WaitForVBlank) while the previous frame is still being moved, then
 * hands the buffer at gsu_scbr to the NMI and switches gsu_scbr to the other
 * one. Call it after the job has stopped (gsuLaunch() returned, or
 * gsuWait()).
 */
void gsuPresent(void);

/** @brief 1 while a presented frame is still being moved to VRAM */
u8 gsuPresentBusy(void);

/** @brief Wait until the last presented frame is on screen */
void gsuPresentWait(void);

/** @} */

/**
 * @brief Setup column-major tilemap for SuperFX PLOT framebuffer
 * @param vramAddr VRAM word address for tilemap (typically 0x4000)
 *
 * 32 columns of H/8 tiles, H from gsu_scmr's height (128, 160 or 192; since
 * 2026-09-29, 16 rows whatever the height before); the rest of the 32x32 map
 * points at tile 0. Set gsu_scmr first.
 */
extern void gsuSetupBitmapTilemap(u16 vramAddr);

/**
 * @brief One 16 KB DMA from Game Pak RAM to VRAM $0000, in the letterbox
 *
 * Waits for a line from which the whole frame lands before the display
 * comes back — the bottom band's first line (225 - bottom) up to
 * 152 + top — and starts the DMA there; called later than that window, it
 * waits for the next frame's. Needs gsuSetupHdmaBlanking() bands with
 * top + bottom >= 73 (40 + 40: lines 185-192). The CPU waits the whole
 * time; gsuPresent() is the variant that does not.
 *
 * Until 2026-09-29 it started at any line it read as >= 184, from a V
 * counter it never re-latched: in superfx_3d a third of the bytes landed on
 * visible lines and were dropped (luna --dma-trace).
 */
extern void gsuDmaFullFrame(void);

/**
 * @brief Setup HDMA screen blanking for DMA bandwidth
 * @param topBlank Scanlines of forced blank at top (e.g., 40)
 * @param bottomBlank Scanlines of forced blank at bottom (e.g., 40)
 *
 * Creates black bars like Star Fox, on HDMA channel 1 (INIDISP). HDMA
 * starts a channel at line 0, so the bands appear from the next frame: the
 * function returns only once they are on screen (it waits up to a frame)
 * and then publishes them to gsuDmaFullFrame() and the gsuPresent() NMI,
 * which use the blanked lines as extra transfer time. Leave the bands on
 * while either is in use.
 */
extern void gsuSetupHdmaBlanking(u16 topBlank, u16 bottomBlank);

#endif /* SNES_SUPERFX_H */
