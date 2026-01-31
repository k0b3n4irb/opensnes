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
 * ## Memory Layout (LoROM)
 *
 * SRAM is mapped at bank $70, addresses $0000-$7FFF (32KB max).
 * Most games use 2KB-8KB of SRAM.
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
 * SRAM Functions
 *============================================================================*/

/**
 * @brief Save data to SRAM
 *
 * Copies data from Work RAM to battery-backed SRAM. The data will
 * persist when the console is powered off.
 *
 * @param data Pointer to data in Work RAM to save
 * @param size Number of bytes to save (max 32KB)
 *
 * @code
 * u8 saveData[64] = { ... };
 * sramSave(saveData, 64);
 * @endcode
 *
 * @note Uses bank $70 (LoROM SRAM), starting at address $0000
 * @warning Ensure ROM header has SRAM enabled!
 */
void sramSave(u8 *data, u16 size);

/**
 * @brief Load data from SRAM
 *
 * Copies data from battery-backed SRAM to Work RAM.
 *
 * @param data Pointer to destination buffer in Work RAM
 * @param size Number of bytes to load (max 32KB)
 *
 * @code
 * u8 saveData[64];
 * sramLoad(saveData, 64);
 * @endcode
 *
 * @note If no valid save exists, SRAM contents are undefined
 */
void sramLoad(u8 *data, u16 size);

/**
 * @brief Save data to SRAM at offset
 *
 * Like sramSave() but writes to a specific offset within SRAM.
 * Useful for multiple save slots.
 *
 * @param data Pointer to data in Work RAM to save
 * @param size Number of bytes to save
 * @param offset Starting offset in SRAM (0-32767)
 *
 * @code
 * // Save slot 2 (each slot is 256 bytes)
 * sramSaveOffset(saveData, 256, 512);
 * @endcode
 */
void sramSaveOffset(u8 *data, u16 size, u16 offset);

/**
 * @brief Load data from SRAM at offset
 *
 * Like sramLoad() but reads from a specific offset within SRAM.
 * Useful for multiple save slots.
 *
 * @param data Pointer to destination buffer in Work RAM
 * @param size Number of bytes to load
 * @param offset Starting offset in SRAM (0-32767)
 *
 * @code
 * // Load slot 2 (each slot is 256 bytes)
 * sramLoadOffset(saveData, 256, 512);
 * @endcode
 */
void sramLoadOffset(u8 *data, u16 size, u16 offset);

/**
 * @brief Clear SRAM to zero
 *
 * Erases all SRAM contents by filling with zeros.
 * Useful for "delete save" functionality.
 *
 * @param size Number of bytes to clear (usually your save data size)
 *
 * @code
 * // Clear first 256 bytes (one save slot)
 * sramClear(256);
 * @endcode
 */
void sramClear(u16 size);

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
u8 sramChecksum(u8 *data, u16 size);

#endif /* OPENSNES_SRAM_H */
