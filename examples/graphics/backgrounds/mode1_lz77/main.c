/**
 * Mode 1 LZ77 — Compressed tile demo using LZSS decompression
 *
 * Demonstrates loading LZ77-compressed tile data to VRAM using
 * LzssDecodeVram(). The compressed .pic file (8.5 KB) decompresses
 * to 12.5 KB of 4bpp tile data — a 31% size reduction.
 *
 * Mode 1 (16 colors, 4bpp BG1) with a static image.
 *
 * Based on PVSnesLib Mode1LZ77 example by Alekmaul.
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/video.h>
#include <snes/dma.h>
#include <snes/background.h>
#include <snes/lzss.h>

/* Graphics data */
extern u8 patterns[];           /* LZ77-compressed tile data */
extern u8 palette[], palette_end[];
extern u8 map[], map_end[];

int main(void) {
    /* Initialize console (sets up NMI handler) */
    consoleInit();

    /* Force blank for VRAM writes */
    REG_INIDISP = 0x80;

    /* Decompress tiles directly to VRAM at $4000 (LZ77 → VRAM) */
    LzssDecodeVram(patterns, 0x4000);

    /* Load palette (16 colors) */
    dmaCopyCGram(palette, 0,
                 (u16)(palette_end - palette));

    /* Load tilemap to VRAM at $0000 */
    dmaCopyVram(map, 0x0000,
                (u16)(map_end - map));

    /* Configure BG1: tilemap at $0000, tiles at $4000 */
    bgSetMapPtr(0, 0x0000, SC_32x32);
    bgSetGfxPtr(0, 0x4000);

    /* Mode 1, enable BG1 only */
    setMode(BG_MODE1, 0);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
