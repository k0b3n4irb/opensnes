#ifndef MAP32X32_H
#define MAP32X32_H

#include <snes/types.h>

/* Max scroll area (visible: 16x14 sprites = 256x224 pixels) */
#define MAX_SCROLL_WIDTH_32x32  (32*16 - 16*16)   /* 256 */
#define MAX_SCROLL_HEIGHT_32x32 (32*16 - 14*16)   /* 288 */

/* The tilemap buffer shared by both map engines: 16 KB of u16 entries in
 * far RAM ($7E:2000 and up), defined in map32x32.c. */
extern FAR u16 spritemap_buf[0x2000];

/* DMA byte_count bytes of the buffer, from byte_offset, to VRAM. */
void spritemapDma(u16 byte_offset, u16 vram_addr, u16 byte_count);

void initSpriteMap32x32(u16 len);
void drawSprite32x32(u8 x, u8 y, u16 sprite);
u16 element2sprite32x32(u8 elem);
void screenRefreshPos32x32(u8 x, u8 y, u16 address);

#endif
