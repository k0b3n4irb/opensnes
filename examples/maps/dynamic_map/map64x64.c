/*
 * DynamicMap - 64x64 sprite map engine
 *
 * Manages a 64x64 grid of 16x16 pixel "sprites" as tilemap entries.
 * Uses 16x16 tile mode (BG1_TSIZE16x16), so each sprite = 1 tile.
 * The tilemap uses SC_64x64 layout (4 pages of 32x32 tiles).
 *
 * In 16x16 tile mode, sprite graphics have a 1024-byte gap between
 * the top 8 rows (128 bytes) and bottom 8 rows (128 bytes).
 */

#include <snes.h>
#include "map64x64.h"

/* Assembly helpers */
extern void smapWrite(u16 byte_offset, u16 value);
extern u16 smapRead(u16 byte_offset);
extern void smapClear(u16 byte_count);
extern void smapDma(u16 byte_offset, u16 vram_addr, u16 byte_count);
extern void sramDma(u16 byte_offset, u16 vram_addr, u16 byte_count);

static u16 map64_len;

void initSpriteMap64x64(u16 len) {
    map64_len = len;
    smapClear(len);
}

/*
 * Raw tilemap access for 64x64 map.
 * SC_64x64 = 4 pages of 32x32 tiles:
 *   Page 0: y=0-31,  x=0-31   (top-left)
 *   Page 1: y=32-63, x=0-31   (top-right)
 *   Page 2: y=64-95, x=0-31   (bottom-left)
 *   Page 3: y=96-127, x=0-31  (bottom-right)
 *
 * Each sprite = 1 tile in 16x16 mode.
 * Row stride = 32 entries.
 */
static void drawSpriteRaw64x64(u8 x, u8 y, u16 sprite) {
    u16 i = (u16)(32 * y + x) * 2; /* byte offset */
    smapWrite(i, sprite);
}

static u16 getSpriteRaw64x64(u8 x, u8 y) {
    u16 i = (u16)(32 * y + x) * 2;
    return smapRead(i);
}

void drawSprite64x64(u8 x, u8 y, u16 sprite) {
    if (x < 32 && y < 32)
        drawSpriteRaw64x64(x, y, sprite);
    else if (x < 64 && y < 32)
        drawSpriteRaw64x64(x - 32, y + 32, sprite);
    else if (x < 32 && y < 64)
        drawSpriteRaw64x64(x, y + 32, sprite);
    else if (x < 64 && y < 64)
        drawSpriteRaw64x64(x - 32, y + 64, sprite);
}

u16 getSprite64x64(u8 x, u8 y) {
    if (x < 32 && y < 32)
        return getSpriteRaw64x64(x, y);
    else if (x < 64 && y < 32)
        return getSpriteRaw64x64(x - 32, y + 32);
    else if (x < 32 && y < 64)
        return getSpriteRaw64x64(x, y + 32);
    else if (x < 64 && y < 64)
        return getSpriteRaw64x64(x - 32, y + 64);
    return 0;
}

u16 element2sprite64x64(u8 elem) {
    /* BG1_TSIZE16x16: tile index with row-of-16 wrap */
    u8 mult = elem / 8;
    return (u16)(elem * 2) + (u16)(mult * 16);
}

u16 calculateSpriteIndex64x64(u8 elem) {
    /* In 16x16 tile mode, top and bottom halves are 1024 bytes apart.
     * To prevent top overwriting an existing bottom, apply correction. */
    u16 index = (u16)elem * 128;
    u8 mult = index / 1024;
    index += (u16)1024 * mult;
    return index;
}

u16 calculateSpritesLength64x64(u16 number_of_sprites) {
    /* Last sprite's top + 1024 gap + 128 bottom bytes */
    return calculateSpriteIndex64x64(number_of_sprites - 1) + 1024 + 128;
}

void screenRefreshPos64x64(u8 x, u8 y, u16 address) {
    u16 offset = 0;
    if (x < 32 && y < 32)
        offset = x + (u16)y * 32;
    else if (x < 64 && y < 32)
        offset = 32 * 32 + (x - 32) + (u16)y * 32;
    else if (x < 32 && y < 64)
        offset = 32 * 32 * 2 + x + (u16)(y - 32) * 32;
    else if (x < 64 && y < 64)
        offset = 32 * 32 * 3 + (x - 32) + (u16)(y - 32) * 32;

    smapDma(offset * 2, address + offset, 256);
}

void updateSprite64x64(u16 vram_addr, u16 elem) {
    u16 src_off = calculateSpriteIndex64x64(elem);
    u16 vram_base = vram_addr + element2sprite64x64(elem) * 32;
    /* DMA top half (128 bytes) */
    sramDma(src_off, vram_base, 128);
    /* DMA bottom half (128 bytes, 1024 bytes later in source, 512 words later in VRAM) */
    sramDma(src_off + 1024, vram_base + 512, 128);
}
