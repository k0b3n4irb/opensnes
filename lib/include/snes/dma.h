/**
 * @file dma.h
 * @brief SNES DMA (Direct Memory Access)
 *
 * Functions for fast memory transfers using DMA.
 *
 * ## DMA Overview
 *
 * DMA allows fast transfers between:
 * - Work RAM ↔ VRAM (video RAM)
 * - Work RAM ↔ CGRAM (palette RAM)
 * - Work RAM ↔ OAM (sprite RAM)
 *
 * DMA is much faster than CPU copying and should be used for
 * all bulk transfers during VBlank.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_DMA_H
#define OPENSNES_DMA_H

#include <snes/types.h>

/*============================================================================
 * VRAM Transfers
 *============================================================================*/

/**
 * @brief Copy data to VRAM
 *
 * @param src Source address in Work RAM
 * @param dest Destination word address in VRAM
 * @param size Number of bytes to copy
 *
 * @code
 * // Copy tile data to VRAM
 * dmaCopyToVRAM(tileData, 0x0000, sizeof(tileData));
 * @endcode
 *
 * @note Must be called during VBlank!
 */
void dmaCopyToVRAM(const void* src, u16 dest, u16 size);

/**
 * @brief Set VRAM to a value
 *
 * @param value Value to fill (repeated as word)
 * @param dest Destination word address in VRAM
 * @param size Number of bytes to fill
 */
void dmaFillVRAM(u16 value, u16 dest, u16 size);

/*============================================================================
 * CGRAM (Palette) Transfers
 *============================================================================*/

/**
 * @brief Copy palette data to CGRAM
 *
 * @param src Source address (array of 16-bit colors)
 * @param dest Starting color index (0-255)
 * @param count Number of colors to copy
 *
 * @code
 * // Copy 16-color palette to slot 0
 * dmaCopyToCGRAM(paletteData, 0, 16);
 * @endcode
 */
void dmaCopyToCGRAM(const void* src, u8 dest, u16 count);

/*============================================================================
 * OAM Transfers
 *============================================================================*/

/**
 * @brief Copy OAM data
 *
 * @param src Source address (544 bytes)
 * @param size Number of bytes to copy (usually 544)
 *
 * @code
 * dmaCopyToOAM(oamBuffer, 544);
 * @endcode
 */
void dmaCopyToOAM(const void* src, u16 size);

/*============================================================================
 * Generic DMA
 *============================================================================*/

/**
 * @brief Perform generic DMA transfer
 *
 * @param channel DMA channel (0-7)
 * @param mode DMA mode byte
 * @param srcBank Source bank
 * @param srcAddr Source address
 * @param destReg Destination B-bus register
 * @param size Transfer size
 */
void dmaTransfer(u8 channel, u8 mode, u8 srcBank, u16 srcAddr, u8 destReg, u16 size);

#endif /* OPENSNES_DMA_H */
