/**
 * Callback Test - Verify NMI callback is being called
 * Changes background color in callback to prove it runs
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

static u8 callback_count;

void vblankCallback(void) {
    /* Change background color to prove callback runs */
    callback_count++;
    if (callback_count < 60) {
        /* Red background for first second */
        *(vu8*)0x2121 = 0;  /* CGRAM address 0 */
        *(vu8*)0x2122 = 0x1F;  /* Red (low byte) */
        *(vu8*)0x2122 = 0x00;  /* Red (high byte) */
    } else {
        /* Green background after */
        *(vu8*)0x2121 = 0;
        *(vu8*)0x2122 = 0xE0;  /* Green (low byte) */
        *(vu8*)0x2122 = 0x03;  /* Green (high byte) */
    }

    /* Also try the VRAM queue update */
    oamVramQueueUpdate();
}

int main(void) {
    u8 i;

    REG_INIDISP = INIDISP_FORCE_BLANK;

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Set initial background to blue */
    *(vu8*)0x2121 = 0;
    *(vu8*)0x2122 = 0x00;
    *(vu8*)0x2122 = 0x7C;  /* Blue */

    nmiSet(vblankCallback);

    /* Set up ONE sprite */
    oambuffer[0].oamx = 120;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);

    oamDynamic16Draw(0);
    oamInitDynamicSpriteEndFrame();

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
