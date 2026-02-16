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
 * ## VBlank Timing Requirement
 *
 * @warning **CRITICAL**: VRAM, CGRAM, and OAM can only be safely accessed
 * during VBlank (vertical blanking period) or when the screen is in force blank
 * mode (INIDISP bit 7 set). Accessing these memories during active display
 * causes visual corruption and undefined behavior.
 *
 * @warning **VBlank Budget**: You have approximately 2,200 CPU cycles during
 * VBlank for DMA transfers. This translates to roughly **4KB of data** that
 * can be safely transferred per frame. While the theoretical maximum is higher,
 * 4KB is a safe practical limit that accounts for NMI handler overhead.
 *
 * ## Bank Limitation
 *
 * @note The current implementation assumes source data is in bank $00 for
 * ROM addresses. Data in SUPERFREE sections beyond bank 0 requires using
 * dmaCopyVramBank() with an explicit bank parameter.
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
 * WaitForVBlank();
 * dmaCopyVram(tileset, 0x0000, tileset_end - tileset);
 * @endcode
 *
 * @warning Must be called during VBlank or force blank! Calling during
 * active display causes visual corruption.
 *
 * @note Source data must be in bank $00 (LoROM $00:8000-$FFFF). For data
 * in other banks, use dmaCopyVramBank().
 */
void dmaCopyVram(u8 *source, u16 vramAddr, u16 size);

/**
 * @brief Copy data to VRAM with explicit source bank byte.
 *
 * Same as dmaCopyVram() but allows specifying the ROM bank for data
 * in banks other than $00 (e.g., SUPERFREE sections placed by linker).
 *
 * @param source   Source address (16-bit offset within bank)
 * @param bank     Source bank byte ($00-$3F for LoROM, $00-$7D for HiROM)
 * @param vramAddr VRAM destination word address
 * @param size     Number of bytes to transfer
 *
 * @warning Must be called during VBlank or force blank!
 */
void dmaCopyVramBank(u8 *source, u8 bank, u16 vramAddr, u16 size);

/**
 * @brief Load Mode 7 interleaved data to VRAM.
 *
 * Mode 7 stores tilemap in VRAM low bytes and tile pixels in VRAM high bytes.
 * This function performs two DMA transfers with appropriate VMAIN settings:
 *   1. Tilemap -> VMDATAL (VMAIN=$00, increment after low byte)
 *   2. Tiles   -> VMDATAH (VMAIN=$80, increment after high byte)
 *
 * VRAM destination is always $0000 (Mode 7 uses the full 32K word space).
 *
 * @param tilemap      Tilemap data source (.mp7 file, 128x128 = 16384 bytes)
 * @param tilemapSize  Tilemap data size in bytes
 * @param tiles        Tile pixel data source (.pc7 file, 256 tiles x 64 bytes)
 * @param tilesSize    Tile pixel data size in bytes
 *
 * @warning Must be called during forced blank (INIDISP=$80) or VBlank.
 */
void dmaCopyVramMode7(u8 *tilemap, u16 tilemapSize, u8 *tiles, u16 tilesSize);

/**
 * @brief Copy data to VRAM (WRAM source only)
 *
 * @param src Source address in Work RAM (bank $7E)
 * @param dest Destination word address in VRAM
 * @param size Number of bytes to copy
 *
 * @note Must be called during VBlank!
 * @deprecated Use dmaCopyVram() instead for ROM/RAM sources
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
 * WaitForVBlank();
 * dmaCopyCGram(palette, 0, 32);  // 16 colors * 2 bytes
 * @endcode
 *
 * @warning Must be called during VBlank or force blank! Calling during
 * active display causes palette corruption.
 *
 * @note Source data must be in bank $00. For data in other banks,
 * manual DMA setup with explicit bank is required.
 */
void dmaCopyCGram(u8 *source, u16 startColor, u16 size);

/**
 * @brief Copy palette data to CGRAM (WRAM source only)
 *
 * @param src Source address (array of 16-bit colors)
 * @param dest Starting color index (0-255)
 * @param count Number of colors to copy
 *
 * @deprecated Use dmaCopyCGram() instead for ROM/RAM sources
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
 * @deprecated Use dmaCopyOam() instead for ROM/RAM sources
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
