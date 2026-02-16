/**
 * @file scoring.h
 * @brief Score management for OpenSNES
 *
 * Provides a simple scoring system using a two-part representation
 * (low/high u16 pairs) to handle scores beyond 16-bit range.
 * Each part represents a value 0-9999, giving a total range of 0-99,999,999.
 *
 * ## Attribution
 *
 * Originally from: PVSnesLib (https://github.com/alekmaul/pvsneslib)
 * Author: Alekmaul
 * License: zlib (compatible with MIT)
 * Modifications:
 *   - Pure C implementation (PVSnesLib used assembly)
 *   - Renamed for clarity
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_SCORING_H
#define OPENSNES_SCORING_H

#include <snes/types.h>

/**
 * @brief Score memory structure
 *
 * Stores a score as two 16-bit values, each representing 0-9999.
 * The full score = scohi * 10000 + scolo.
 */
typedef struct {
    u16 scolo;  /**< Low part of score (0-9999) */
    u16 scohi;  /**< High part of score (0-9999) */
} scoMemory;

/**
 * @brief Clear a score to zero
 *
 * @param score Pointer to score to clear
 */
void scoreClear(scoMemory *score);

/**
 * @brief Add a value to a score
 *
 * Adds value to the low part. If it overflows 9999 (0x2710),
 * the excess carries into the high part.
 *
 * @param score Pointer to score to modify
 * @param value Value to add (0-9999)
 */
void scoreAdd(scoMemory *score, u16 value);

/**
 * @brief Copy a score
 *
 * @param src Source score
 * @param dst Destination score
 */
void scoreCpy(scoMemory *src, scoMemory *dst);

/**
 * @brief Compare two scores
 *
 * @param a First score
 * @param b Second score
 * @return 0 if equal, 0xFF if a > b, 1 if a < b
 */
u8 scoreCmp(scoMemory *a, scoMemory *b);

#endif /* OPENSNES_SCORING_H */
