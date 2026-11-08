/**
 * @file save.c
 * @brief The save: magic, version, checksum, and defaults when anything is off
 * @ingroup examples
 *
 * Record (8 bytes): 'S' 'D' | version | best (u16, little-endian) | wins |
 * 0 | checksum of the seven bytes before. The record is flat bytes, never a
 * struct dumped as is: no padding or endianness to depend on, and a layout
 * change is a version bump.
 */
#include <snes.h>
#include <snes/sram.h>
#include <snes/debug.h>
#include "game.h"
#include "save.h"

u16 save_best;
u8 save_wins;
u8 save_status;

/** @brief The record as it sits in RAM. */
u8 save_rec[SAVE_SIZE];

/** @brief The values a fresh game starts with. */
static void defaults(void) {
    save_best = SAVE_NONE;
    save_wins = 0;
}

void saveLoad(void) {
    u16 best;
    defaults();
    if (sramLoad(save_rec, SAVE_SIZE) != SRAM_OK) {
        save_status = SAVE_NO_SRAM;
        return;
    }
    if (save_rec[0] != 'S' || save_rec[1] != 'D') {
        save_status = SAVE_FRESH;       /* blank cartridge: not an error */
        return;
    }
    best = (u16)(save_rec[3] | (save_rec[4] << 8));
    if (save_rec[2] != SAVE_VERSION
        || save_rec[7] != sramChecksum(save_rec, SAVE_SIZE - 1)
        || (best != SAVE_NONE && (best == 0 || best > TIME_LIMIT))) {
        save_status = SAVE_CORRUPT;     /* never trust it, never crash on it */
        return;
    }
    save_best = best;
    save_wins = save_rec[5];
    save_status = SAVE_OK;
}

u8 saveRecord(u16 time) {
    u8 better = 0;
    SNES_ASSERT(time > 0 && time <= TIME_LIMIT);    /* a win is inside the clock */
    if (save_wins < 255) save_wins++;
    if (time < save_best) {
        save_best = time;
        better = 1;
    }
    save_rec[0] = 'S';
    save_rec[1] = 'D';
    save_rec[2] = SAVE_VERSION;
    save_rec[3] = (u8)(save_best & 0xFF);
    save_rec[4] = (u8)(save_best >> 8);
    save_rec[5] = save_wins;
    save_rec[6] = 0;
    save_rec[7] = sramChecksum(save_rec, SAVE_SIZE - 1);
    sramSave(save_rec, SAVE_SIZE);
    return better;
}
