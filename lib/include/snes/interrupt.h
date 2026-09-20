/**
 * @file interrupt.h
 * @brief SNES Interrupt Handling
 *
 * Manages NMI (VBlank), IRQ, and other interrupts.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_INTERRUPT_H
#define OPENSNES_INTERRUPT_H

#include <snes/types.h>

/*============================================================================
 * VBlank Callback
 *============================================================================*/

/**
 * @brief VBlank callback function pointer type
 *
 * Functions of this type can be registered with nmiSet() to be called
 * automatically during every VBlank interrupt.
 */
typedef void (*VBlankCallback)(void);

/**
 * @brief Register a VBlank callback function
 *
 * The callback runs inside the NMI handler, once per frame the main thread
 * was waiting for — NOT on lag frames (an NMI that arrives while the main
 * loop is still computing skips all VBlank work, this callback included). It
 * runs AFTER the library's own VBlank work: the OAM upload, the text tilemap
 * flush and the background scroll sync have already used part of VBlank.
 *
 * @param callback Function to call during VBlank, or NULL to disable (same as
 *                 nmiClear())
 *
 * @warning Inside the callback: never call WaitForVBlank() (it waits for an
 *          NMI that cannot arrive — deadlock); never touch the WRAM data port
 *          $2180-$2183; never use a lib DMA helper while the main thread may
 *          be setting one up (they all share DMA channel 0). Plain C
 *          multiply / divide are safe (the runtime avoids the hardware unit
 *          in NMI context); fixMul and fix32Mul are not.
 *
 * @code
 * void myVBlankHandler(void) {
 *     // DMA transfer, scroll updates, etc.
 * }
 *
 * int main(void) {
 *     consoleInit();
 *     nmiSet(myVBlankHandler);
 *     while (1) {
 *         WaitForVBlank();
 *         // Game logic here
 *     }
 * }
 * @endcode
 *
 * @note Keep callbacks short: VBlank is short and the library has already
 *       spent part of it. `make test-nmi-budget` measures the handler.
 * @note Callback runs with interrupts disabled
 * @note The callback may live in any ROM bank: the bank is taken from the
 *       function pointer. (This note said "must be in bank 0" until
 *       2026-09-20; that was only ever true of irqSet, fixed the same day.)
 */
void nmiSet(VBlankCallback callback);

/**
 * @brief Register a VBlank callback with explicit bank
 *
 * Not needed from C — nmiSet() reads the bank from the pointer. Kept for
 * callers that only have a 16-bit address (assembly).
 *
 * @param callback Function to call during VBlank
 * @param bank ROM bank where the callback is located (0-255)
 */
void nmiSetBank(VBlankCallback callback, u8 bank);

/**
 * @brief Clear the VBlank callback
 *
 * Equivalent to nmiSet(NULL).
 */
void nmiClear(void);

/*============================================================================
 * Hardware IRQ (H/V timer)
 *============================================================================*/

/** @brief Fire an IRQ when the H counter reaches the HTIME value (every scanline). */
#define IRQ_HTIMER 0x10
/** @brief Fire an IRQ when the V counter reaches the VTIME value (once per frame). */
#define IRQ_VTIMER 0x20

/**
 * @brief Install a raw hardware IRQ handler (H/V timer interrupts)
 *
 * The crt0 IRQ vector does a single `JML [irq_callback]` into your handler —
 * nothing is saved, acknowledged, or set up for you. This is deliberate:
 * an H-timer IRQ fires EVERY SCANLINE (~15.7 kHz) and cannot afford a C
 * prologue, so the SDK imposes zero overhead and zero policy.
 *
 * @warning ASM handlers only. The handler contract:
 *   - Save and restore every register it touches (A/X/Y, DP, DBR — the
 *     interrupted code is arbitrary C). P is restored by RTI automatically.
 *   - Read REG_TIMEUP ($4211) to acknowledge the IRQ, or it re-fires forever.
 *   - Return with `rti`.
 *   - After any `rep`/`sep`, add explicit `.ACCU`/`.INDEX` directives
 *     (WLA-DX tracking rule).
 *   A C function pointer passed here WILL corrupt the main thread —
 *   cc65816 C prologues assume the tcc register file, which the raw
 *   vector does not bank-switch (unlike the NMI path's DP isolation).
 *
 * @warning A handler that fires general DMA owns that channel: don't call
 *   lib DMA helpers (dmaCopyVram etc.) from the main loop while such a
 *   handler is armed — the channel registers are shared state.
 *
 * @param handler Address of the ASM handler (see examples/graphics/effects/
 *                hicolor_1792/irq_stream.asm for the canonical shape)
 */
void irqSet(void *handler);

/**
 * @brief irqSet() with an explicit ROM bank for the handler
 * @param handler Address of the ASM handler
 * @param bank ROM bank containing the handler
 */
void irqSetBank(void *handler, u8 bank);

/**
 * @brief Restore the default IRQ handler (acknowledge + return)
 */
void irqClear(void);

/**
 * @brief Set the H-timer target (0-339 dots)
 *
 * With IRQ_HTIMER enabled, the IRQ fires at this horizontal position on
 * every scanline. Values near the end of the visible line (e.g. 190) let
 * a short handler's PPU writes land in H-blank.
 */
void irqSetHTimer(u16 h);

/**
 * @brief Set the V-timer target (0-261 scanlines NTSC)
 *
 * With IRQ_VTIMER enabled, the IRQ fires once per frame at this scanline
 * (combined with IRQ_HTIMER: at that exact H/V position).
 */
void irqSetVTimer(u16 v);

/**
 * @brief Enable H/V timer IRQs and unmask interrupts (CLI)
 *
 * Sets the requested timer bits in the NMITIMEN shadow (NMI and auto-joypad
 * bits are preserved) and clears the CPU I flag. Install the handler with
 * irqSet() and program the timer with irqSetHTimer()/irqSetVTimer() BEFORE
 * calling this.
 *
 * @param flags IRQ_HTIMER, IRQ_VTIMER, or both
 */
void irqEnable(u8 flags);

/**
 * @brief Disable H/V timer IRQs
 *
 * Clears both timer bits in the NMITIMEN shadow. The I flag is left clear —
 * with no timer source enabled, no IRQ can fire.
 */
void irqDisable(void);

#endif /* OPENSNES_INTERRUPT_H */
