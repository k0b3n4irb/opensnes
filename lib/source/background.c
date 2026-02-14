/**
 * @file background.c
 * @brief OpenSNES Background Functions Implementation
 *
 * Functions for scrolling and configuring background layers.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/*============================================================================
 * Shadow Registers
 *
 * PPU registers $210B and $210C are write-only, so we maintain shadow copies
 * to allow read-modify-write operations.
 *============================================================================*/

/* Shadow registers for read-modify-write operations.
 * Zero-initialized by C standard (uninitialized statics are zero).
 */
static u8 bg12nba_shadow;  /* Shadow for REG_BG12NBA ($210B) */
static u8 bg34nba_shadow;  /* Shadow for REG_BG34NBA ($210C) */

static u16 bg_scroll_x[4]; /* Shadow for BG1-4 horizontal scroll */
static u16 bg_scroll_y[4]; /* Shadow for BG1-4 vertical scroll */

/*============================================================================
 * Scrolling Functions
 *============================================================================*/

void bgSetScroll(u8 bg, u16 x, u16 y) {
    bg_scroll_x[bg] = x;
    bg_scroll_y[bg] = y;
    switch (bg) {
        case 0:
            REG_BG1HOFS = x & 0xFF;
            REG_BG1HOFS = (x >> 8) & 0xFF;
            REG_BG1VOFS = y & 0xFF;
            REG_BG1VOFS = (y >> 8) & 0xFF;
            break;
        case 1:
            REG_BG2HOFS = x & 0xFF;
            REG_BG2HOFS = (x >> 8) & 0xFF;
            REG_BG2VOFS = y & 0xFF;
            REG_BG2VOFS = (y >> 8) & 0xFF;
            break;
        case 2:
            REG_BG3HOFS = x & 0xFF;
            REG_BG3HOFS = (x >> 8) & 0xFF;
            REG_BG3VOFS = y & 0xFF;
            REG_BG3VOFS = (y >> 8) & 0xFF;
            break;
        case 3:
            REG_BG4HOFS = x & 0xFF;
            REG_BG4HOFS = (x >> 8) & 0xFF;
            REG_BG4VOFS = y & 0xFF;
            REG_BG4VOFS = (y >> 8) & 0xFF;
            break;
    }
}

void bgSetScrollX(u8 bg, u16 x) {
    bg_scroll_x[bg] = x;
    switch (bg) {
        case 0:
            REG_BG1HOFS = x & 0xFF;
            REG_BG1HOFS = (x >> 8) & 0xFF;
            break;
        case 1:
            REG_BG2HOFS = x & 0xFF;
            REG_BG2HOFS = (x >> 8) & 0xFF;
            break;
        case 2:
            REG_BG3HOFS = x & 0xFF;
            REG_BG3HOFS = (x >> 8) & 0xFF;
            break;
        case 3:
            REG_BG4HOFS = x & 0xFF;
            REG_BG4HOFS = (x >> 8) & 0xFF;
            break;
    }
}

void bgSetScrollY(u8 bg, u16 y) {
    bg_scroll_y[bg] = y;
    switch (bg) {
        case 0:
            REG_BG1VOFS = y & 0xFF;
            REG_BG1VOFS = (y >> 8) & 0xFF;
            break;
        case 1:
            REG_BG2VOFS = y & 0xFF;
            REG_BG2VOFS = (y >> 8) & 0xFF;
            break;
        case 2:
            REG_BG3VOFS = y & 0xFF;
            REG_BG3VOFS = (y >> 8) & 0xFF;
            break;
        case 3:
            REG_BG4VOFS = y & 0xFF;
            REG_BG4VOFS = (y >> 8) & 0xFF;
            break;
    }
}

u16 bgGetScrollX(u8 bg) {
    return bg_scroll_x[bg];
}

u16 bgGetScrollY(u8 bg) {
    return bg_scroll_y[bg];
}

/*============================================================================
 * Memory Configuration Functions
 *============================================================================*/

void bgSetMapPtr(u8 bg, u16 vramAddr, u8 mapSize) {
    /* BGnSC format: aaaaaayx
     * aaaaaa = tilemap address / 0x400 (bits 2-7)
     * y = vertical size (0=32, 1=64)
     * x = horizontal size (0=32, 1=64)
     */
    u8 value = ((vramAddr >> 8) & 0xFC) | (mapSize & 0x03);

    switch (bg) {
        case 0: REG_BG1SC = value; break;
        case 1: REG_BG2SC = value; break;
        case 2: REG_BG3SC = value; break;
        case 3: REG_BG4SC = value; break;
    }
}

void bgSetGfxPtr(u8 bg, u16 vramAddr) {
    /* BG12NBA format: bbbbaaaa
     * aaaa = BG1 character base / 0x1000 (bits 0-3)
     * bbbb = BG2 character base / 0x1000 (bits 4-7)
     *
     * BG34NBA format: bbbbaaaa
     * aaaa = BG3 character base / 0x1000 (bits 0-3)
     * bbbb = BG4 character base / 0x1000 (bits 4-7)
     *
     * NOTE: These registers are write-only, so we use shadow copies.
     */
    u8 nibble = (vramAddr >> 12) & 0x0F;

    switch (bg) {
        case 0:
            bg12nba_shadow = (bg12nba_shadow & 0xF0) | nibble;
            REG_BG12NBA = bg12nba_shadow;
            break;
        case 1:
            bg12nba_shadow = (bg12nba_shadow & 0x0F) | (nibble << 4);
            REG_BG12NBA = bg12nba_shadow;
            break;
        case 2:
            bg34nba_shadow = (bg34nba_shadow & 0xF0) | nibble;
            REG_BG34NBA = bg34nba_shadow;
            break;
        case 3:
            bg34nba_shadow = (bg34nba_shadow & 0x0F) | (nibble << 4);
            REG_BG34NBA = bg34nba_shadow;
            break;
    }
}

/*============================================================================
 * Initialization
 *============================================================================*/

void bgInit(u8 bg) {
    /* Reset scroll position to (0, 0) */
    bgSetScroll(bg, 0, 0);
}

/*============================================================================
 * Combined Initialization (PVSnesLib compatible)
 *============================================================================*/

void bgInitTileSet(u8 bgNumber, u8 *tileSource, u8 *tilePalette,
                   u8 paletteEntry, u16 tileSize, u16 paletteSize,
                   u16 colorMode, u16 vramAddr) {
    /* Calculate palette color index based on color mode and entry
     * CGRAM address register ($2121) takes a color index (0-255), not byte offset.
     * Each color is 2 bytes in CGRAM.
     */
    u16 colorIndex;

    if (colorMode == BG_4COLORS || colorMode == BG_4COLORS0) {
        /* 4 colors per palette, color index = entry * 4 */
        colorIndex = paletteEntry * 4;
    } else if (colorMode == BG_16COLORS) {
        /* 16 colors per palette, color index = entry * 16 */
        colorIndex = paletteEntry * 16;
    } else {
        /* 256 colors - always starts at color 0 */
        colorIndex = 0;
    }

    /* Copy tile graphics to VRAM */
    dmaCopyVram(tileSource, vramAddr, tileSize);

    /* Copy palette to CGRAM (dmaCopyCGram takes color index, not byte offset) */
    dmaCopyCGram(tilePalette, colorIndex, paletteSize);

    /* Set the tile graphics pointer for this background */
    bgSetGfxPtr(bgNumber, vramAddr);
}

void bgInitTileSetData(u8 bgNumber, u8 *tileSource, u16 tileSize, u16 vramAddr) {
    /* Copy tile graphics to VRAM */
    dmaCopyVram(tileSource, vramAddr, tileSize);

    /* Set the tile graphics pointer (unless bgNumber is 0xFF) */
    if (bgNumber != 0xFF) {
        bgSetGfxPtr(bgNumber, vramAddr);
    }
}
