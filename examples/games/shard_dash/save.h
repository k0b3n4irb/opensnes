/**
 * @file save.h
 * @brief The save: one versioned, checksummed record in SRAM
 * @ingroup examples
 */
#ifndef SHARD_DASH_SAVE_H
#define SHARD_DASH_SAVE_H

#include <snes.h>

/** @brief save_best when no round was ever won. */
#define SAVE_NONE      0xFFFF
/** @brief Bytes of the record. */
#define SAVE_SIZE      8
/** @brief Layout version: a record of another version is not read. */
#define SAVE_VERSION   1

/** @name Values of save_status
 *  @{ */
#define SAVE_UNREAD    0   /**< saveLoad() has not run */
#define SAVE_OK        1   /**< a valid record was read */
#define SAVE_FRESH     2   /**< no record (blank SRAM): the defaults */
#define SAVE_CORRUPT   3   /**< a record whose version, checksum or content is wrong: the defaults */
#define SAVE_NO_SRAM   4   /**< the cartridge declares no SRAM: the defaults, nothing is written */
/** @} */

/** @brief Best time in frames, or SAVE_NONE. */
extern u16 save_best;
/** @brief Rounds won, capped at 255. */
extern u8 save_wins;
/** @brief What saveLoad() found. */
extern u8 save_status;

/** @brief Read the record; whatever is there, the globals end up valid. */
void saveLoad(void);
/** @brief Note a won round and write the record.
 *  @return 1 if @p time beat the best. */
u8 saveRecord(u16 time);

#endif
