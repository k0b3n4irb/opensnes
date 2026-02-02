/**
 * @file mosaic.c
 * @brief SNES Mosaic Effects Implementation
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/mosaic.h>

/*============================================================================
 * MOSAIC Register ($2106)
 *
 * Bits 7-4: XXXX = Mosaic Size (actual block size = X + 1 pixels)
 * Bit 3: Enable mosaic for BG4
 * Bit 2: Enable mosaic for BG3
 * Bit 1: Enable mosaic for BG2
 * Bit 0: Enable mosaic for BG1
 *
 * Note: Size is shared across all enabled backgrounds.
 *============================================================================*/

/* Shadow register to track current state */
static u8 mosaic_size;
static u8 mosaic_bg_mask;

/*============================================================================
 * Internal Helper
 *============================================================================*/

static void mosaic_update_register(void) {
    REG_MOSAIC = (mosaic_size << 4) | mosaic_bg_mask;
}

/*============================================================================
 * Public Functions
 *============================================================================*/

void mosaicInit(void) {
    mosaic_size = 0;
    mosaic_bg_mask = 0;
    REG_MOSAIC = 0;
}

void mosaicEnable(u8 bgMask) {
    mosaic_bg_mask = bgMask & 0x0F;
    mosaic_update_register();
}

void mosaicDisable(void) {
    mosaic_bg_mask = 0;
    mosaic_update_register();
}

void mosaicSetSize(u8 size) {
    if (size > MOSAIC_MAX) {
        size = MOSAIC_MAX;
    }
    mosaic_size = size;
    mosaic_update_register();
}

u8 mosaicGetSize(void) {
    return mosaic_size;
}

void mosaicFadeIn(u8 speed) {
    u8 i;
    u8 wait;

    /* Fade from current size down to 0 */
    while (mosaic_size > 0) {
        mosaic_size--;
        mosaic_update_register();

        /* Wait specified number of frames */
        for (wait = 0; wait < speed; wait++) {
            WaitForVBlank();
        }
    }
}

void mosaicFadeOut(u8 speed) {
    u8 wait;

    /* Fade from current size up to maximum */
    while (mosaic_size < MOSAIC_MAX) {
        mosaic_size++;
        mosaic_update_register();

        /* Wait specified number of frames */
        for (wait = 0; wait < speed; wait++) {
            WaitForVBlank();
        }
    }
}
