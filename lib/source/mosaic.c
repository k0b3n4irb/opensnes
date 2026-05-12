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

/* Shadow register to track current state. External linkage so the
 * `inline mosaicInit()` in mosaic.h can access it from any TU. */
u8 mosaic_size;
u8 mosaic_bg_mask;

/*============================================================================
 * Internal Helper
 *============================================================================*/

static void mosaic_update_register(void) {
    REG_MOSAIC = (mosaic_size << 4) | mosaic_bg_mask;
}

/*============================================================================
 * Public Functions
 *============================================================================*/

/* mosaicInit() is `inline` in mosaic.h. Force-emit the standalone here
 * via address-taking so non-inlining callers (fn-ptr, etc.) link. */
void (*const __opensnes_force_emit_mosaicInit)(void) = mosaicInit;

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
