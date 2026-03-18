/**
 * @file main.c
 * @brief LZ77-compressed tile loading with LZSS decompression to VRAM
 * @ingroup examples
 *
 * Demonstrates ROM space savings by storing tile data in LZ77-compressed
 * form and decompressing directly into VRAM at load time using
 * LzssDecodeVram(). The compressed .pic file occupies ~8.5 KB in ROM but
 * expands to ~12.5 KB of 4bpp tile data in VRAM, achieving a 31% size
 * reduction. The tilemap and palette are stored uncompressed and loaded
 * via standard DMA.
 *
 * The gfx4snes tool produces LZ77-compressed output when invoked with the
 * -z flag. LzssDecodeVram() handles the VRAM write timing internally.
 *
 * Based on the PVSnesLib "Mode1LZ77" example by Alekmaul.
 *
 * @par SNES Concepts
 * - LZSS/LZ77 decompression: LzssDecodeVram() streams compressed data to VRAM word-by-word
 * - gfx4snes -z flag enables LZ77 compression for tile data at build time
 * - Mode 1 BG1 at 4bpp (16 colors) with tilemap at $0000, tiles at $4000
 * - Force blank via REG_INIDISP during decompression (slower than DMA, needs full blanking)
 * - Palette loaded uncompressed via dmaCopyCGram(); only tile data benefits from compression
 *
 * @par What to Observe
 * - A static full-screen image identical in appearance to the uncompressed Mode 1 example
 * - The ROM is smaller due to LZ77-compressed tile data
 * - Startup may be slightly slower than DMA (decompression is CPU-driven, not DMA)
 *
 * @par Modules Used
 * console, lzss, background, sprite
 *
 * @see lzss.h, background.h, dma.h, video.h
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
