/**
 * @file main.c
 * @brief Dynamic Sprite Engine Test - Fixed timing
 *
 * Key fix: Call oamDynamic16Draw and oamVramQueueUpdate BEFORE
 * enabling display to ensure VRAM is populated on first frame.
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_pal[];

#define NUM_SPRITES 8
#define NUM_FRAMES 24
#define ANIM_SPEED 8

int main(void) {
    u8 i;
    u8 frame_counter = 0;

    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Initialize dynamic sprite engine */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Upload palette */
    dmaCopyCGram(spr16_pal, 128, 32);

    /* Initialize sprites */
    for (i = 0; i < NUM_SPRITES; i++) {
        oambuffer[i].oamx = 40 + (i % 4) * 50;
        oambuffer[i].oamy = 60 + (i / 4) * 60;
        oambuffer[i].oamframeid = i % NUM_FRAMES;
        oambuffer[i].oamattribute = OBJ_PRIO(2);
        oambuffer[i].oamrefresh = 1;
        OAM_SET_GFX(i, spr16_tiles);
    }

    /* IMPORTANT: Draw sprites and upload VRAM BEFORE enabling display */
    for (i = 0; i < NUM_SPRITES; i++) {
        oamDynamic16Draw(i);
    }
    oamInitDynamicSpriteEndFrame();

    /* Process initial VRAM uploads - force blank is still active */
    oamVramQueueUpdate();

    /* Now enable display */
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
    while (1) {
        /* Update animation */
        frame_counter++;
        if (frame_counter >= ANIM_SPEED) {
            frame_counter = 0;
            for (i = 0; i < NUM_SPRITES; i++) {
                oambuffer[i].oamframeid = (oambuffer[i].oamframeid + 1) % NUM_FRAMES;
                oambuffer[i].oamrefresh = 1;
            }
        }

        /* Draw sprites */
        for (i = 0; i < NUM_SPRITES; i++) {
            oamDynamic16Draw(i);
        }
        oamInitDynamicSpriteEndFrame();

        WaitForVBlank();

        /* Upload VRAM during VBlank */
        oamVramQueueUpdate();
    }

    return 0;
}
