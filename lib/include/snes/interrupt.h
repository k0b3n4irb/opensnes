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
 * The registered callback will be called during every VBlank interrupt,
 * BEFORE the vblank_flag is set. This allows time-critical operations
 * (like DMA transfers) to be performed reliably during VBlank.
 *
 * @param callback Function to call during VBlank, or NULL to disable
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
 * @note Keep callbacks short! VBlank time is limited (~2200 scanlines on NTSC)
 * @note Callback runs with interrupts disabled
 * @note The callback function must be in the same ROM bank as the main code (bank 0).
 *       For larger projects, use nmiSetBank() to specify the bank explicitly.
 */
void nmiSet(VBlankCallback callback);

/**
 * @brief Register a VBlank callback with explicit bank
 *
 * Use this when the callback function might not be in bank 0.
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

#endif /* OPENSNES_INTERRUPT_H */
