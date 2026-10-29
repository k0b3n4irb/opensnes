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
 * replaces. But six 128-byte transfers called directly put 15,900 master
 * cycles in the VBlank, and queued only 10,600 — 8 per byte plus about 740
 * per entry, the call to the flush included (`devtools/libbench`, rows
 * `vramc` and `vramq`).
 *
 * **The budget is yours to keep.** A VBlank is about 49,000 master cycles
 * and the NMI handler takes the first 5,000 to 8,000 of them. What does
 * not fit lands in active display,
 * where the PPU ignores it: no error, missing tiles. vramQueueCost() says
 * what the entries already queued will take, whichever module queued them.
 *
 * Request and first user: issue #165.
 */

#ifndef OPENSNES_VRAMQUEUE_H
#define OPENSNES_VRAMQUEUE_H

#include <snes/types.h>

/**
 * @brief Transfers the queue holds
 *
 * 32 unless the project's Makefile says otherwise (`VRAM_QUEUE_MAX := 64`,
 * 1 to 255): the build gives the C files this value and sizes the queue's
 * arrays to it (templates/vramqueue_ram.asm). Each entry takes 12 bytes of
 * the C RAM band ($0000-$1FFF). vramQueueFlush() sends every entry in one
 * call; vramQueueFlushUntil() sends what ends before a given scanline.
 */
#ifndef VRAM_QUEUE_MAX
#define VRAM_QUEUE_MAX     32
#endif

/** @brief Step after each word: the next VRAM word (a row, tiles) */
#define VRAM_QUEUE_ROW     0x80
/** @brief Step after each word: 32 words on (a column of a 32-wide tilemap) */
#define VRAM_QUEUE_COLUMN  0x81

/* The queue itself (vramqueue.asm): six arrays of words, one entry each. */
extern u16 vram_queue_src[VRAM_QUEUE_MAX];
extern u16 vram_queue_bank[VRAM_QUEUE_MAX];
extern u16 vram_queue_addr[VRAM_QUEUE_MAX];
extern u16 vram_queue_size[VRAM_QUEUE_MAX];
extern u16 vram_queue_step[VRAM_QUEUE_MAX];
extern u16 vram_queue_cum[VRAM_QUEUE_MAX];  /* vram_queue_cost up to each entry */
extern u16 vram_queue_count;
extern u16 vram_queue_cost;

/** @brief Entries still free */
#define vramQueueFree() (VRAM_QUEUE_MAX - vram_queue_count)

/**
 * @brief What one entry costs the flush beyond its bytes, in bytes
 *
 * The flush spends 8 master cycles per byte and, in its loop, 619 per entry
 * (SlowROM; 560 with USE_FASTROM): 619 / 8 is 77 bytes' worth, rounded up.
 */
#define VRAM_QUEUE_ENTRY_COST  80

/**
 * @brief VBlank cost of what is queued, in bytes: each entry's size plus
 *        VRAM_QUEUE_ENTRY_COST
 *
 * Kept by vramQueuePush() and vramQueuePushSprite(), set to 0 by
 * vramQueueFlush(), to the cost of what is left by vramQueueFlushUntil();
 * it stops at 0xFFFF rather than wrap. Multiply by 8 for
 * master cycles. A module that streams what is left of the VBlank checks it
 * against its own budget before each push:
 *
 * @code
 * // a 32x32 frame: four strips of 128 bytes
 * while (next < wanted && vramQueueCost() + 512 + 4 * VRAM_QUEUE_ENTRY_COST <= BUDGET) {
 *     vramQueuePushSprite(frame_src[next], frame_dst[next], 32);
 *     next++;
 * }
 * @endcode
 *
 * A count written by hand (`vram_queue_count = 0`) leaves it as it was
 * until the next flush.
 */
#define vramQueueCost() (vram_queue_cost)

/**
 * @brief Note one transfer for the next vramQueueFlush()
 *
 * @param src   Source bytes (ROM, plain RAM or FAR RAM)
 *              The queue keeps the POINTER: the bytes are read at the
 *              flush, not here. They must not change before
 *              vramQueueFlush(); a buffer filled again before the flush
 *              sends its last contents for every entry that points to it.
 * @param addr  VRAM word address
 * @param size  Bytes (even; an entry of 0 bytes is skipped by the flush)
 * @param step  VRAM_QUEUE_ROW or VRAM_QUEUE_COLUMN
 * @return 1, or 0 when the queue is full: nothing was noted
 */
u16 vramQueuePush(const u8 *src, u16 addr, u16 size, u16 step);

/**
 * @brief Note one frame of a streamed sprite: all its strips in one call
 *
 * A sprite frame in OBJ VRAM is `size_px / 8` rows of tiles; each row is a
 * strip of `size_px * 4` bytes (at 4 bpp), and the rows are 16 tiles apart:
 * 512 bytes in the sheet, 256 words in VRAM. That is the layout
 * `opensnes-sprite sheet` writes (a 128-pixel-wide raster) and the one OBJ
 * VRAM expects. This notes the 1, 2, 4 or 8 strips of one frame.
 *
 * @param src      First byte of the frame in the sheet (its top-left tile).
 *                 The frame must not straddle a bank boundary.
 *                 Read at the flush, like vramQueuePush(): the bytes
 *                 must not change before vramQueueFlush().
 * @param addr     VRAM word address of the frame's top-left tile
 * @param size_px  Width and height of the sprite: 8, 16, 32 or 64
 * @return 1, or 0 when the strips do not all fit in the queue (none is
 *         noted) or size_px is under 8
 */
u16 vramQueuePushSprite(const u8 *src, u16 addr, u16 size_px);

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

/**
 * @brief Send the queued transfers that end before a scanline; keep the rest
 *
 * Reads the vertical counter once, turns the lines left before @p line
 * into a budget (165 bytes of vramQueueCost() a line: 1324 master cycles,
 * the DRAM refresh taken out), and sends the entries in queue order while
 * their running cost fits. The first one that does not fit stops it: the
 * entries left move to the head of the queue **in the same order** and go
 * first at the next flush, so a VRAM address pushed twice still receives
 * the second transfer last. Use it to fill every VBlank whatever the NMI
 * took, and push beyond a safe budget.
 *
 * - The vertical blank ends after line 261 (NTSC) or 311 (PAL): pass 262
 *   or 312 to use all of it, less to keep a margin. A blank begins at line
 *   225, or 240 with overscan.
 * - Outside the vertical blank (HVBJOY bit 7 clear) it sends nothing: a
 *   flush called late, in a lag frame, does not write into active display.
 *   In force blank use vramQueueFlush().
 * - What is left stays queued: its sources are read at a later flush, so a
 *   RAM buffer behind a kept entry must not change until then (ROM is
 *   safe). vramQueueFree() counts the entries left as taken, and
 *   vramQueueCost() is their cost.
 * - The routine itself costs VRAM_QUEUE_UNTIL_COST bytes of the budget up
 *   to its first transfer, whatever the queue's length (the cut is found
 *   by halving). Moving what is left takes time after the blank, not in it.
 * - It reads the H/V counter: a Super Scope or light-gun latch read later
 *   in the same frame sees this one.
 *
 * @param line  Scanline by whose start the transfers must be over
 * @return      Entries left in the queue
 *
 * @warning Not from an NMI callback while the main code may be in the
 *          middle of a vramQueuePush().
 */
u16 vramQueueFlushUntil(u16 line);

/** @brief Bytes of the budget vramQueueFlushUntil() spends before its first transfer */
#define VRAM_QUEUE_UNTIL_COST  64

#endif /* OPENSNES_VRAMQUEUE_H */
