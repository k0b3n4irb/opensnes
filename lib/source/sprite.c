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
 *
 * Uses oamMemory defined in crt0.asm at $7E:0300 to ensure all code
 * (C library, assembly helpers, NMI handler) uses the same buffer.
 */
/* oamMemory[] and oam_update_flag are declared in <snes/system.h> (via <snes.h>) */
extern u8 oam_max_id; /* Defined in crt0.asm - highest sprite ID written */
#define oam_buffer oamMemory

/* Update oam_max_id tracking (inline to avoid function call overhead) */
#define OAM_TRACK_MAX(id) do { if ((id) > oam_max_id) oam_max_id = (id); } while(0)

/*============================================================================
 * Initialization
 *============================================================================*/

void oamInit(void) {
    oamInitEx(OBJ_SIZE8_L16, 0);
}

void oamInitEx(u16 size, u16 tileBase) {
    /* All parameters u16 to avoid calling convention issues */
    /* Set sprite size and tile base address */
    REG_OBJSEL = (u8)((size << 5) | (tileBase & 0x07));

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

/* OAM high table bit helpers — inline to avoid static const bank spill.
 * Each byte covers 4 sprites: 2 bits per sprite (bit 0 = X high, bit 1 = size).
 * slot = id & 3 gives the sprite's position within the byte (0-3).
 * Uses conditionals instead of variable shifts to avoid a codegen bug in the
 * 65816 backend's pha/tax/pla shift loop (wrong stack offset after pha). */
#define OAM_XHI_BIT(slot)  ((u8)((slot) == 0 ? 0x01 : (slot) == 1 ? 0x04 : (slot) == 2 ? 0x10 : 0x40))
#define OAM_SIZE_BIT(slot) ((u8)((slot) == 0 ? 0x02 : (slot) == 1 ? 0x08 : (slot) == 2 ? 0x20 : 0x80))

/* oamSet() is now implemented in assembly (sprite_oamset.asm) for performance.
 * The C version had framesize=158 (~100+ cycles overhead per call).
 * The assembly version eliminates all frame allocation. */

void oamSetX(u8 id, u16 x) {
    if (id >= MAX_SPRITES) return;

    u16 offset = id << 2;
    oam_buffer[offset + 0] = (u8)(x & 0xFF);

    /* Update X high bit in extension table */
    u8 ext_offset = 512 + (id >> 2);
    u8 slot = id & 0x03;

    if (x & 0x100) {
        oam_buffer[ext_offset] = (oam_buffer[ext_offset] & ~OAM_XHI_BIT(slot)) | OAM_XHI_BIT(slot);
    } else {
        oam_buffer[ext_offset] &= ~OAM_XHI_BIT(slot);
    }

    OAM_TRACK_MAX(id);
    oam_update_flag = 1;
}

void oamSetY(u8 id, u8 y) {
    if (id >= MAX_SPRITES) return;
    oam_buffer[(id << 2) + 1] = y;
    OAM_TRACK_MAX(id);
    oam_update_flag = 1;
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

    OAM_TRACK_MAX(id);
    oam_update_flag = 1;
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
    /* Y=240 + X=256 (high bit set) to hide off-screen.
     * Y=240 alone wraps for sprites > 16px tall.
     */
    oam_buffer[(id << 2) + 0] = 0;          /* X low = 0 */
    oam_buffer[(id << 2) + 1] = OBJ_HIDE_Y; /* Y = 240 */

    /* Set X high bit in extension table */
    u16 ext_offset = 512 + (id >> 2);
    u8 slot = id & 0x03;
    oam_buffer[ext_offset] |= OAM_XHI_BIT(slot);

    OAM_TRACK_MAX(id);
    oam_update_flag = 1;
}

void oamSetSize(u16 id, u16 large) {
    /* All parameters u16 to avoid calling convention issues */
    if (id >= MAX_SPRITES) return;

    u16 ext_offset = 512 + (id >> 2);
    u16 slot = id & 0x03;

    if (large) {
        oam_buffer[ext_offset] |= OAM_SIZE_BIT(slot);
    } else {
        oam_buffer[ext_offset] &= ~OAM_SIZE_BIT(slot);
    }

    OAM_TRACK_MAX((u8)id);
    oam_update_flag = 1;
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

    /* Hide all sprites: Y=240 + X=256 (high bit set).
     * Y=240 alone causes large sprites (32x32, 64x64) to wrap to
     * the top of the screen. Setting X bit 8 = 1 pushes them
     * off-screen to the right, safe for any sprite size.
     */
    for (i = 0; i < 128; i++) {
        u16 offset = i << 2;
        oam_buffer[offset + 0] = 0;
        oam_buffer[offset + 1] = OBJ_HIDE_Y;
        oam_buffer[offset + 2] = 0;
        oam_buffer[offset + 3] = 0;
    }

    /* Extension table: set X high bit (bit 0 of each 2-bit pair) for all sprites.
     * Each byte covers 4 sprites: bits 0,2,4,6 = X high; bits 1,3,5,7 = size.
     * Pattern 0x55 = 01010101 = all X high bits set, all size bits clear.
     */
    for (i = 0; i < 32; i++) {
        oam_buffer[512 + i] = 0x55;
    }

    oam_max_id = 127;  /* Force full OAM DMA to clear all sprites */
    oam_update_flag = 1;
}

/*============================================================================
 * Metasprite Functions
 *============================================================================*/

u8 oamDrawMeta(u8 startId, s16 x, s16 y, const MetaspriteItem *meta,
               u16 baseTile, u8 basePalette, u8 size) {
    u8 count = 0;
    u8 id = startId;

    while (meta->dx != metasprite_end && id < MAX_SPRITES) {
        /* Calculate sprite position */
        s16 sx = x + meta->dx;
        s16 sy = y + meta->dy;

        /* Skip sprites that are completely off-screen */
        if (sx > -64 && sx < 256 && sy > -64 && sy < 240) {
            /* Calculate tile number */
            u16 tile = baseTile + meta->tile;

            /* Extract attributes from metasprite item */
            u8 attr = meta->attr;
            u8 palette = (attr >> 1) & 0x07;  /* Bits 1-3 */
            u8 priority = (attr >> 4) & 0x03; /* Bits 4-5 */
            u8 flags = attr & 0xC0;           /* Bits 6-7 (flip) */

            /* Use base palette if item palette is 0 */
            if (palette == 0) {
                palette = basePalette;
            }

            /* Set sprite in OAM buffer */
            oamSet(id, (u16)sx, (u8)sy, tile, palette, priority, flags);

            /* Set sprite size */
            oamSetSize(id, size);

            id++;
            count++;
        }

        meta++;
    }

    return count;
}

u8 oamDrawMetaFlip(u8 startId, s16 x, s16 y, const MetaspriteItem *meta,
                   u16 baseTile, u8 basePalette, u8 size,
                   u8 flipX, u8 flipY, u8 width, u8 height) {
    u8 count = 0;
    u8 id = startId;

    /* Sprite size for offset calculations (depends on size mode) */
    /* For now, assume 16x16 when large, 8x8 when small */
    u8 spriteSize = size ? 16 : 8;

    while (meta->dx != metasprite_end && id < MAX_SPRITES) {
        s16 dx = meta->dx;
        s16 dy = meta->dy;
        u8 flags = meta->attr & 0xC0;

        /* Apply horizontal flip */
        if (flipX) {
            dx = width - dx - spriteSize;
            flags ^= OBJ_FLIPX;
        }

        /* Apply vertical flip */
        if (flipY) {
            dy = height - dy - spriteSize;
            flags ^= OBJ_FLIPY;
        }

        /* Calculate final position */
        s16 sx = x + dx;
        s16 sy = y + dy;

        /* Skip sprites that are completely off-screen */
        if (sx > -64 && sx < 256 && sy > -64 && sy < 240) {
            u16 tile = baseTile + meta->tile;

            u8 attr = meta->attr;
            u8 palette = (attr >> 1) & 0x07;
            u8 priority = (attr >> 4) & 0x03;

            if (palette == 0) {
                palette = basePalette;
            }

            oamSet(id, (u16)sx, (u8)sy, tile, palette, priority, flags);
            oamSetSize(id, size);

            id++;
            count++;
        }

        meta++;
    }

    return count;
}

u8 oamDrawMetasprite(u8 startId, u16 x, u8 y, const u8 *data, u8 palette) {
    /* Legacy interface - cast data to MetaspriteItem and call oamDrawMeta */
    return oamDrawMeta(startId, (s16)x, (s16)y,
                       (const MetaspriteItem *)data,
                       0, palette, OBJ_LARGE);
}
