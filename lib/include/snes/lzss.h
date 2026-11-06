/**
 * @file lzss.h
 * @brief LZSS (LZ77) Decompression for SNES
 *
 * LZ77 decompression into VRAM (lzssDecodeVram, under forced blank) or
 * into RAM of any bank (lzssDecodeRam, screen on). Used for compressed
 * tile/tilemap data written by `opensnes-tileset convert --lz`,
 * `opensnes-sprite sheet --lz` or external LZ77 tools.
 *
 * ## LZ77 Format
 *
 * - Tag byte: 0x10 (lzssDecodeVram accepts 0x1x, lzssDecodeRam exactly 0x10)
 * - 3-byte little-endian uncompressed data length
 * - Flag bytes: 8 bits each (1=reference, 0=literal)
 * - References: 12-bit distance, 4-bit length (min 3 bytes)
 *
 * ## Usage
 *
 * @code
 * extern u8 compressed_tiles[], compressed_tiles_end[];
 *
 * // Decompress to VRAM during force blank
 * REG_INIDISP = 0x80;  // Force blank (required for VRAM writes)
 * lzssDecodeVram(compressed_tiles, 0x0000);
 * REG_INIDISP = 0x0F;  // Screen on, full brightness
 * @endcode
 *
 * @warning VRAM writes must happen during VBlank or force blank.
 *          lzssDecodeVram disables interrupts internally, so call it
 *          during force blank for safety.
 *
 * @author OpenSNES Team (ported from PVSnesLib by Alekmaul)
 * @copyright zlib License
 */

#ifndef OPENSNES_LZSS_H
#define OPENSNES_LZSS_H

#include <snes/types.h>

/* Removed on 2026-10-05 (1.0 plan, lot C): LzssDecodeVram.
 * The replacements are in docs/UPGRADING.md; `make check-upgrade` names them. */

/**
 * @brief Decompress LZ77 data directly to VRAM
 *
 * Reads LZ77-compressed data from ROM/RAM and writes decompressed
 * output directly to VRAM via PPU registers. It masks IRQs (`sei`) for the
 * duration; it does NOT mask NMI — `sei` cannot — so call it under forced
 * blank, where the NMI handler's own VRAM traffic is not a concern.
 *
 * @param source Pointer to LZ77-compressed data (tag byte 0x10)
 * @param address VRAM word address to write decompressed data
 *
 * @note The source data must start with the LZ77 tag byte (0x10).
 *       If the tag doesn't match, the function returns immediately.
 * @note IRQs are masked during decompression and restored after; NMI is not.
 * @note The source may be in any bank (the bank byte of the pointer is
 *       honoured since 2026-09-20). VMAIN is left at $00 on return.
 */
void lzssDecodeVram(const u8 *source, u16 address);

/**
 * @brief Decompress LZ77 data into RAM, screen on, interrupts left alone
 *
 * Expands an LZ77 stream (the format above, tag byte exactly 0x10) from
 * ROM or RAM of any bank into RAM of any bank: a FAR buffer in bank $7E,
 * the 64 KB of bank $7F, the low 8 KB. It writes no PPU register and does
 * not mask interrupts, so it runs with the screen on while the NMI keeps
 * its frame; the decoded block then goes to VRAM by DMA (dmaCopyVram, or
 * the VRAM queue) like any RAM buffer.
 *
 * Back references are copied with MVN when the destination is in bank $7E
 * or $7F, and byte by byte through the data bank for any other bank. An
 * overlapping reference (distance shorter than its length, distance 1 for
 * a run of one byte) gives the LZ77 result, each byte read after the one
 * before it is written.
 *
 * Cost, measured with luna profile on examples/memory/lzss_ram (master
 * cycles per output byte, SlowROM / FastROM): 104 / 88 for literals only,
 * 101 / 87 for long references, 163 / 139 for a 2 bpp tile block (32768
 * bytes from 17856). 376 bytes of ROM, linked only by a ROM that calls it.
 *
 * @code
 * extern const u8 tiles_lz[];
 * static FAR u8 tiles[4096];
 *
 * u16 n = lzssDecodeRam(tiles_lz, tiles);        // screen on, any time
 * WaitForVBlank();
 * dmaCopyVram(tiles, 0x2000, n);                 // to VRAM in VBlank
 *
 * u16 m = lzssDecodeRam(tiles_lz, (u8 FAR *)0x7F0000UL);   // bank $7F
 * @endcode
 *
 * @param source LZ77 stream (tag 0x10, 3-byte length). It must not cross
 *               the end of its bank, as no asset section does.
 * @param dest   First byte written. The whole output must fit before the
 *               end of that bank.
 * @return The number of bytes written (the stream's length), or 0 with
 *         nothing written when the tag is not 0x10, the length is 0 or
 *         above 65535, or dest + length passes the end of the bank.
 *
 * @note Even a corrupt stream writes only from dest to dest + length - 1,
 *       in order, and ends: a reference past the end is cut there, and one
 *       reaching back before dest copies whatever that RAM holds.
 * @note Uses the caller-saved compiler slots tcc__r0 to tcc__r5 of the
 *       main thread; the NMI runs on its own direct page.
 */
u16 lzssDecodeRam(const u8 *source, u8 FAR *dest);

#endif /* OPENSNES_LZSS_H */
