/**
 * @file main.c
 * @brief HDMA wave: build the table by hand, then let the hdma module do it
 * @ingroup examples
 *
 * One water image, one per-scanline ripple, two ways to make it.
 *
 * At boot the ripple comes from an HDMA table written BY HAND and animated
 * the way krom's assembler demo does it: the table is never rewritten; each
 * VBlank the table START POINTER advances one entry, so line L reads entry
 * phase+L and the crests flow up the screen. That is the HDMA table format
 * itself — `[line-count, value...]` entries and a terminator — and the
 * cheapest animation there is.
 *
 * Press A and the same ripple comes from the `hdma` module instead:
 * hdmaWaveH() computes the sine table in RAM and hdmaWaveUpdate() animates
 * it, with the amplitude now a parameter (LEFT/RIGHT). The defaults
 * (amplitude 10, frequency 10: a 25.6-line period) sit close to krom's
 * table (round(10 * sin), a ~25.8-line quasi-period), so the switch shows
 * the helper reproducing what the hand-built table did. Press A again to go
 * back.
 *
 * C port of "SNES Wave HDMA Demo" by krom (Peter Lemon),
 * github.com/PeterLemon/SNES, PPU/HDMA/WaveHDMA — technique reproduced on
 * the snes/hdma.h API in the original demo configuration (BG Mode 3,
 * full-screen 256-color image). Art is original: procedurally generated
 * water caustics (res/water.bmp), no krom assets.
 *
 * @par SNES Concepts
 * - HDMA table format: `count` byte (1 = apply to one scanline) followed
 *   by the register payload (2 bytes for a write-twice register), then a
 *   0x00 terminator byte
 * - HDMA_MODE_1REG_2X: one register written twice per line — exactly what
 *   the 16-bit scroll registers ($210D BG1HOFS low/high) expect
 * - Animation by START-POINTER repoint (hdmaSetup once per frame with
 *   `table + phase*3`): the table itself is immutable, so HDMA never
 *   observes a partially rewritten entry — no tearing, ~zero CPU cost
 * - The same effect from the library: hdmaWaveH / hdmaWaveUpdate /
 *   hdmaWaveStop, tables computed and double-buffered in RAM
 * - BG Mode 3 (8bpp, 256 colors) with a full-screen image — the same
 *   configuration as the original demo (~57 KB of unique tiles, split
 *   across two ROM banks; the tilemap sits above them at VRAM $7C00)
 *
 * @par What to Observe
 * - At boot: the water image in tight sine ripples flowing UPWARD at one
 *   scanline per frame — krom's exact table (896 entries, verbatim) and
 *   cadence (wrap at 672); the white rulers every 64px stay straight
 * - Press A: the hdma module takes over (channel 6 instead of 0); the
 *   ripple looks nearly the same
 * - LEFT/RIGHT (module mode): amplitude from 2 to 24 pixels, in steps of 2
 * - Press A again: back to the hand-built table, from where it was
 *
 * @par Modules Used
 * console, dma, background, input, hdma
 *
 * @see hdma.h, examples/hdma/hdma_helpers (the other hdma module effects)
 */

#include <snes.h>
#include <snes/hdma.h>
#include <snes/input.h>

/** @brief Wrap length of the start-pointer animation, in table entries.
 *  krom's exact value: his sine's quasi-period is ~25.8 lines (non-
 *  integer), so his seamless wrap needs 672 entries; the table holds
 *  224 more so any start phase has a full screen of valid lines. */
#define WAVE_WRAP     672
/** @brief Bytes per HDMA entry in 1REG_2X mode: count + 16-bit value */
#define ENTRY_BYTES   3

/** @brief Channel of the hand-built table (krom's channel 0) */
#define CH_HAND       HDMA_CHANNEL_0
/** @brief Channel of the hdma module's wave (7 belongs to the OAM DMA) */
#define CH_LIB        HDMA_CHANNEL_6

/** @brief Module-mode amplitude: default, bounds and step, in pixels */
#define AMP_DEFAULT   10
#define AMP_MIN       2
#define AMP_MAX       24
#define AMP_STEP      2
/** @brief Module-mode frequency: period = 256 / 10 = 25.6 lines */
#define WAVE_FREQ     10

/** @brief VRAM word address of BG1 tile graphics (image start) */
#define VRAM_BG1_GFX  0x0000
/** @brief VRAM word address of the second tile half (bytes 32768+) */
#define VRAM_BG1_GFX2 0x4000
/** @brief VRAM word address of BG1 tilemap — above the 57 KB of tiles,
 *  krom's layout (his BG1SC put the map at VRAM byte $F800) */
#define VRAM_BG1_MAP  0x7C00

/** @brief Mode 3 image data (from data.asm; generated original art) */
extern u8 tiles[], tiles_end[];
extern u8 tiles2[], tiles2_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

/**
 * @brief krom's exact HDMA table, in ROM (data.asm).
 *
 * 896 entries of [1][offset16] + terminator, extracted verbatim from
 * the original demo: entry values are round(10*sin) samples with a
 * quasi-period of ~25.8 lines. The table is never written: the
 * animation only moves the start pointer (hdmaSetup reads the bank
 * from the far pointer, so the table can sit in any ROM bank).
 */
extern u8 wavetable[];

/** @brief Current wave phase of the hand-built table, in entries */
static u16 wave_phase;
/** @brief 0 = hand-built table (channel 0), 1 = hdma module (channel 6) */
static u8 use_lib;
/** @brief Module-mode amplitude in pixels */
static u8 amp;

/** @brief Point channel 0 at the hand-built table from the current phase. */
static void hand_point(void) {
    hdmaSetup(CH_HAND, HDMA_MODE_1REG_2X, HDMA_DEST_BG1HOFS,
              wavetable + wave_phase * ENTRY_BYTES);
}

/** @brief (Re)start the module's wave with the current amplitude. */
static void lib_start(void) {
    hdmaWaveH(CH_LIB, 0, amp, WAVE_FREQ);
    hdmaEnableMask(1 << CH_LIB);
}

/**
 * @brief Entry point — hand-built HDMA table, then the hdma module.
 *
 * Init order per the SDK convention: console, VRAM, palette, BG pointers,
 * mode, HDMA, screen on. The main loop repoints the hand-built table once
 * per VBlank, or lets hdmaWaveUpdate() animate the module's wave.
 *
 * @return Never returns (infinite loop)
 */
int main(void) {
    u16 pressed;

    consoleInit();

    /* Load the full-screen 8bpp image: two tile halves (>32KB tileset),
     * tilemap above them, 256-color palette — krom's Mode 3 setup on
     * the SDK API. All transfers run during the boot force blank. */
    dmaCopyVram(tiles,   VRAM_BG1_GFX,  (u16)(tiles_end - tiles));
    dmaCopyVram(tiles2,  VRAM_BG1_GFX2, (u16)(tiles2_end - tiles2));
    dmaCopyVram(tilemap, VRAM_BG1_MAP,  (u16)(tilemap_end - tilemap));
    dmaCopyCGram(palette, 0, (u16)(palette_end - palette));

    bgSetGfxPtr(0, VRAM_BG1_GFX);
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    setMode(BG_MODE3, 0);

    wave_phase = 0;
    use_lib = 0;
    amp = AMP_DEFAULT;
    hand_point();
    hdmaEnableMask(1 << CH_HAND);

    setMainScreen(TM_BG1);
    setScreenOn();

    while (1) {
        WaitForVBlank();
        pressed = padPressed(0);

        if (pressed & KEY_A) {
            if (use_lib) {
                /* Back to the hand-built table, from the phase it left. */
                hdmaWaveStop();
                use_lib = 0;
                hand_point();
                hdmaEnableMask(1 << CH_HAND);
            } else {
                /* hdmaWaveInit() switches every channel off, channel 0
                 * included — call it before arming the module's own. */
                hdmaWaveInit();
                use_lib = 1;
                lib_start();
            }
        }

        if (use_lib) {
            if ((pressed & KEY_RIGHT) && amp < AMP_MAX) {
                amp = amp + AMP_STEP;
                lib_start();
            }
            if ((pressed & KEY_LEFT) && amp > AMP_MIN) {
                amp = amp - AMP_STEP;
                lib_start();
            }
            hdmaWaveUpdate();
        } else {
            /* krom's exact cadence: advance one entry per frame, wrap after
             * 672 entries. Repointing during VBlank is safe: HDMA reloads
             * its table address at the start of every frame. */
            wave_phase++;
            if (wave_phase >= WAVE_WRAP)
                wave_phase = 0;
            hand_point();
        }
    }

    return 0;
}
