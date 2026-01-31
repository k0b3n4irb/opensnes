/**
 * 8 animated sprites - using helper to reduce stack size
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

static u8 frame_counter;

void vblankCallback(void) {
    oamVramQueueUpdate();
}

void setupSprite(u8 id, s16 x, s16 y, u8 frame) {
    oambuffer[id].oamx = x;
    oambuffer[id].oamy = y;
    oambuffer[id].oamframeid = frame;
    oambuffer[id].oamattribute = OBJ_PRIO(3);
    oambuffer[id].oamrefresh = 1;
    OAM_SET_GFX(id, spr16_tiles);
}

void animateSprite(u8 id) {
    oambuffer[id].oamframeid = (oambuffer[id].oamframeid + 1) % 24;
    oambuffer[id].oamrefresh = 1;
}

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Setup 8 sprites in 2 rows */
    setupSprite(0, 48, 80, 0);
    setupSprite(1, 96, 80, 1);
    setupSprite(2, 144, 80, 2);
    setupSprite(3, 192, 80, 3);
    setupSprite(4, 48, 130, 4);
    setupSprite(5, 96, 130, 5);
    setupSprite(6, 144, 130, 6);
    setupSprite(7, 192, 130, 7);

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
            animateSprite(0);
            animateSprite(1);
            animateSprite(2);
            animateSprite(3);
            animateSprite(4);
            animateSprite(5);
            animateSprite(6);
            animateSprite(7);
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
