/**
 * @file sprite.c
 * @brief OpenSNES Sprite (OAM) Management Implementation
 *
 * Sprite buffer management and DMA to OAM.
 *
 * Attribution:
 *   Based on PVSnesLib sprite handling by Alekmaul
 *   License: zlib (compatible with MIT)
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/*============================================================================
 * OAM Buffer
 *============================================================================*/

/**
 * OAM buffer structure:
 * - Bytes 0-511: 4 bytes per sprite (128 sprites)
 *   - Byte 0: X position (low 8 bits)
 *   - Byte 1: Y position
 *   - Byte 2: Tile number (low 8 bits)
 *   - Byte 3: Attributes (vhoopppc - flip, priority, palette, tile high)
 * - Bytes 512-543: 2 bits per sprite (X high bit, size select)
 */
static u8 oam_buffer[544];

/*============================================================================
 * Initialization
 *============================================================================*/

void oamInit(void) {
    oamInitEx(OBJ_SIZE8_L16, 0);
}

void oamInitEx(u8 size, u8 tileBase) {
    /* Set sprite size and tile base address */
    REG_OBJSEL = (size << 5) | (tileBase & 0x07);

    /* Clear OAM buffer - hide all sprites */
    oamClear();
}

void oamInitGfxSet(u8 *tileSource, u16 tileSize, u8 *tilePalette,
                   u16 paletteSize, u8 paletteEntry, u16 vramAddr, u8 oamSize) {
    /* Load sprite tiles to VRAM */
    dmaCopyVram(tileSource, vramAddr, tileSize);

    /* Load sprite palette to CGRAM (sprites use colors 128-255) */
    /* Each sprite palette is 16 colors = 32 bytes, starting at color 128 */
    u16 palOffset = 128 + (paletteEntry * 16);
    dmaCopyCGram(tilePalette, palOffset, paletteSize);

    /* Calculate tile base from VRAM address (divide by 0x2000) */
    u8 tileBase = (vramAddr >> 13) & 0x07;

    /* Initialize OAM with size and tile base */
    oamInitEx(oamSize, tileBase);
}

/*============================================================================
 * Sprite Properties
 *============================================================================*/

void oamSet(u8 id, u16 x, u8 y, u16 tile, u8 palette, u8 priority, u8 flags) {
    if (id >= MAX_SPRITES) return;

    u16 offset = id << 2;  /* id * 4 */

    /* Set position and tile */
    oam_buffer[offset + 0] = (u8)(x & 0xFF);
    oam_buffer[offset + 1] = y;
    oam_buffer[offset + 2] = (u8)(tile & 0xFF);

    /* Set attributes: vhoopppc */
    /* v = vertical flip (bit 7 of flags) */
    /* h = horizontal flip (bit 6 of flags) */
    /* oo = priority (0-3) */
    /* ppp = palette (0-7) */
    /* c = tile high bit */
    oam_buffer[offset + 3] = (flags & 0xC0) |
                              ((priority & 0x03) << 4) |
                              ((palette & 0x07) << 1) |
                              ((tile >> 8) & 0x01);

    /* Set high table bits (X high bit, size) */
    u8 ext_offset = 512 + (id >> 2);
    u8 bit_offset = (id & 0x03) << 1;
    u8 mask = ~(0x03 << bit_offset);

    /* X high bit (bit 0), size (bit 1 - default to small) */
    u8 ext_bits = (x >> 8) & 0x01;
    oam_buffer[ext_offset] = (oam_buffer[ext_offset] & mask) | (ext_bits << bit_offset);
}

void oamSetX(u8 id, u16 x) {
    if (id >= MAX_SPRITES) return;

    u16 offset = id << 2;
    oam_buffer[offset + 0] = (u8)(x & 0xFF);

    /* Update high bit */
    u8 ext_offset = 512 + (id >> 2);
    u8 bit_offset = (id & 0x03) << 1;
    u8 mask = ~(0x01 << bit_offset);
    u8 ext_bits = (x >> 8) & 0x01;
    oam_buffer[ext_offset] = (oam_buffer[ext_offset] & mask) | (ext_bits << bit_offset);
}

void oamSetY(u8 id, u8 y) {
    if (id >= MAX_SPRITES) return;
    oam_buffer[(id << 2) + 1] = y;
}

void oamSetXY(u8 id, u16 x, u8 y) {
    oamSetX(id, x);
    oamSetY(id, y);
}

void oamSetTile(u8 id, u16 tile) {
    if (id >= MAX_SPRITES) return;

    u16 offset = id << 2;
    oam_buffer[offset + 2] = (u8)(tile & 0xFF);

    /* Update tile high bit in attributes */
    oam_buffer[offset + 3] = (oam_buffer[offset + 3] & 0xFE) | ((tile >> 8) & 0x01);
}

void oamSetVisible(u8 id, u8 visible) {
    if (visible) {
        /* Visibility is controlled by Y position - no specific "show" */
        /* User should set proper Y position */
    } else {
        oamHide(id);
    }
}

void oamHide(u8 id) {
    if (id >= MAX_SPRITES) return;
    /* Y = 240 is off-screen (below visible area) */
    oam_buffer[(id << 2) + 1] = OBJ_HIDE_Y;
}

void oamSetSize(u8 id, u8 large) {
    if (id >= MAX_SPRITES) return;

    u8 ext_offset = 512 + (id >> 2);
    u8 bit_offset = ((id & 0x03) << 1) + 1;  /* Size is bit 1 of the pair */
    u8 mask = ~(0x01 << bit_offset);

    if (large) {
        oam_buffer[ext_offset] |= (0x01 << bit_offset);
    } else {
        oam_buffer[ext_offset] &= mask;
    }
}

void oamSetEx(u8 id, u8 size, u8 visible) {
    if (id >= MAX_SPRITES) return;

    /* Set size */
    oamSetSize(id, size);

    /* Set visibility */
    if (!visible) {
        oamHide(id);
    }
}

/*============================================================================
 * OAM Update
 *============================================================================*/

void oamUpdate(void) {
    /* Set OAM address to 0 */
    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    /* DMA transfer OAM buffer to hardware OAM */
    /* Using DMA channel 0 */

    /* DMA mode: CPU->PPU, auto-increment, 1 register write */
    REG_DMAP(0) = 0x00;

    /* Destination: OAMDATA ($2104) */
    REG_BBAD(0) = 0x04;

    /* Source address */
    REG_A1TL(0) = (u16)oam_buffer & 0xFF;
    REG_A1TH(0) = ((u16)oam_buffer >> 8) & 0xFF;
    REG_A1B(0) = 0x7E;  /* Bank $7E (Work RAM) */

    /* Transfer size: 544 bytes */
    REG_DASL(0) = 544 & 0xFF;
    REG_DASH(0) = (544 >> 8) & 0xFF;

    /* Start DMA */
    REG_MDMAEN = 0x01;
}

void oamClear(void) {
    u8 i;

    /* Hide all sprites by setting Y = 240 */
    for (i = 0; i < 128; i++) {
        u16 offset = i << 2;
        oam_buffer[offset + 0] = 0;
        oam_buffer[offset + 1] = OBJ_HIDE_Y;
        oam_buffer[offset + 2] = 0;
        oam_buffer[offset + 3] = 0;
    }

    /* Clear extension table */
    for (i = 0; i < 32; i++) {
        oam_buffer[512 + i] = 0;
    }
}
