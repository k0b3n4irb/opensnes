/**
 * @file main.c
 * @brief Dynamic Sprite Engine Demo - Multiple animated sprites
 *
 * Demonstrates the dynamic sprite engine with:
 * - Multiple 16x16 sprites
 * - Animation (cycling through frames)
 * - Dynamic VRAM uploading during VBlank
 *
 * IMPORTANT: oamVramQueueUpdate() must be called DURING VBlank (from NMI callback),
 * not after WaitForVBlank() returns, because VRAM is only accessible during VBlank.
 */

#include <snes.h>

/* 16x16 sprite tiles and proper palette */
extern u8 spr16_tiles[];
extern u8 spr16_properpal[];

#define NUM_SPRITES 8
#define NUM_FRAMES 24  /* 24 frames in the sprite sheet */

/* Sprite state */
static u8 frame_counter;

/**
 * VBlank callback - called during NMI (while VRAM is accessible)
 * This is where we must do VRAM DMA transfers.
 */
void vblankCallback(void) {
    oamVramQueueUpdate();
}

int main(void) {
    u8 i;

    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Initialize dynamic sprite engine */
    /* VRAM base=0x0000, VRAM limit=0x1000, palette=0, tile base=0, size=8/16 */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Upload palette to CGRAM (sprite palette starts at 128) */
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* Register VBlank callback for VRAM updates */
    nmiSet(vblankCallback);

    /* Initialize sprites in a grid pattern */
    for (i = 0; i < NUM_SPRITES; i++) {
        oambuffer[i].oamx = 48 + (i % 4) * 48;  /* 4 columns */
        oambuffer[i].oamy = 64 + (i / 4) * 64;  /* 2 rows */
        oambuffer[i].oamframeid = i % NUM_FRAMES;  /* Stagger animation */
        oambuffer[i].oamattribute = OBJ_PRIO(3);
        oambuffer[i].oamrefresh = 1;
        OAM_SET_GFX(i, spr16_tiles);
    }

    /* Initial draw */
    for (i = 0; i < NUM_SPRITES; i++) {
        oamDynamic16Draw(i);
    }
    oamInitDynamicSpriteEndFrame();

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop with animation */
    while (1) {
        WaitForVBlank();

        /* Note: oamVramQueueUpdate is called in vblankCallback during NMI */

        /* Animate every 8 frames */
        frame_counter++;
        if (frame_counter >= 8) {
            frame_counter = 0;

            /* Update each sprite's frame */
            for (i = 0; i < NUM_SPRITES; i++) {
                oambuffer[i].oamframeid = (oambuffer[i].oamframeid + 1) % NUM_FRAMES;
                oambuffer[i].oamrefresh = 1;
            }
        }

        /* Draw sprites for next frame */
        for (i = 0; i < NUM_SPRITES; i++) {
            oamDynamic16Draw(i);
        }
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
