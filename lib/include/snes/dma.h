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
 * @brief Copy data to VRAM (PVSnesLib compatible)
 *
 * Copies data from ROM or RAM to VRAM using DMA. This function handles
 * 24-bit source addresses, so it works with ROM data defined via extern.
 *
 * @param source Source address (can be ROM or RAM)
 * @param vramAddr Destination word address in VRAM
 * @param size Number of bytes to copy
 *
 * @code
 * extern char tileset[];
 * extern char tileset_end[];
 * dmaCopyVram(tileset, 0x0000, tileset_end - tileset);
 * @endcode
 *
 * @note Must be called during VBlank or force blank!
 */
void dmaCopyVram(u8 *source, u16 vramAddr, u16 size);

/**
 * @brief Copy data to VRAM (WRAM source only)
 *
 * @param src Source address in Work RAM (bank $7E)
 * @param dest Destination word address in VRAM
 * @param size Number of bytes to copy
 *
 * @note Must be called during VBlank!
 * @deprecated Use dmaCopyVram() instead
 */
void dmaCopyToVRAM(const void* src, u16 dest, u16 size);

/**
 * @brief Set VRAM to a value
 *
 * @param value Value to fill (repeated as word)
 * @param dest Destination word address in VRAM
 * @param size Number of bytes to fill (0 = 65536)
 */
void dmaFillVRAM(u16 value, u16 dest, u16 size);

/**
 * @brief Clear all VRAM to zero
 *
 * Clears all 64KB of VRAM. Should be called during force blank.
 */
void dmaClearVRAM(void);

/*============================================================================
 * CGRAM (Palette) Transfers
 *============================================================================*/

/**
 * @brief Copy palette data to CGRAM (PVSnesLib compatible)
 *
 * Copies palette data from ROM or RAM to CGRAM using DMA.
 *
 * @param source Source address (can be ROM or RAM)
 * @param startColor Starting color index (0-255)
 * @param size Number of bytes to copy (2 bytes per color)
 *
 * @code
 * extern char palette[];
 * dmaCopyCGram(palette, 0, 32);  // 16 colors * 2 bytes
 * @endcode
 *
 * @note Must be called during VBlank or force blank!
 */
void dmaCopyCGram(u8 *source, u16 startColor, u16 size);

/**
 * @brief Copy palette data to CGRAM (WRAM source only)
 *
 * @param src Source address (array of 16-bit colors)
 * @param dest Starting color index (0-255)
 * @param count Number of colors to copy
 *
 * @deprecated Use dmaCopyCGram() instead
 */
void dmaCopyToCGRAM(const void* src, u8 dest, u16 count);

/*============================================================================
 * OAM Transfers
 *============================================================================*/

/**
 * @brief Copy OAM data (PVSnesLib compatible)
 *
 * @param source Source address (544 bytes)
 * @param size Number of bytes to copy (usually 544)
 *
 * @code
 * dmaCopyOam(oamBuffer, 544);
 * @endcode
 */
void dmaCopyOam(u8 *source, u16 size);

/**
 * @brief Copy OAM data (WRAM source only)
 *
 * @param src Source address (544 bytes)
 * @param size Number of bytes to copy (usually 544)
 *
 * @deprecated Use dmaCopyOam() instead
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
