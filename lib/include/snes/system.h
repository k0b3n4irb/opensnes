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
 * @note Defined in crt0.asm. Automatically set by WaitForVBlank().
 */
extern volatile u8 oam_update_flag;

/*============================================================================
 * VBlank Synchronization
 *============================================================================*/

/**
 * @brief VBlank flag (set by NMI handler each frame)
 *
 * Set to 1 by the NMI handler at the start of each VBlank.
 * WaitForVBlank() polls this flag and clears it.
 *
 * For manual VBlank timing after heavy computation, clear this flag
 * before calling WaitForVBlank() to avoid using a stale flag:
 * @code
 * vblank_flag = 0;  // Clear stale flag
 * WaitForVBlank();  // Wait for next real VBlank
 * @endcode
 *
 * @note Defined in crt0.asm.
 */
extern volatile u8 vblank_flag;

#endif /* OPENSNES_SYSTEM_H */
