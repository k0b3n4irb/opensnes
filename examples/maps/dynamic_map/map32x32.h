#ifndef MAP32X32_H
#define MAP32X32_H

#include <snes/types.h>

/* Max scroll area (visible: 16x14 sprites = 256x224 pixels) */
#define MAX_SCROLL_WIDTH_32x32  (32*16 - 16*16)   /* 256 */
#define MAX_SCROLL_HEIGHT_32x32 (32*16 - 14*16)   /* 288 */

void initSpriteMap32x32(u16 len);
void drawSprite32x32(u8 x, u8 y, u16 sprite);
u16 getSprite32x32(u8 x, u8 y);
u16 element2sprite32x32(u8 elem);
u16 calculateSpriteIndex32x32(u8 elem);
u16 calculateSpritesLength32x32(u16 number_of_sprites);
void screenRefreshPos32x32(u8 x, u8 y, u16 address);
void updateSprite32x32(u16 vram_addr, u16 elem);

#endif
