/**
 * @file main_simple.c
 * @brief Minimal Dynamic Sprite Test - just 1 sprite
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_pal[];

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Initialize dynamic sprite engine */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Upload palette */
    dmaCopyCGram(spr16_pal, 128, 32);

    /* Set up ONE sprite */
    oambuffer[0].oamx = 100;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(2);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);

    /* Draw and upload */
    oamDynamic16Draw(0);
    oamInitDynamicSpriteEndFrame();
    oamVramQueueUpdate();

    /* Enable display */
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop - just wait */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
