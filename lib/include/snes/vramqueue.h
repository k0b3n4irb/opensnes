/**
 * @file vramqueue.h
 * @brief A queue of VRAM uploads, filled during the frame and sent in VBlank
 *
 * A game that streams graphics during play — sprite frames, the rows and
 * columns of a scrolling map — makes several small VRAM transfers in each
 * VBlank. One dmaCopyVram() call per transfer costs about as much to set up
 * from C as it does to run for 128 bytes, and the VBlank is short. Here a
 * transfer is noted while the frame is computed, when time is not short,
 * and all of them are sent in VBlank by one assembly routine:
 *
 * @code
 * #include <snes/vramqueue.h>          // and `vramqueue` in LIB_MODULES
 *
 * // during the frame, wherever the game decides something must reach VRAM
 * if (vramQueueFree() >= 4) {
 *     vramQueuePush(frame_tiles,        0x0000, 128, VRAM_QUEUE_ROW);
 *     vramQueuePush(frame_tiles + 512,  0x0100, 128, VRAM_QUEUE_ROW);
 *     // ...
 * }
 *
 * WaitForVBlank();
 * vramQueueFlush();                    // first thing in VBlank
 * @endcode
 *
 * **What it buys is VBlank time, not time.** Noting an entry is a call
 * too, so in total the queue costs more than the dmaCopyVram() calls it
 * replaces: six 128-byte transfers measure 21,700 master cycles queued
 * against 16,600 called directly. But only 10,700 of the 21,700 fall in the
 * VBlank — 8 per byte plus about 770 per entry — where the six calls put
 * all 16,600 (`devtools/libbench`, rows `vramc` and `vramq`).
 *
 * **The budget is yours to keep.** A VBlank is about 49,000 master cycles
 * and the NMI handler takes the first 5,000 to 8,000 of them. A real project measured six 512-byte sprite frames as the most
 * that fit, and queues five. What does not fit lands in active display,
 * where the PPU ignores it: no error, missing tiles.
 *
 * Request and first user: issue #165.
 */

#ifndef OPENSNES_VRAMQUEUE_H
#define OPENSNES_VRAMQUEUE_H

#include <snes/types.h>

/** @brief Transfers the queue holds */
#define VRAM_QUEUE_MAX     32

/** @brief Step after each word: the next VRAM word (a row, tiles) */
#define VRAM_QUEUE_ROW     0x80
/** @brief Step after each word: 32 words on (a column of a 32-wide tilemap) */
#define VRAM_QUEUE_COLUMN  0x81

/* The queue itself (vramqueue.asm): five arrays of words, one entry each. */
extern u16 vram_queue_src[VRAM_QUEUE_MAX];
extern u16 vram_queue_bank[VRAM_QUEUE_MAX];
extern u16 vram_queue_addr[VRAM_QUEUE_MAX];
extern u16 vram_queue_size[VRAM_QUEUE_MAX];
extern u16 vram_queue_step[VRAM_QUEUE_MAX];
extern u16 vram_queue_count;

/** @brief Entries still free */
#define vramQueueFree() (VRAM_QUEUE_MAX - vram_queue_count)

/**
 * @brief Note one transfer for the next vramQueueFlush()
 *
 * @param src   Source bytes (ROM, plain RAM or FAR RAM)
 * @param addr  VRAM word address
 * @param size  Bytes (even; an entry of 0 bytes is skipped by the flush)
 * @param step  VRAM_QUEUE_ROW or VRAM_QUEUE_COLUMN
 * @return 1, or 0 when the queue is full: nothing was noted
 */
u16 vramQueuePush(const u8 *src, u16 addr, u16 size, u16 step);

/**
 * @brief Send every queued transfer to VRAM and empty the queue
 *
 * One DMA per entry, on channel 0. Call it in VBlank — right after
 * WaitForVBlank() — or in force blank: it writes VRAM. With an empty queue
 * it returns at once. VMAIN is left at $80, as the rest of the library
 * leaves it.
 *
 * @warning Not from an NMI callback while the main code may be in the
 *          middle of a vramQueuePush(): the entry would be half written.
 */
void vramQueueFlush(void);

#endif /* OPENSNES_VRAMQUEUE_H */
