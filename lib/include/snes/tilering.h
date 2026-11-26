/**
 * @file tilering.h
 * @brief Scrolling in eight directions over a map with more characters than VRAM holds
 *
 * A tilemap entry names one of 1,024 characters, and a 4 bpp background
 * rarely gets more than 512 to 768 of them once sprites and a status bar
 * have their share of VRAM. A large hand-drawn map easily has 1,200 to
 * 1,600 distinct 8x8 characters. What the screen shows at once is much
 * less (300 to 450 in the maps measured): the module keeps a **ring** of
 * character slots in VRAM, and as the camera moves it loads the characters
 * of the row and the column that come into view, and frees the slots of
 * those that leave.
 *
 * @code
 * #include <snes/tilering.h>          // and `tilering vramqueue` in LIB_MODULES
 * #include "res/world.inc"           // opensnes-tilering convert res/world.png
 *
 * static const TileRingMap world = world_TILERING_MAP;
 * TileRing ring;                      // plain RAM, 258 bytes
 * FAR u16 slot_of[world_CHARS];       // the work areas, FAR RAM
 * FAR u16 char_of[640];
 * FAR u8  refs[640];
 * FAR u8  staging[TILERING_STAGING_BYTES(33, 29)];
 *
 * tileRingInit(&ring, &world, 0x0000, 0, 640, 0x6000, slot_of, char_of, refs, staging);
 * bgSetGfxPtr(0, 0x0000);
 * bgSetMapPtr(0, 0x6000, SC_64x32);
 * tileRingFill(&ring, camx, camy);    // screen off: the whole first screen
 * setScreenOn();
 * while (1) {
 *     // ... move camx, camy (at most 8 px a frame on each axis)
 *     tileRingUpdate(&ring, camx, camy);     // queues what changed
 *     WaitForVBlank();
 *     vramQueueFlush();                      // sends it
 *     bgSetScroll(0, camx, camy);
 * }
 * @endcode
 *
 * **The map** is what `opensnes-tilering convert` writes from an indexed
 * PNG: each 8x8 character kept once, mirrored copies merged (the map entry
 * carries the flip), numbered in order of first appearance row by row from
 * the bottom of the map. The map entry is 16 bits: character 0-4095 in bits
 * 0-11, priority in bit 13, the flips in bits 14 and 15; the palette is a
 * property of the character.
 *
 * **The tilemap** is 64x32 (`SC_64x32`, 4 KB of VRAM). The window, the
 * cells in view, is 33x29 characters by default (the converter's
 * `--window`): enough for a 256x224 screen at any scroll. Map cell (x, y)
 * lives at tilemap cell (x mod 64, y mod 32).
 *
 * **The slots.** Slot `s` is the character `first_char + s` of the
 * background, at VRAM word `chr_vram + 16 * s`. Each slot counts the cells
 * of the window that show it; a slot whose count falls to 0 keeps its
 * character until it is needed again: the character comes back without a
 * transfer if the camera returns first. New characters take the free slots
 * in sequence, from a cursor that goes round the ring, so the characters of
 * a new row usually land in consecutive slots and leave in one or two
 * transfers.
 *
 * **What it costs, and who decides.** tileRingUpdate() reads the map,
 * updates the counts, and notes in the VRAM queue the characters to load
 * and the tilemap row and column to write. It sends nothing itself:
 * vramQueueFlush() does, in VBlank, and vramQueueCost() says beforehand
 * what that will take. tileRingCost() says the most a frame can bring for a
 * speed. No budget is imposed: a game that wants a cap tests
 * vramQueueCost() before the flush, or moves the camera less.
 *
 * Two ways to send the characters:
 * - **staging** (a buffer given): the frame's new characters are copied to
 *   it, one DMA from ROM to WRAM per run of consecutive characters, outside
 *   VBlank; the queue then holds one entry per run of consecutive SLOTS,
 *   usually one or two;
 * - **direct** (no buffer, `staging` NULL): one queue entry per run of
 *   characters consecutive both in ROM and in the ring. No copy and no RAM,
 *   but in VBlank each entry costs VRAM_QUEUE_ENTRY_COST more.
 *
 * @note No HDMA on DMA channel 0 with this module, in either mode: the
 *       staging copy, tileRingFill() and vramQueueFlush() all rewrite that
 *       channel's registers. HDMA goes on channels 1-6 (snes/hdma.h).
 * @note The camera moves at most one character (8 px) a frame on each
 *       axis: tileRingUpdate() moves the window one row and one column at
 *       most and returns how far it still is. A jump is a new
 *       tileRingFill() (screen off) or the game's own re-centring over
 *       several frames.
 */

#ifndef OPENSNES_TILERING_H
#define OPENSNES_TILERING_H

#include <snes/types.h>

/** @brief Bytes of one 4 bpp character */
#define TILERING_CHAR_BYTES 32

/** @brief Bytes of the staging buffer for a window of w x h characters:
 *         one row and one column of new characters at most */
#define TILERING_STAGING_BYTES(w, h) (((w) + (h)) * TILERING_CHAR_BYTES)

/** @brief Value of slot_of[] for a character not in the ring, of char_of[]
 *         for a slot that never held one */
#define TILERING_NONE 0xFFFF

/**
 * @brief The map as opensnes-tilering wrote it: `NAME_TILERING_MAP` in the
 *        generated .inc fills it
 */
typedef struct {
    const u8 *header;               /**< .trh: sizes, window, worst figures */
    const u8 *charpal;              /**< .tpl: one byte a character, palette << 2 */
    const u8 *const *char_parts;    /**< the characters, 1,024 (32 KB) a part */
    const u8 *const *map_parts;     /**< the map, whole rows, 32 KB at most a part */
} TileRingMap;

/**
 * @brief One ring: fill it with tileRingInit(); the fields are read-only
 */
typedef struct {
    /* the map */
    const TileRingMap *map;
    u16 width, height, chars;       /**< map size in characters; distinct characters */
    u16 rows_per_part;
    u16 win_w, win_h;               /**< the window, in characters */
    u16 worst_row, worst_col, worst_window;   /**< the converter's figures */
    /* VRAM */
    u16 chr_vram;                   /**< VRAM word address of slot 0's character */
    u16 first_char;                 /**< character number of slot 0 in the BG */
    u16 slots;                      /**< slots in the ring */
    u16 tilemap;                    /**< VRAM word address of the 64x32 tilemap */
    /* the work areas */
    u16 FAR *slot_of;               /**< chars words: the slot of each character */
    u16 FAR *char_of;               /**< slots words: the character of each slot */
    u8 FAR *refs;                   /**< slots bytes: cells of the window on each slot */
    u8 FAR *staging;                /**< NULL: direct mode */
    /* state */
    u16 cx, cy;                     /**< the window's top-left cell */
    u16 cursor;                     /**< the next slot looked at for a new character */
    u16 resident;                   /**< slots shown by at least one cell of the window */
    /** @brief Characters noted by the last tileRingUpdate() */
    u16 last_chars;
    /** @brief Queue entries noted by the last tileRingUpdate() (characters and tilemap) */
    u16 last_entries;
    /** @brief Characters that found no free slot since tileRingInit():
     *         0 on a healthy run (the ring is smaller than the window needs) */
    u16 starved;
    /** @brief Pushes the VRAM queue refused since tileRingInit() (queue full):
     *         0 on a healthy run */
    u16 refused;
    /* internal */
    u16 filling;
    u16 row_buf[64];
    u16 col_buf[32];
} TileRing;

/**
 * @brief Set a ring up: no VRAM is written
 *
 * @param r          The ring (plain RAM)
 * @param map        The converted map (`static const TileRingMap m = NAME_TILERING_MAP;`)
 * @param chr_vram   VRAM word address of slot 0's character
 * @param first_char Its character number for the background (from the BG's
 *                   character base: (chr_vram - base) / 16)
 * @param slots      Slots in the ring; first_char + slots <= 1024. The
 *                   window's worst (`NAME_WORST_WINDOW`) must fit, with room
 *                   to spare for the characters kept for a return
 * @param tilemap    VRAM word address of a 64x32 tilemap
 * @param slot_of    `NAME_CHARS` words (FAR RAM is fine)
 * @param char_of    `slots` words
 * @param refs       `slots` bytes
 * @param staging    TILERING_STAGING_BYTES(window) bytes, or NULL (direct mode)
 * @return 1, or 0 when the map is not a tilering map or the window is larger than 64x32
 */
u16 tileRingInit(TileRing *r, const TileRingMap *map, u16 chr_vram, u16 first_char,
                 u16 slots, u16 tilemap, u16 FAR *slot_of, u16 FAR *char_of,
                 u8 FAR *refs, u8 FAR *staging);

/**
 * @brief Load the whole window at a camera position: screen off or in force blank
 *
 * Empties the ring, then writes every character of the window and the
 * window's tilemap with direct DMA (not through the queue). For the
 * first screen of a level, or after a jump. 448 characters, the worst of
 * the maps measured, are 14,336 bytes: a VBlank holds about 5 KB.
 *
 * @param r     The ring
 * @param camx  Camera x in map pixels (the screen's left edge)
 * @param camy  Camera y in map pixels (the screen's top edge)
 */
void tileRingFill(TileRing *r, u16 camx, u16 camy);

/**
 * @brief Follow the camera: queue the characters and tilemap the move needs
 *
 * Moves the window one column and one row at most toward the camera, then
 * notes in the VRAM queue the new characters (see the two modes above) and
 * the tilemap column (one or two entries) and row (two entries). Call it
 * once a frame, before WaitForVBlank(), and vramQueueFlush() after: the
 * entries point into the ring and the staging buffer, which the next call
 * rewrites.
 *
 * @param r     The ring
 * @param camx  Camera x in map pixels, at most width * 8 - 256
 * @param camy  Camera y in map pixels, at most height * 8 - 224
 * @return Cells the window still lags behind the camera, the two axes
 *         added: 0 when it caught up
 */
u16 tileRingUpdate(TileRing *r, u16 camx, u16 camy);

/**
 * @brief The most characters a frame can bring at a speed
 *
 * From the converter's figures for this map: a speed of dx pixels a frame
 * crosses (dx + 7) / 8 column boundaries at most, each bringing at most
 * the map's worst column; the same for dy and rows. In bytes for the
 * VBlank: the characters times TILERING_CHAR_BYTES, plus the tilemap (a
 * row 2 x win_w bytes in two entries, a column 2 x win_h in two), plus
 * VRAM_QUEUE_ENTRY_COST for each entry.
 *
 * @param r   The ring
 * @param dx  Horizontal speed, pixels a frame (absolute value)
 * @param dy  Vertical speed, pixels a frame (absolute value)
 * @return Characters, at most
 */
u16 tileRingCost(const TileRing *r, u16 dx, u16 dy);

#endif /* OPENSNES_TILERING_H */
