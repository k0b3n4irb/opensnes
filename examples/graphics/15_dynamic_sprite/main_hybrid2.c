/**
 * Hybrid approach: Initial VRAM upload during force blank,
 * then use callback for subsequent animation frames
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

#define NUM_SPRITES 8
#define NUM_FRAMES 24

static u8 frame_counter;

void vblankCallback(void) {
    oamVramQueueUpdate();
}

int main(void) {
    u8 i;

    REG_INIDISP = INIDISP_FORCE_BLANK;

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Initialize sprites */
    for (i = 0; i < NUM_SPRITES; i++) {
        oambuffer[i].oamx = 48 + (i % 4) * 48;
        oambuffer[i].oamy = 64 + (i / 4) * 64;
        oambuffer[i].oamframeid = i % NUM_FRAMES;
        oambuffer[i].oamattribute = OBJ_PRIO(3);
        oambuffer[i].oamrefresh = 1;
        OAM_SET_GFX(i, spr16_tiles);
    }

    /* Initial draw - queues graphics */
    for (i = 0; i < NUM_SPRITES; i++) {
        oamDynamic16Draw(i);
    }

    /* Process queue DURING FORCE BLANK - this works! */
    oamVramQueueUpdate();

    oamInitDynamicSpriteEndFrame();

    /* Register callback for subsequent animation updates */
    nmiSet(vblankCallback);

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Animate every 8 frames */
        frame_counter++;
        if (frame_counter >= 8) {
            frame_counter = 0;

            for (i = 0; i < NUM_SPRITES; i++) {
                oambuffer[i].oamframeid = (oambuffer[i].oamframeid + 1) % NUM_FRAMES;
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
