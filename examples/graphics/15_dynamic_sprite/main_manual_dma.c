/**
 * Manual DMA Test - Manually DMA one 16x16 sprite to VRAM
 * This bypasses oamVramQueueUpdate to test if DMA itself works
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

/* Manual 16x16 sprite DMA - mimics what oamVramQueueUpdate should do */
void manual16x16DMA(u8 *source, u16 vramDest) {
    /* Set VRAM increment mode */
    REG_VMAIN = 0x80;

    /* Set VRAM address for first row (top 2 tiles) */
    REG_VMADDL = vramDest & 0xFF;
    REG_VMADDH = vramDest >> 8;

    /* DMA channel 2: transfer 64 bytes (top row) */
    *(vu8*)0x4320 = 0x01;  /* Word transfer to $2118/$2119 */
    *(vu8*)0x4321 = 0x18;  /* VMDATAL */
    *(vu16*)0x4322 = (u16)(unsigned long)source;  /* Source address */
    *(vu8*)0x4324 = (u8)((unsigned long)source >> 16);  /* Source bank */
    *(vu16*)0x4325 = 64;   /* Transfer size: 64 bytes */

    /* Start DMA channel 2 */
    *(vu8*)0x420B = 0x04;

    /* Set VRAM address for second row (bottom 2 tiles) - offset $100 */
    REG_VMADDL = vramDest & 0xFF;
    REG_VMADDH = (vramDest >> 8) + 1;

    /* DMA channel 2: transfer 64 bytes (bottom row) from source+$200 */
    *(vu16*)0x4322 = (u16)(unsigned long)source + 0x200;
    *(vu16*)0x4325 = 64;

    /* Start DMA channel 2 */
    *(vu8*)0x420B = 0x04;
}

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Initialize dynamic sprite system */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Upload palette */
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Manually DMA sprite frame 0 to VRAM $0000 */
    manual16x16DMA(spr16_tiles, 0x0000);

    /* Set up sprite using oambuffer (no refresh - tiles already uploaded) */
    oambuffer[0].oamx = 120;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 0;
    OAM_SET_GFX(0, spr16_tiles);

    /* Draw sprite */
    oamDynamic16Draw(0);
    oamInitDynamicSpriteEndFrame();

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();
        oamDynamic16Draw(0);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
