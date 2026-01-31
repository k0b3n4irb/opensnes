/**
 * Static Sprite Test (uses oamInitGfxSet like simple_sprite)
 * This should definitely work - if it doesn't, basic sprite display is broken
 */

#include <snes.h>

extern u8 spr16_tiles[], spr16_tiles_end[];
extern u8 spr16_properpal[];

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Use static sprite initialization - uploads tiles directly to VRAM */
    oamInitGfxSet(spr16_tiles, spr16_tiles_end - spr16_tiles,
                  spr16_properpal, 32, 0, 0x0000, OBJ_SIZE8_L16);

    /* Set up one sprite using standard oamSet */
    oamSet(0, 120, 100, 0, 0, 3, 0);
    oamSetSize(0, 1);  /* Large size (16x16) */
    oamSetVisible(0, 1);

    /* Hide other sprites */
    for (u8 i = 1; i < 128; i++) {
        oamHide(i);
    }

    /* Update OAM to hardware */
    oamUpdate();

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
