#ifndef MAP64X64_H
#define MAP64X64_H

#include <snes/types.h>

/* Max scroll area (visible: 16x14 sprites = 256x224 pixels) */
#define MAX_SCROLL_WIDTH_64x64  (64*16 - 16*16)   /* 768 */
#define MAX_SCROLL_HEIGHT_64x64 (64*16 - 14*16)   /* 800 */

void initSpriteMap64x64(u16 len);
void drawSprite64x64(u8 x, u8 y, u16 sprite);
u16 element2sprite64x64(u8 elem);
void screenRefreshPos64x64(u8 x, u8 y, u16 address);

#endif
