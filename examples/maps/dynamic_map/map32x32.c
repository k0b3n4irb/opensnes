/*
 * DynamicMap - 32x32 sprite map engine
 *
 * Manages a 32x32 grid of 16x16 pixel "sprites" as tilemap entries.
 * Uses 8x8 tile mode (BG1_TSIZE8x8), so each 16x16 sprite = 4 tiles (2x2).
 * The tilemap uses SC_64x64 layout (4 pages of 32x32 tiles).
 *
 * All buffer access goes through assembly helpers since the spritemap
 * lives in bank $7E (beyond compiler-accessible bank $00 WRAM).
 */

#include <snes.h>
#include "map32x32.h"

/* Assembly helpers */
extern void smapWrite(u16 byte_offset, u16 value);
extern u16 smapRead(u16 byte_offset);
extern void smapClear(u16 byte_count);
extern void smapDma(u16 byte_offset, u16 vram_addr, u16 byte_count);
extern void sramDma(u16 byte_offset, u16 vram_addr, u16 byte_count);

static u16 map32_len;

void initSpriteMap32x32(u16 len) {
    map32_len = len;
    smapClear(len);
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
 * Row stride = 64 u16 entries = 128 bytes.
 */
static void drawSpriteRaw32x32(u8 x, u8 y, u16 sprite) {
    u16 i = (u16)(64 * y + x * 2) * 2; /* byte offset (each u16 = 2 bytes) */
    smapWrite(i, sprite);
    smapWrite(i + 2, sprite + 1);
    smapWrite(i + 64, sprite + 2);   /* 32 entries down = 64 bytes */
    smapWrite(i + 66, sprite + 3);
}

static u16 getSpriteRaw32x32(u8 x, u8 y) {
    u16 i = (u16)(64 * y + x * 2) * 2;
    return smapRead(i);
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

u16 getSprite32x32(u8 x, u8 y) {
    if (x < 16 && y < 16)
        return getSpriteRaw32x32(x, y);
    else if (x < 32 && y < 16)
        return getSpriteRaw32x32(x - 16, y + 16);
    else if (x < 16 && y < 32)
        return getSpriteRaw32x32(x, y + 16);
    else if (x < 32 && y < 32)
        return getSpriteRaw32x32(x - 16, y + 32);
    return 0;
}

u16 element2sprite32x32(u8 elem) {
    return (u16)elem * 4;   /* 4 tiles per sprite in 8x8 mode */
}

u16 calculateSpriteIndex32x32(u8 elem) {
    return (u16)elem * 256; /* 256 bytes per sprite (8bpp, 4 tiles × 64 bytes) */
}

u16 calculateSpritesLength32x32(u16 number_of_sprites) {
    return 256 * number_of_sprites;
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
    smapDma(offset * 2, address + offset, 256);
}

void updateSprite32x32(u16 vram_addr, u16 elem) {
    u16 src_off = calculateSpriteIndex32x32(elem);
    u16 dst_vram = vram_addr + element2sprite32x32(elem) * 32;
    sramDma(src_off, dst_vram, 256);
}
