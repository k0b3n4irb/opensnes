/**
 * @file system.h
 * @brief SNES System Variables (crt0.asm exports)
 *
 * Declares variables defined in crt0.asm that are commonly needed
 * by game code. Including <snes.h> automatically includes this header.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_SYSTEM_H
#define OPENSNES_SYSTEM_H

#include <snes/types.h>

/*============================================================================
 * OAM (Sprite) Buffer
 *============================================================================*/

/**
 * @brief Hardware OAM buffer (544 bytes at $7E:0300)
 *
 * This buffer is DMA'd to OAM hardware during VBlank by the NMI handler
 * when oam_update_flag is set.
 *
 * Layout:
 * - Bytes 0-511: 4 bytes per sprite (128 sprites)
 *   - offset+0: X position (low 8 bits)
 *   - offset+1: Y position
 *   - offset+2: Tile number (low 8 bits)
 *   - offset+3: Attributes (vhoopppc)
 * - Bytes 512-543: 2 bits per sprite (X high bit, size select)
 *
 * @note Defined in crt0.asm. Used by oamSetFast()/oamSetXYFast() macros.
 */
extern u8 oamMemory[];

/**
 * @brief OAM DMA trigger flag
 *
 * Set to 1 to request OAM buffer DMA during the next VBlank.
 * The NMI handler clears this after the transfer.
 *
 * @note Defined in crt0.asm. WaitForVBlank() does NOT set it: every
 *       OAM-mutating function of sprite.h (and the oamSetFast macros) sets it
 *       itself, and code that writes oamMemory[] directly must do the same —
 *       otherwise the change never reaches the PPU.
 */
extern volatile u8 oam_update_flag;

/*============================================================================
 * VBlank Synchronization
 *============================================================================*/

/**
 * @brief Main-thread / NMI handshake flag — internal, do not write
 *
 * WaitForVBlank() sets it to 1 ("the main thread is ready") and sleeps; the
 * NMI handler does its VBlank work only when it finds 1, then clears it to 0,
 * which is what wakes WaitForVBlank(). An NMI that finds 0 is a lag frame: it
 * counts it and skips all VBlank work, the user callback included. There is
 * no stale flag to clear — this text described the opposite protocol until
 * 2026-09-20. Writing the flag from user code breaks the handshake.
 *
 * @note Defined in crt0.asm.
 */
extern volatile u8 vblank_flag;

/**
 * @brief Frame counter incremented every VBlank by the NMI handler
 *
 * Counts total frames since boot (including lag frames).
 * Useful for timing, animation, and elapsed-time calculations.
 *
 * @note Defined in crt0.asm.
 */
extern volatile u16 frame_count;

#endif /* OPENSNES_SYSTEM_H */
