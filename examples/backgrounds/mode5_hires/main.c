/**
 * @file main.c
 * @brief Hi-res text — BG Mode 5 + interlace (512 horizontal pixels)
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s InterlaceFont demo (PeterLemon/SNES,
 * PPU/Interlace/InterlaceFont). BG Mode 5 renders 512 horizontal pixels
 * with 16x8 tiles; SETINI bit 0 adds interlace for 448 vertical lines.
 * The text page is original art (a procedurally rendered 512x256 text
 * image with 1-pixel-column test bands), converted to Mode 5's paired
 * 8x8 tile layout.
 *
 * @par SNES Concepts
 * - BG Mode 5: 512px horizontal, 16x8 tiles (stored as 8x8 pairs)
 * - SETINI screen interlace via videoSetInterlace() (write-only shadow)
 * - The Mode 5 trap: content displays through MAIN AND SUB screen —
 *   both setMainScreen() and setSubScreen() are required
 *
 * @par What to Observe
 * Crisp 8px-wide glyphs and 1px vertical stripe bands that a 256-wide
 * mode cannot represent (they'd alias to solid gray).
 *
 * @par Modules Used
 * console, dma, background
 *
 * @see https://github.com/PeterLemon/SNES — original demo
 * @see video.h (videoSetInterlace and the SETINI shadow)
 */

#include <snes.h>

/** @brief 410 deduplicated 8x8 subtiles (Mode 5 pairs, original art) */
extern u8 text_pic[], text_pic_end[];
/** @brief 32x32 tilemap of 16x8 entries — 32 entries x 16px = the full 512 */
extern u8 text_map[], text_map_end[];
/** @brief 4 colors: black, white, cyan, gold */
extern u8 text_pal[], text_pal_end[];

/** @brief krom's VRAM layout: map at word $4000, tiles at word $8000 */
#define VRAM_MAP  0x4000
#define VRAM_GFX  0x8000

int main(void) {
    consoleInit();

    /* krom: BGMODE = %00001101 — Mode 5, priority bit, BG1 16x8 tiles */
    setMode(BG_MODE5, 0x08);

    /* Screen interlace (SETINI bit 0) through the lib's write-only
     * shadow — 448 visible lines when paired with Mode 5. */
    videoSetInterlace(1);

    bgSetGfxPtr(0, VRAM_GFX);
    bgSetMapPtr(0, VRAM_MAP, SC_32x64);

    dmaCopyCGram(text_pal, 0, (u16)(text_pal_end - text_pal));
    dmaCopyVram(text_map, VRAM_MAP, (u16)(text_map_end - text_map));
    dmaCopyVram(text_pic, VRAM_GFX, (u16)(text_pic_end - text_pic));

    bgSetScroll(0, 0, 0);

    /* THE Mode 5 trap: hi-res content renders through both screens.
     * Main-only shows the even pixel columns; sub fills the odd ones. */
    setMainScreen(LAYER_BG1);
    setSubScreen(LAYER_BG1);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
