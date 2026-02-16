/**
 * @file main.c
 * @brief Dynamic Sprite — VRAM Streaming Animation
 *
 * Demonstrates the dynamic sprite engine: 4 animated 16x16 sprites
 * with per-frame VRAM tile uploads.
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

static const s16 xpos[4] = {64, 112, 160, 208};
static u8 frame_counter;

/* Individual frame counters - all start at 0 now */
static u16 frame0;
static u16 frame1;
static u16 frame2;
static u16 frame3;

int main(void) {
    u8 i;

    setScreenOff();

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Initialize frame counters - all start at same frame */
    frame0 = 0;
    frame1 = 0;
    frame2 = 0;
    frame3 = 0;

    /* Initialize sprites - explicit to avoid for-loop compiler bugs */
    oambuffer[0].oamx = xpos[0];
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = frame0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);

    oambuffer[1].oamx = xpos[1];
    oambuffer[1].oamy = 100;
    oambuffer[1].oamframeid = frame1;
    oambuffer[1].oamattribute = OBJ_PRIO(3);
    oambuffer[1].oamrefresh = 1;
    OAM_SET_GFX(1, spr16_tiles);

    oambuffer[2].oamx = xpos[2];
    oambuffer[2].oamy = 100;
    oambuffer[2].oamframeid = frame2;
    oambuffer[2].oamattribute = OBJ_PRIO(3);
    oambuffer[2].oamrefresh = 1;
    OAM_SET_GFX(2, spr16_tiles);

    oambuffer[3].oamx = xpos[3];
    oambuffer[3].oamy = 100;
    oambuffer[3].oamframeid = frame3;
    oambuffer[3].oamattribute = OBJ_PRIO(3);
    oambuffer[3].oamrefresh = 1;
    OAM_SET_GFX(3, spr16_tiles);

    /* Draw in normal order now that init is explicit */
    oamDynamic16Draw(0);
    oamDynamic16Draw(1);
    oamDynamic16Draw(2);
    oamDynamic16Draw(3);
    oamVramQueueUpdate();
    oamInitDynamicSpriteEndFrame();

    setMode(BG_MODE1, 0);
    REG_TM = TM_OBJ;
    setScreenOn();

    while (1) {
        WaitForVBlank();

        frame_counter++;
        if (frame_counter >= 8) {
            frame_counter = 0;

            /* Animate sprite 0 */
            frame0++;
            if (frame0 >= 24) frame0 = 0;
            oambuffer[0].oamframeid = frame0;
            oambuffer[0].oamrefresh = 1;

            /* Animate sprite 1 */
            frame1++;
            if (frame1 >= 24) frame1 = 0;
            oambuffer[1].oamframeid = frame1;
            oambuffer[1].oamrefresh = 1;

            /* Animate sprite 2 */
            frame2++;
            if (frame2 >= 24) frame2 = 0;
            oambuffer[2].oamframeid = frame2;
            oambuffer[2].oamrefresh = 1;

            /* Animate sprite 3 */
            frame3++;
            if (frame3 >= 24) frame3 = 0;
            oambuffer[3].oamframeid = frame3;
            oambuffer[3].oamrefresh = 1;
        }

        /* Draw all sprites */
        oamDynamic16Draw(0);
        oamDynamic16Draw(1);
        oamDynamic16Draw(2);
        oamDynamic16Draw(3);
        oamVramQueueUpdate();
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
