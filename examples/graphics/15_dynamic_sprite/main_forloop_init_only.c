/**
 * Test: For loops ONLY during initialization, not in main loop
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

#define NUM_SPRITES 4

int main(void) {
    u8 i;

    REG_INIDISP = INIDISP_FORCE_BLANK;

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Initialize with for loop */
    for (i = 0; i < NUM_SPRITES; i++) {
        oambuffer[i].oamx = 64 + i * 48;
        oambuffer[i].oamy = 100;
        oambuffer[i].oamframeid = i;
        oambuffer[i].oamattribute = OBJ_PRIO(3);
        oambuffer[i].oamrefresh = 1;
        OAM_SET_GFX(i, spr16_tiles);
    }

    /* Initial draw with for loop */
    for (i = 0; i < NUM_SPRITES; i++) {
        oamDynamic16Draw(i);
    }
    oamVramQueueUpdate();
    oamInitDynamicSpriteEndFrame();

    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Simple main loop - NO for loops, no animation */
    while (1) {
        WaitForVBlank();
        /* Just redraw the same sprites explicitly */
        oamDynamic16Draw(0);
        oamDynamic16Draw(1);
        oamDynamic16Draw(2);
        oamDynamic16Draw(3);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
