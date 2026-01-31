/**
 * 4 animated sprites with callback
 * Tests if multiple sprites work (under 7/frame limit)
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

#define NUM_SPRITES 4

static u8 frame_counter;

void vblankCallback(void) {
    oamVramQueueUpdate();
}

int main(void) {
    u8 i;

    REG_INIDISP = INIDISP_FORCE_BLANK;

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Initialize 4 sprites in a row */
    for (i = 0; i < NUM_SPRITES; i++) {
        oambuffer[i].oamx = 64 + i * 48;
        oambuffer[i].oamy = 100;
        oambuffer[i].oamframeid = i;
        oambuffer[i].oamattribute = OBJ_PRIO(3);
        oambuffer[i].oamrefresh = 1;
        OAM_SET_GFX(i, spr16_tiles);
    }

    /* Initial draw and VRAM upload during force blank */
    for (i = 0; i < NUM_SPRITES; i++) {
        oamDynamic16Draw(i);
    }
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
            for (i = 0; i < NUM_SPRITES; i++) {
                oambuffer[i].oamframeid = (oambuffer[i].oamframeid + 1) % 24;
                oambuffer[i].oamrefresh = 1;
            }
        }

        for (i = 0; i < NUM_SPRITES; i++) {
            oamDynamic16Draw(i);
        }
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
