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

void dmaCopyToVRAM(const void* src, u16 dest, u16 size) {
    /* Set VRAM address (word address) */
    REG_VMADDL = dest & 0xFF;
    REG_VMADDH = (dest >> 8) & 0xFF;

    /* Set VRAM increment mode: increment after high byte write */
    REG_VMAIN = 0x80;

    /* DMA channel 0 */
    /* Mode: 1 = Write to 2 registers (VMDATAL/VMDATAH alternating) */
    REG_DMAP(0) = 0x01;

    /* Destination: VMDATAL ($2118) */
    REG_BBAD(0) = 0x18;

    /* Source address (16-bit, bank $7E for WRAM) */
    REG_A1TL(0) = (u16)src & 0xFF;
    REG_A1TH(0) = ((u16)src >> 8) & 0xFF;
    REG_A1B(0) = 0x7E;

    /* Transfer size */
    REG_DASL(0) = size & 0xFF;
    REG_DASH(0) = (size >> 8) & 0xFF;

    /* Start DMA on channel 0 */
    REG_MDMAEN = 0x01;
}

void dmaFillVRAM(u16 value, u16 dest, u16 size) {
    /* For fill, we use fixed source mode */
    /* Set VRAM address */
    REG_VMADDL = dest & 0xFF;
    REG_VMADDH = (dest >> 8) & 0xFF;
    REG_VMAIN = 0x80;

    /* Store value in a temp location */
    static u16 fill_value;
    fill_value = value;

    /* DMA channel 0 - fixed source mode (bit 3 = 1) */
    REG_DMAP(0) = 0x09;  /* Fixed source, write to 2 registers */

    REG_BBAD(0) = 0x18;

    REG_A1TL(0) = (u16)&fill_value & 0xFF;
    REG_A1TH(0) = ((u16)&fill_value >> 8) & 0xFF;
    REG_A1B(0) = 0x7E;

    REG_DASL(0) = size & 0xFF;
    REG_DASH(0) = (size >> 8) & 0xFF;

    REG_MDMAEN = 0x01;
}

/*============================================================================
 * CGRAM (Palette) Transfers
 *============================================================================*/

void dmaCopyToCGRAM(const void* src, u8 dest, u16 count) {
    /* Set CGRAM address (color index, 0-255) */
    REG_CGADD = dest;

    /* DMA channel 0 */
    /* Mode: 0 = Write to 1 register */
    REG_DMAP(0) = 0x00;

    /* Destination: CGDATA ($2122) */
    REG_BBAD(0) = 0x22;

    /* Source address */
    REG_A1TL(0) = (u16)src & 0xFF;
    REG_A1TH(0) = ((u16)src >> 8) & 0xFF;
    REG_A1B(0) = 0x7E;

    /* Transfer size (2 bytes per color) */
    u16 size = count * 2;
    REG_DASL(0) = size & 0xFF;
    REG_DASH(0) = (size >> 8) & 0xFF;

    /* Start DMA */
    REG_MDMAEN = 0x01;
}

/*============================================================================
 * OAM Transfers
 *============================================================================*/

void dmaCopyToOAM(const void* src, u16 size) {
    /* Set OAM address to 0 */
    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    /* DMA channel 0 */
    REG_DMAP(0) = 0x00;

    /* Destination: OAMDATA ($2104) */
    REG_BBAD(0) = 0x04;

    /* Source address */
    REG_A1TL(0) = (u16)src & 0xFF;
    REG_A1TH(0) = ((u16)src >> 8) & 0xFF;
    REG_A1B(0) = 0x7E;

    /* Transfer size */
    REG_DASL(0) = size & 0xFF;
    REG_DASH(0) = (size >> 8) & 0xFF;

    /* Start DMA */
    REG_MDMAEN = 0x01;
}

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
