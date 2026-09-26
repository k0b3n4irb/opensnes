/**
 * @file main.c
 * @brief HDMA-driven sinusoidal wave distortion on background
 * @ingroup examples
 *
 * Creates a wavy horizontal distortion effect by using HDMA to write
 * per-scanline BG1 horizontal scroll offsets (BG1HOFS, $210D). A set of
 * 7 pre-computed sine tables at increasing amplitudes (0 to 24 pixels) are
 * stored in ROM, each containing 335 entries (224 visible scanlines + 111
 * wrap entries for smooth phase animation). HDMA channel 6 is configured
 * in write-twice mode (1REG_2X) to write both the low and high bytes of
 * BG1HOFS each scanline. When animation is enabled, the table pointer
 * advances by 3 bytes per frame (one HDMA entry), cycling through the
 * wrap portion to create continuous wave motion. The background consists
 * of simple alternating solid/empty tiles to make the distortion clearly
 * visible.
 *
 * @par SNES Concepts
 * - HDMA write-twice mode (1REG_2X) targeting BG1HOFS ($210D)
 * - Per-scanline horizontal scroll offset for wave distortion
 * - Repeat-mode HDMA entries (bit 7 set = write every scanline)
 * - Pre-computed sine tables with wrap region for seamless animation
 * - Bare-metal VRAM and CGRAM writes (no library DMA helpers)
 *
 * @par What to Observe
 * - Press A to toggle the wave effect on/off
 * - Press LEFT/RIGHT to decrease/increase wave amplitude (7 levels)
 * - Press UP to start wave animation (the wave scrolls vertically)
 * - Press DOWN to stop animation (wave freezes in place)
 *
 * @par Modules Used
 * console, dma, sprite, input, hdma
 *
 * @see hdma.h, input.h, video.h
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/input.h>
#include <snes/hdma.h>

/**
 * @brief Minimal 4bpp tile data: one empty tile and one solid tile, in VRAM
 *        byte order (the order dmaCopyVram writes).
 *
 * A 4bpp tile is 32 bytes: rows 0-7 of bitplanes 0 and 1 interleaved
 * (plane 0 byte, plane 1 byte), then rows 0-7 of planes 2 and 3. "Solid
 * colour 1" sets plane 0 on every row and nothing else, so the tile is
 * eight {0xFF, 0x00} pairs followed by sixteen zeros. The alternating
 * empty/solid tilemap draws vertical stripes that make the wave visible.
 */
static const u8 tiles[64] = {
    /* Tile 0: empty (colour 0) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* Tile 1: solid colour 1 — plane 0 = 0xFF on each row, planes 1-3 = 0 */
    0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/** @brief Palette: colour 0 black, colour 1 white (BGR555). */
static const u16 palette[2] = { 0x0000, 0x7FFF };

/** @brief BG1 tilemap built at boot: tile 0 / tile 1 alternating (32x32). */
static u16 FAR stripes[1024];

#define AMP_LEVELS    7     /**< Number of amplitude levels (0, 4, 8, 12, 16, 20, 24 pixels) */
#define TABLE_ENTRIES 335   /**< Entries per table: 224 visible scanlines + 111 wrap entries for smooth animation */
#define TABLE_SIZE    1006  /**< Bytes per amplitude table: 335 entries x 3 bytes + 1 end marker (0x00) */
#define SINE_PERIOD   112   /**< Sine wave period in scanlines (half the visible screen height) */
#define AMP_DEFAULT   3     /**< Default amplitude index (amplitude level 12 pixels) */

/**
 * @brief Pre-computed HDMA sine wave tables for 7 amplitude levels.
 *
 * 7 × 1006 = 7042 bytes, defined in `res/hdma_wave_tables.bin` and
 * brought in by `data.asm` via `.incbin`. Layout per amplitude level
 * (TABLE_SIZE = 1006 bytes):
 * - 335 HDMA entries, each 3 bytes:
 *   - Byte 0: 0x81 = repeat flag (bit 7) + 1 scanline count (bits 6-0).
 *     The repeat flag tells HDMA to re-read data bytes for every scanline
 *     in the group (here, just 1 scanline), required for smooth per-line
 *     distortion.
 *   - Bytes 1-2: BG1HOFS low + high (signed 16-bit scroll offset).
 *     Positive shifts BG right, negative (two's complement, 0xFF 0xFF =
 *     -1) shifts left.
 * - 1 terminator byte (0x00).
 *
 * The 335 entries cover 224 visible scanlines plus 111 wrap entries. By
 * advancing the HDMA start pointer by 3 bytes (one entry) per frame, the
 * wave pattern appears to scroll vertically. After 112 entries (one full
 * sine period), the pattern repeats seamlessly thanks to the wrap region.
 *
 * Amplitude levels: 0, 4, 8, 12, 16, 20, 24 pixels peak displacement.
 */
extern u8 hdma_tables[];

/** @name Wave Effect State
 *  @brief Runtime state variables for the wave distortion effect.
 *
 *  These are `static` (file-scope) variables, which the compiler places in
 *  WRAM via RAMSECTION + .data_init. They persist across frames and are
 *  initialized explicitly in main() rather than relying on C static init.
 *  @{
 */
static u8 wave_on;      /**< Wave effect enabled flag (0=off, 1=on) */
static u8 amp_idx;       /**< Current amplitude index (0-6, selects which table to use) */
static u8 animating;     /**< Animation enabled flag (0=frozen, 1=scrolling) */
static u8 phase;         /**< Current animation phase (0-111, index into sine period) */
/** @} */

/**
 * @brief Pre-computed byte offsets into hdma_tables[] for each amplitude level.
 *
 * Since each amplitude table is TABLE_SIZE (1006) bytes, the offset for level
 * N is N * 1006. These are pre-computed to avoid runtime multiplication (the
 * 65816 has no hardware multiply for 16-bit values, so `amp_idx * 1006` would
 * require an expensive software multiply routine).
 */
static const u16 amp_offsets[AMP_LEVELS] = {
    0, 1006, 2012, 3018, 4024, 5030, 6036
};

/**
 * @brief Entry point -- HDMA-driven sinusoidal wave distortion on background.
 *
 * Sets up a simple striped background using bare-metal VRAM and CGRAM writes
 * (no library DMA helpers), then configures HDMA channel 6 to apply
 * per-scanline horizontal scroll offsets from pre-computed sine tables.
 *
 * Controls:
 * - A: toggle wave on/off
 * - LEFT/RIGHT: decrease/increase amplitude
 * - UP: start animation (wave scrolls vertically)
 * - DOWN: stop animation (wave freezes)
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    u16 i;

    /* State set explicitly at the top of main (C static initialisers work
     * too: crt0 copies them from ROM before main runs) */
    wave_on = 0;
    amp_idx = AMP_DEFAULT;
    animating = 0;
    phase = 0;

    consoleInit();
    setMode(BG_MODE1, 0);

    /* Everything below happens in force blank (consoleInit leaves the screen
     * off), so the DMA helpers may write VRAM and CGRAM at any time. BG1's
     * tilemap is at VRAM $0400 and its tiles at $0000 (consoleInit's
     * defaults). Until 2026-09-26 this example wrote $2115-$2122 by hand —
     * the opposite of what an example should teach. */
    dmaCopyVram(tiles, 0x0000, sizeof(tiles));
    dmaCopyCGram((const u8 *)palette, 0, sizeof(palette));
    for (i = 0; i < 1024; i++) {
        stripes[i] = i & 1;           /* tile 1 on odd columns, 0 on even */
    }
    dmaCopyVram((const u8 *)stripes, 0x0400, sizeof(stripes));

    /* Configure HDMA channel 6 in write-twice mode (1REG_2X) targeting
     * BG1HOFS ($210D). In this mode, each HDMA entry writes 2 bytes to the
     * same register: the low byte then the high byte of BG1's horizontal
     * scroll offset. This produces per-scanline horizontal displacement. */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG_2X, HDMA_DEST_BG1HOFS,
              &hdma_tables[amp_offsets[amp_idx]]);

    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* A button: toggle wave on/off */
        if (padPressed(0) & KEY_A) {
            if (wave_on == 0) {
                wave_on = 1;
            } else {
                wave_on = 0;
                animating = 0;
                phase = 0;
            }
        }

        /* D-pad RIGHT: increase amplitude */
        if (padPressed(0) & KEY_RIGHT) {
            if (amp_idx < 6) {
                amp_idx = amp_idx + 1;
            }
        }

        /* D-pad LEFT: decrease amplitude */
        if (padPressed(0) & KEY_LEFT) {
            if (amp_idx > 0) {
                amp_idx = amp_idx - 1;
            }
        }

        /* D-pad UP: start animation */
        if (padPressed(0) & KEY_UP) {
            animating = 1;
        }

        /* D-pad DOWN: stop animation (freeze at current phase) */
        if (padPressed(0) & KEY_DOWN) {
            animating = 0;
        }

        /* Advance animation phase.
         * Each frame, the phase advances by 1 entry (3 bytes) in the HDMA table.
         * After 112 steps (one full sine period), it wraps to 0. Because the
         * table has 335 entries (224 + 111 wrap), the 224 visible scanlines
         * always stay within bounds regardless of the starting phase. */
        if (animating) {
            phase = phase + 1;
            if (phase >= 112) {
                phase = 0;
            }
        }

        /* Update HDMA table pointer and enable/disable.
         * hdmaSetTable() changes the DMA source address for channel 6 to point
         * into the current amplitude's table, offset by (phase * 3) bytes.
         * The `* 3` is because each HDMA entry is 3 bytes (1 control + 2 data).
         * 0x40 = bit 6 = HDMA channel 6 bitmask. */
        if (wave_on) {
            hdmaSetTable(HDMA_CHANNEL_6,
                         &hdma_tables[amp_offsets[amp_idx] + (u16)phase * 3]);
            hdmaEnable(1 << HDMA_CHANNEL_6);
        } else {
            hdmaDisable(1 << HDMA_CHANNEL_6);
        }
    }
    return 0;
}
