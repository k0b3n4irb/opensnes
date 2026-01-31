/**
 * Test if nmiSet() itself causes the problem
 * Same as working force blank test but with nmiSet added
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

void emptyCallback(void) {
    /* Do nothing */
}

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Set up ONE sprite */
    oambuffer[0].oamx = 120;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);

    oamDynamic16Draw(0);

    /* Process queue during force blank - this should work */
    oamVramQueueUpdate();

    oamInitDynamicSpriteEndFrame();

    /* Add nmiSet AFTER everything else - does this break it? */
    nmiSet(emptyCallback);

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
