/**
 * @file dma.c
 * @brief OpenSNES DMA Implementation
 *
 * DMA (Direct Memory Access) for fast transfers to VRAM, CGRAM, and OAM.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/*============================================================================
 * VRAM Transfers
 *============================================================================*/

/* One fixed-source DMA pass: `count` copies of *src to $2118 or $2119. */
static void fill_pass(u8 vmain, u8 bbad, u16 src, u16 dest, u16 count) {
    REG_VMAIN = vmain;
    REG_VMADDL = dest & 0xFF;
    REG_VMADDH = (dest >> 8) & 0xFF;
    REG_DMAP(0) = 0x08;          /* fixed source, one register */
    REG_BBAD(0) = bbad;
    REG_A1TL(0) = src & 0xFF;
    REG_A1TH(0) = (src >> 8) & 0xFF;
    REG_A1B(0) = 0x7E;           /* bank-$00 RAM through its $7E mirror */
    REG_DASL(0) = count & 0xFF;
    REG_DASH(0) = (count >> 8) & 0xFF;
    REG_MDMAEN = 0x01;
}

void dmaFillVRAM(u16 value, u16 dest, u16 size) {
    /* The header promises a WORD fill. The old single pass used a fixed
     * source with the two-register mode ($2118 then $2119): a fixed source
     * never advances, so both registers received the LOW byte and
     * dmaFillVRAM(0x1234, ...) wrote $3434. Latent — every caller passed 0 —
     * until the API audit (2026-09-20). Two passes instead: the low bytes
     * with the address stepping on $2118 (VMAIN $00), then the high bytes
     * with it stepping on $2119 (VMAIN $80, the lib's resting value). Same
     * bus time for the same size. */
    static u8 fill_lo, fill_hi;
    u16 words = (size == 0) ? 0x8000 : (u16)((size >> 1) + (size & 1));

    fill_lo = (u8)(value & 0xFF);
    fill_hi = (u8)(value >> 8);
    fill_pass(0x00, 0x18, (u16)&fill_lo, dest, words);
    fill_pass(0x80, 0x19, (u16)&fill_hi, dest, words);
}

void dmaClearVRAM(void) {
    /* Clear all 64KB of VRAM to 0 */
    dmaFillVRAM(0, 0, 0);  /* Size 0 = 65536 bytes (full wraparound) */
}

/*============================================================================
 * CGRAM (Palette) Transfers
 *============================================================================*/

/*============================================================================
 * Generic DMA
 *============================================================================*/

void dmaTransfer(u8 channel, u8 mode, u8 srcBank, u16 srcAddr, u8 destReg, u16 size) {
    /* Validate channel */
    if (channel > 7) return;

    /* Set up DMA channel */
    REG_DMAP(channel) = mode;
    REG_BBAD(channel) = destReg;

    REG_A1TL(channel) = srcAddr & 0xFF;
    REG_A1TH(channel) = (srcAddr >> 8) & 0xFF;
    REG_A1B(channel) = srcBank;

    REG_DASL(channel) = size & 0xFF;
    REG_DASH(channel) = (size >> 8) & 0xFF;

    /* Start DMA on specified channel */
    REG_MDMAEN = (1 << channel);
}
