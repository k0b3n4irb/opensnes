/**
 * Force Blank DMA Test
 * Calls oamVramQueueUpdate during force blank (before display enable)
 * This eliminates any VBlank timing issues
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Initialize dynamic sprite engine */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Upload palette */
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Set up ONE sprite */
    oambuffer[0].oamx = 120;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;  /* Request VRAM upload */
    OAM_SET_GFX(0, spr16_tiles);

    /* Draw sprite - this queues graphics */
    oamDynamic16Draw(0);

    /* Process queue DURING FORCE BLANK - not during VBlank */
    oamVramQueueUpdate();

    /* Now finish frame */
    oamInitDynamicSpriteEndFrame();

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();
        /* Don't call oamVramQueueUpdate - tiles already uploaded */
        oamDynamic16Draw(0);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
