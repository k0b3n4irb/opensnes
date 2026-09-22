/**
 * @file sram.h
 * @brief SNES SRAM (Save RAM) Functions
 *
 * Functions for saving and loading game data to battery-backed SRAM.
 *
 * ## SRAM Overview
 *
 * SRAM (Static RAM) on SNES cartridges is battery-backed RAM that
 * persists when the console is powered off. It's used for save games.
 *
 * ## Memory Layout
 *
 * - **LoROM**: bank $70, $0000-$7FFF (32 KB max).
 * - **HiROM** (`USE_HIROM=1`): bank $30, $6000-$7FFF — the hardware exposes
 *   battery RAM in 8 KB windows there, and this module addresses the first
 *   one only, so `offset + size` must stay within 8 KB (the default
 *   SRAM_SIZE). Until 2026-09-20 the LoROM address was used on HiROM too.
 * - **SA-1**: not supported — `USE_SRAM=1` with `USE_SA1=1` is a build error.
 *   SA-1 save memory is BW-RAM, which the SNES CPU may only write after
 *   enabling it, and the library does not.
 *
 * Source pointers may be in any bank (a `const` save template in ROM works);
 * destination pointers are work RAM. Most games use 2 KB-8 KB of SRAM.
 *
 * ## Usage Example
 *
 * @code
 * // Define your save data structure
 * typedef struct {
 *     u8  magic[4];      // "SAVE" to detect valid save
 *     u16 score;
 *     u8  level;
 *     u8  lives;
 *     u8  checksum;
 * } SaveData;
 *
 * SaveData save;
 *
 * // Load save data at game start
 * sramLoad((u8*)&save, sizeof(SaveData));
 * if (save.magic[0] != 'S' || save.magic[1] != 'A') {
 *     // No valid save, initialize defaults
 *     save.score = 0;
 *     save.level = 1;
 *     save.lives = 3;
 * }
 *
 * // Save when needed (e.g., at checkpoint)
 * save.magic[0] = 'S'; save.magic[1] = 'A';
 * save.magic[2] = 'V'; save.magic[3] = 'E';
 * sramSave((u8*)&save, sizeof(SaveData));
 * @endcode
 *
 * ## ROM Header Requirement
 *
 * To enable SRAM, your ROM header (hdr.asm) must set:
 * - CARTRIDGETYPE $02 (ROM + SRAM)
 * - SRAMSIZE $03 (8KB) or appropriate size
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_SRAM_H
#define OPENSNES_SRAM_H

#include <snes/types.h>

/*============================================================================
 * SRAM Size Constants
 *============================================================================*/

/** @brief No SRAM (0KB) */
#define SRAM_SIZE_NONE    0x00

/** @brief 2KB SRAM (16Kbit) */
#define SRAM_SIZE_2KB     0x01

/** @brief 4KB SRAM (32Kbit) */
#define SRAM_SIZE_4KB     0x02

/** @brief 8KB SRAM (64Kbit) - Most common */
#define SRAM_SIZE_8KB     0x03

/** @brief 16KB SRAM (128Kbit) */
#define SRAM_SIZE_16KB    0x04

/** @brief 32KB SRAM (256Kbit) */
#define SRAM_SIZE_32KB    0x05

/*============================================================================
 * Return Codes
 *============================================================================*/

/**
 * @name SRAM return codes
 * Returned by sramSave(), sramLoad(), sramSaveOffset(), sramLoadOffset() and
 * sramClear(). The family returned nothing until 2026-09-21 and copied
 * whatever it was asked; a refused transfer now copies NOTHING.
 *
 * The capacity checked against is the one your ROM declares: the SRAMSIZE
 * byte of its header (`SRAM_SIZE` in your Makefile, 8 KB by default), capped
 * by what this module addresses in one bank (32 KB on LoROM, 8 KB on HiROM).
 * @{
 */
/** @brief The transfer fits and was done (a `size` of 0 is also OK). */
#define SRAM_OK           0
/** @brief `offset + size` exceeds the declared SRAM; nothing was copied. */
#define SRAM_ERR_RANGE    1
/** @brief The ROM header declares no SRAM (`USE_SRAM := 1` is missing). */
#define SRAM_ERR_NO_SRAM  2
/** @} */

/*============================================================================
 * SRAM Functions
 *============================================================================*/

/**
 * @brief Save data to SRAM
 *
 * Copies data from Work RAM to battery-backed SRAM. The data will
 * persist when the console is powered off.
 *
 * @param data Pointer to data in Work RAM to save
 * @param size Number of bytes to save
 * @return SRAM_OK, or SRAM_ERR_RANGE / SRAM_ERR_NO_SRAM (nothing written)
 *
 * @code
 * u8 saveData[64] = { ... };
 * sramSave(saveData, 64);
 * @endcode
 *
 * @note Writes from SRAM offset 0 — $70:0000 on LoROM, $30:6000 on HiROM
 * @warning The ROM header must declare SRAM (`USE_SRAM := 1`); without it
 *          this returns SRAM_ERR_NO_SRAM.
 */
u8 sramSave(const u8 *data, u16 size);

/**
 * @brief Load data from SRAM
 *
 * Copies data from battery-backed SRAM to Work RAM.
 *
 * @param data Pointer to destination buffer in Work RAM — a bank-0 object
 *             or a `FAR` (bank $7E) one; the copy runs as a block move into
 *             bank $7E, whose first 8 KB mirror bank 0
 * @param size Number of bytes to load
 * @return SRAM_OK, or SRAM_ERR_RANGE / SRAM_ERR_NO_SRAM (`data` untouched)
 *
 * @code
 * u8 saveData[64];
 * sramLoad(saveData, 64);
 * @endcode
 *
 * @note If no valid save exists, SRAM contents are undefined
 */
u8 sramLoad(u8 FAR *data, u16 size);

/**
 * @brief Save data to SRAM at offset
 *
 * Like sramSave() but writes to a specific offset within SRAM.
 * Useful for multiple save slots.
 *
 * @param data Pointer to data in Work RAM to save
 * @param size Number of bytes to save
 * @param offset Starting offset in SRAM
 * @return SRAM_OK, or SRAM_ERR_RANGE / SRAM_ERR_NO_SRAM (nothing written)
 *
 * @code
 * // Save slot 2 (each slot is 256 bytes)
 * sramSaveOffset(saveData, 256, 512);
 * @endcode
 */
u8 sramSaveOffset(const u8 *data, u16 size, u16 offset);

/**
 * @brief Load data from SRAM at offset
 *
 * Like sramLoad() but reads from a specific offset within SRAM.
 * Useful for multiple save slots.
 *
 * @param data Pointer to destination buffer in Work RAM
 * @param size Number of bytes to load
 * @param offset Starting offset in SRAM
 * @return SRAM_OK, or SRAM_ERR_RANGE / SRAM_ERR_NO_SRAM (`data` untouched)
 *
 * @code
 * // Load slot 2 (each slot is 256 bytes)
 * sramLoadOffset(saveData, 256, 512);
 * @endcode
 */
u8 sramLoadOffset(u8 FAR *data, u16 size, u16 offset);

/**
 * @brief Clear SRAM to zero
 *
 * Erases all SRAM contents by filling with zeros.
 * Useful for "delete save" functionality.
 *
 * @param size Number of bytes to clear (usually your save data size)
 * @return SRAM_OK, or SRAM_ERR_RANGE / SRAM_ERR_NO_SRAM (nothing cleared)
 *
 * @code
 * // Clear first 256 bytes (one save slot)
 * sramClear(256);
 * @endcode
 */
u8 sramClear(u16 size);

/**
 * @brief Calculate simple checksum
 *
 * Calculates a simple 8-bit checksum of data. Use this to verify
 * save data integrity.
 *
 * @param data Pointer to data
 * @param size Number of bytes
 * @return 8-bit checksum (XOR of all bytes)
 *
 * @code
 * // Before saving:
 * save.checksum = 0;
 * save.checksum = sramChecksum((u8*)&save, sizeof(save));
 * sramSave((u8*)&save, sizeof(save));
 *
 * // After loading:
 * u8 storedChecksum = save.checksum;
 * save.checksum = 0;
 * if (sramChecksum((u8*)&save, sizeof(save)) != storedChecksum) {
 *     // Save data corrupted!
 * }
 * @endcode
 */
u8 sramChecksum(const u8 *data, u16 size);

#endif /* OPENSNES_SRAM_H */
