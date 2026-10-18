/*
 * DynamicMap - 32x32 sprite map engine
 *
 * Manages a 32x32 grid of 16x16 pixel "sprites" as tilemap entries.
 * Uses 8x8 tile mode (BG1_TSIZE8x8), so each 16x16 sprite = 4 tiles (2x2).
 * The tilemap uses SC_64x64 layout (4 pages of 32x32 tiles).
 *
 * The map is built in spritemap_buf, a FAR buffer: 16 KB does not fit in
 * the 8 KB of plain C RAM, so it lives at $7E:2000 and up, read and
 * written by C like any array.
 */

#include <snes.h>
#include "map32x32.h"

FAR u16 spritemap_buf[0x2000];

void spritemapDma(u16 byte_offset, u16 vram_addr, u16 byte_count) {
    dmaCopyVram((const u8 *)spritemap_buf + byte_offset, vram_addr, byte_count);
}

void initSpriteMap32x32(u16 len) {
    u16 i;
    for (i = 0; i < len / 2; i++)
        spritemap_buf[i] = 0;
}

/*
 * Draw a sprite on the raw 32x32 tilemap.
 * The map is 4 pages of 32x32 tiles, addressed as:
 *   Top-left:     y=0-15,  x=0-15
 *   Top-right:    y=16-31, x=0-15
 *   Bottom-left:  y=32-47, x=0-15
 *   Bottom-right: y=48-63, x=0-15
 *
 * Each 16x16 sprite occupies 4 tiles (2x2 in 8x8 mode).
 * Row stride = 32 u16 entries; a sprite's two rows are 32 entries apart.
 */
static void drawSpriteRaw32x32(u8 x, u8 y, u16 sprite) {
    u16 i = (u16)(64 * y + x * 2);     /* entry index */
    spritemap_buf[i] = sprite;
    spritemap_buf[i + 1] = sprite + 1;
    spritemap_buf[i + 32] = sprite + 2;   /* the row below */
    spritemap_buf[i + 33] = sprite + 3;
}

void drawSprite32x32(u8 x, u8 y, u16 sprite) {
    if (x < 16 && y < 16)
        drawSpriteRaw32x32(x, y, sprite);
    else if (x < 32 && y < 16)
        drawSpriteRaw32x32(x - 16, y + 16, sprite);
    else if (x < 16 && y < 32)
        drawSpriteRaw32x32(x, y + 16, sprite);
    else if (x < 32 && y < 32)
        drawSpriteRaw32x32(x - 16, y + 32, sprite);
}

u16 element2sprite32x32(u8 elem) {
    return (u16)elem * 4;   /* 4 tiles per sprite in 8x8 mode */
}

void screenRefreshPos32x32(u8 x, u8 y, u16 address) {
    u16 offset = 0;
    if (x < 16 && y < 16)
        offset = x + (u16)y * 64;
    else if (x < 32 && y < 16)
        offset = 32 * 32 + (x - 16) + (u16)y * 64;
    else if (x < 16 && y < 32)
        offset = 32 * 32 * 2 + x + (u16)(y - 16) * 64;
    else if (x < 32 && y < 32)
        offset = 32 * 32 * 3 + (x - 16) + (u16)(y - 16) * 64;

    /* DMA 256 bytes from spritemap buffer to VRAM */
    spritemapDma(offset * 2, address + offset, 256);
}

