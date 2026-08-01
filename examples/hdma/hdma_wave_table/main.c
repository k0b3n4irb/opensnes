/**
 * @file main.c
 * @brief Raw HDMA table in C — per-scanline wave, krom-style
 * @ingroup examples
 *
 * Builds an HDMA table BY HAND in C and animates it the way the original
 * assembler demo does: the table is written once, and each VBlank the
 * table START POINTER advances one entry — the ripple pattern flows up
 * the screen (line L reads entry phase+L, so each crest sits at
 * X0-phase) without a single byte of the table being rewritten. This is the
 * classic per-scanline effect that plain DMA cannot do: one BG1 horizontal
 * scroll value per line, traced along a sine.
 *
 * The companion example `hdma_wave` shows the same visual through the
 * library's high-level engine (hdmaWaveH / hdmaWaveUpdate, double-buffered
 * RAM tables). THIS example is the low-level counterpart: it teaches the
 * HDMA table FORMAT itself — `[line-count, value...]` entries, terminator,
 * and the repoint-per-frame animation idiom.
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
 * - BG Mode 3 (8bpp, 256 colors) with a full-screen image — the same
 *   configuration as the original demo (~57 KB of unique tiles, split
 *   across two ROM banks; the tilemap sits above them at VRAM $7C00,
 *   mirroring krom's layout)
 *
 * @par What to Observe
 * - A water image distorted into tight sine ripples flowing UPWARD at
 *   one scanline per frame — krom's exact TABLE (896 entries extracted
 *   verbatim) and exact cadence (wrap at 672), so the displacement
 *   field is byte-identical to the original demo's
 * - White horizontal ruler lines every 64px stay perfectly straight
 *   (HOFS only shifts lines horizontally) while the verticals undulate
 * - No flicker or black lines: the table always covers 224 lines from
 *   any start phase
 *
 * @par Modules Used
 * console, dma, background, hdma
 *
 * @see hdma.h, examples/hdma/hdma_wave (high-level engine)
 */

#include <snes.h>
#include <snes/hdma.h>

/** @brief Wrap length of the start-pointer animation, in table entries.
 *  krom's exact value: his sine's quasi-period is ~25.8 lines (non-
 *  integer), so his seamless wrap needs 672 entries; the table holds
 *  224 more so any start phase has a full screen of valid lines. */
#define WAVE_WRAP     672
/** @brief Bytes per HDMA entry in 1REG_2X mode: count + 16-bit value */
#define ENTRY_BYTES   3

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
 * @brief krom's exact HDMA table, in ROM (data.asm, bank 2).
 *
 * 896 entries of [1][offset16] + terminator, extracted verbatim from
 * the original demo: entry values are round(10*sin) samples with a
 * quasi-period of ~25.8 lines. The table lives in ROM exactly like
 * krom's (his in bank 0, ours in bank 2 — hdmaSetup reads the bank
 * from the far pointer), and is never written: the animation only
 * moves the start pointer.
 */
extern u8 wavetable[];

/** @brief Current wave phase in table entries (start of the table) */
static u16 wave_phase;

/**
 * @brief Entry point — raw HDMA wave demo.
 *
 * Init order per the SDK convention: console, mode, palette, tiles,
 * tilemap, BG pointers, HDMA setup, screen on. The main loop is the
 * krom idiom: one hdmaSetup() repoint per VBlank, nothing else.
 *
 * @return Never returns (infinite loop)
 */
int main(void) {
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

    /* The table is immutable ROM; animation only moves the start pointer. */
    wave_phase = 0;
    hdmaSetup(HDMA_CHANNEL_0, HDMA_MODE_1REG_2X, HDMA_DEST_BG1HOFS,
              wavetable);
    hdmaEnable(1 << HDMA_CHANNEL_0);

    setMainScreen(TM_BG1);
    setScreenOn();

    while (1) {
        WaitForVBlank();
        /* krom's exact cadence: advance one entry per frame, wrap after
         * 672 entries. Repointing during VBlank is safe: HDMA reloads
         * its table address at the start of every frame. */
        wave_phase++;
        if (wave_phase >= WAVE_WRAP)
            wave_phase = 0;
        hdmaSetup(HDMA_CHANNEL_0, HDMA_MODE_1REG_2X, HDMA_DEST_BG1HOFS,
                  wavetable + wave_phase * ENTRY_BYTES);
    }

    return 0;
}
