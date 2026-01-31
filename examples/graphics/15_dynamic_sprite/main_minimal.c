/**
 * Minimal Dynamic Sprite Test
 * Tests just ONE sprite to isolate the issue
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

    /* Set up ONE sprite at center of screen */
    oambuffer[0].oamx = 120;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);

    /* Draw the sprite */
    oamDynamic16Draw(0);
    oamInitDynamicSpriteEndFrame();

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop - just keep updating */
    while (1) {
        WaitForVBlank();
        oamVramQueueUpdate();

        /* Redraw sprite each frame (no animation) */
        oamDynamic16Draw(0);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
