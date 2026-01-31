/**
 * 8 animated sprites - explicit setup (no loops)
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

    /* Row 1: Sprites 0-3 */
    oambuffer[0].oamx = 48;
    oambuffer[0].oamy = 80;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);

    oambuffer[1].oamx = 96;
    oambuffer[1].oamy = 80;
    oambuffer[1].oamframeid = 1;
    oambuffer[1].oamattribute = OBJ_PRIO(3);
    oambuffer[1].oamrefresh = 1;
    OAM_SET_GFX(1, spr16_tiles);

    oambuffer[2].oamx = 144;
    oambuffer[2].oamy = 80;
    oambuffer[2].oamframeid = 2;
    oambuffer[2].oamattribute = OBJ_PRIO(3);
    oambuffer[2].oamrefresh = 1;
    OAM_SET_GFX(2, spr16_tiles);

    oambuffer[3].oamx = 192;
    oambuffer[3].oamy = 80;
    oambuffer[3].oamframeid = 3;
    oambuffer[3].oamattribute = OBJ_PRIO(3);
    oambuffer[3].oamrefresh = 1;
    OAM_SET_GFX(3, spr16_tiles);

    /* Row 2: Sprites 4-7 */
    oambuffer[4].oamx = 48;
    oambuffer[4].oamy = 130;
    oambuffer[4].oamframeid = 4;
    oambuffer[4].oamattribute = OBJ_PRIO(3);
    oambuffer[4].oamrefresh = 1;
    OAM_SET_GFX(4, spr16_tiles);

    oambuffer[5].oamx = 96;
    oambuffer[5].oamy = 130;
    oambuffer[5].oamframeid = 5;
    oambuffer[5].oamattribute = OBJ_PRIO(3);
    oambuffer[5].oamrefresh = 1;
    OAM_SET_GFX(5, spr16_tiles);

    oambuffer[6].oamx = 144;
    oambuffer[6].oamy = 130;
    oambuffer[6].oamframeid = 6;
    oambuffer[6].oamattribute = OBJ_PRIO(3);
    oambuffer[6].oamrefresh = 1;
    OAM_SET_GFX(6, spr16_tiles);

    oambuffer[7].oamx = 192;
    oambuffer[7].oamy = 130;
    oambuffer[7].oamframeid = 7;
    oambuffer[7].oamattribute = OBJ_PRIO(3);
    oambuffer[7].oamrefresh = 1;
    OAM_SET_GFX(7, spr16_tiles);

    /* Initial draw */
    oamDynamic16Draw(0);
    oamDynamic16Draw(1);
    oamDynamic16Draw(2);
    oamDynamic16Draw(3);
    oamDynamic16Draw(4);
    oamDynamic16Draw(5);
    oamDynamic16Draw(6);
    oamDynamic16Draw(7);
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
            oambuffer[2].oamframeid = (oambuffer[2].oamframeid + 1) % 24;
            oambuffer[2].oamrefresh = 1;
            oambuffer[3].oamframeid = (oambuffer[3].oamframeid + 1) % 24;
            oambuffer[3].oamrefresh = 1;
            oambuffer[4].oamframeid = (oambuffer[4].oamframeid + 1) % 24;
            oambuffer[4].oamrefresh = 1;
            oambuffer[5].oamframeid = (oambuffer[5].oamframeid + 1) % 24;
            oambuffer[5].oamrefresh = 1;
            oambuffer[6].oamframeid = (oambuffer[6].oamframeid + 1) % 24;
            oambuffer[6].oamrefresh = 1;
            oambuffer[7].oamframeid = (oambuffer[7].oamframeid + 1) % 24;
            oambuffer[7].oamrefresh = 1;
        }

        oamDynamic16Draw(0);
        oamDynamic16Draw(1);
        oamDynamic16Draw(2);
        oamDynamic16Draw(3);
        oamDynamic16Draw(4);
        oamDynamic16Draw(5);
        oamDynamic16Draw(6);
        oamDynamic16Draw(7);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
