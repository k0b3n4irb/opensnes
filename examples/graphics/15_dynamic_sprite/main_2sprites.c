/**
 * 2 animated sprites with callback
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

    /* Sprite 0 */
    oambuffer[0].oamx = 100;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);

    /* Sprite 1 */
    oambuffer[1].oamx = 140;
    oambuffer[1].oamy = 100;
    oambuffer[1].oamframeid = 1;
    oambuffer[1].oamattribute = OBJ_PRIO(3);
    oambuffer[1].oamrefresh = 1;
    OAM_SET_GFX(1, spr16_tiles);

    /* Initial draw */
    oamDynamic16Draw(0);
    oamDynamic16Draw(1);
    oamVramQueueUpdate();
    oamInitDynamicSpriteEndFrame();

    nmiSet(vblankCallback);

    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();

        frame_counter++;
        if (frame_counter >= 8) {
            frame_counter = 0;
            oambuffer[0].oamframeid = (oambuffer[0].oamframeid + 1) % 24;
            oambuffer[0].oamrefresh = 1;
            oambuffer[1].oamframeid = (oambuffer[1].oamframeid + 1) % 24;
            oambuffer[1].oamrefresh = 1;
        }

        oamDynamic16Draw(0);
        oamDynamic16Draw(1);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
