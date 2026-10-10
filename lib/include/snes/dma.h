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
 * @warning **VBlank Budget**: VBlank lasts about 49 000 master clocks
 * (37 lines of 1 324 on NTSC) and a DMA moves one byte per 8, so the whole
 * blank would carry ~6 KB; the NMI handler takes its share first, which
 * leaves roughly **4KB of data** per frame as the safe practical limit.
 * (Until 2026-10-04 this note said "2,200 CPU cycles", which is neither
 * the blank nor the budget.)
 *
 * ## Banks
 *
 * @note Every dmaCopy* function takes a **far pointer** and uses its bank
 * byte, so the source may live in any bank — a SUPERFREE section, a bank
 * you pinned yourself, anywhere. This has been true since the A6 chantier
 * made pointer arguments 4 bytes; the *Bank variants remain for the case
 * where an address and its bank are held separately.
 *
 * This header claimed the opposite ("source data must be in bank $00")
 * long after it stopped being true, and that prose is why the RPG
 * template shipped a pointless `semifree bank 0` section for its
 * palettes — and why issue #122 carried a bank-$00 claim about
 * dmaCopyCGram that testing then had to retract.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_DMA_H
#define OPENSNES_DMA_H

#include <snes/types.h>

/* Removed on 2026-10-05 (1.0 plan, lot C): dmaCopyVramBank, dmaCopyCGramBank.
 * The replacements are in docs/UPGRADING.md; `make check-upgrade` names them. */

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
 * @warning Must be called during VBlank or forced blank. During active
 *          display the PPU rejects VRAM writes and the transfer is
 *          silently lost.
 *
 *          Forced blank means setScreenOff() — INIDISP bit 7.
 *          setBrightness(0) blacks the screen but leaves the PPU
 *          fetching, so it does NOT open the write window.
 *
 * @warning **VBlank fits roughly 4 KB.** A larger transfer has its tail
 *          dropped during active display, and the failure is partial:
 *          the first few KB land, so the screen is half right. Anything
 *          bigger than a few KB belongs between setScreenOff() and
 *          setScreenOn(), not in VBlank.
 *
 * @note The bank is taken from @p source's own bank byte, so the data
 *       may live in ANY bank — a SUPERFREE section, a bank you pinned
 *       yourself.
 * @see @ref perf "Measured frame costs" — what this call costs per frame in five real scenes.
 */
void dmaCopyVram(const u8 *source, u16 vramAddr, u16 size);

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
void dmaCopyVramMode7(const u8 *tilemap, u16 tilemapSize, const u8 *tiles, u16 tilesSize);

/**
 * @brief Set VRAM to a value
 *
 * @param value 16-bit word to fill with (low byte to the even VRAM byte, high
 *              byte to the odd one). Until 2026-09-20 only the low byte was
 *              used, for both halves.
 * @param dest Destination word address in VRAM
 * @param size Number of bytes to fill, rounded up to a whole word — 0 means
 *             65536 (the full VRAM), which is how dmaClearVRAM() uses it
 *
 * @note Uses DMA channel 0 with a fixed source address, like the other
 * lib DMA helpers (oamUpdate, dmaCopyVram). Safe when calls are
 * sequential from the main loop; do NOT call from an NMI callback while
 * a main-thread DMA setup on channel 0 may be in flight.
 * @warning Must be called during forced blank (INIDISP=$80) or VBlank —
 * the PPU silently ignores VRAM writes during active display.
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
 * @warning Must be called during VBlank or forced blank — setScreenOff(),
 *          INIDISP bit 7. setBrightness(0) blacks the screen but leaves
 *          the PPU fetching and the write is rejected.
 *
 * @note The bank comes from @p source's own bank byte: a palette may
 *       live in any bank.
 */
void dmaCopyCGram(const u8 *source, u16 startColor, u16 size);

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
void dmaCopyOam(const u8 *source, u16 size);

/*============================================================================
 * Generic DMA
 *============================================================================*/

/**
 * @brief Perform generic DMA transfer
 *
 * Programs one channel and starts it at once: use it when no named helper
 * fits (the WRAM data port, an experimental mode). The source's bank comes
 * from the far pointer, like every other `dmaCopy*` call — until 0.48 this
 * function took the bank and the 16-bit address as two arguments (the 1.0
 * API since 2026-10-05; a six-argument call no longer compiles, see
 * docs/UPGRADING.md). A channel above 7 is refused.
 *
 * @param channel DMA channel (0-7)
 * @param mode DMA mode byte (DMAP: transfer pattern, direction, fixed source)
 * @param src Source, in any bank (ROM, bank $7E RAM, FAR data)
 * @param destReg Destination B-bus register (the low byte of $21xx)
 * @param size Transfer size in bytes (0 = 65536)
 *
 * @code
 * REG_CGADD = 254;
 * dmaTransfer(1, 0x00, two_colours, 0x22, 4);   // 2 colours to CGDATA
 * @endcode
 */
void dmaTransfer(u8 channel, u8 mode, const u8 *src, u8 destReg, u16 size);

#endif /* OPENSNES_DMA_H */
