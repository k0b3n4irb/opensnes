/**
 * Single animated sprite with callback
 * Tests if oamVramQueueUpdate works in callback
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

static u8 frame_counter;

void vblankCallback(void) {
    oamVramQueueUpdate();
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

    /* Initial draw and VRAM upload during force blank */
    oamDynamic16Draw(0);
    oamVramQueueUpdate();
    oamInitDynamicSpriteEndFrame();

    /* Register callback */
    nmiSet(vblankCallback);

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();

        /* Animate every 8 frames */
        frame_counter++;
        if (frame_counter >= 8) {
            frame_counter = 0;
            oambuffer[0].oamframeid = (oambuffer[0].oamframeid + 1) % 24;
            oambuffer[0].oamrefresh = 1;  /* Request new graphics */
        }

        oamDynamic16Draw(0);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
