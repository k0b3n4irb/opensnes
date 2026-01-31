/**
 * Hybrid Test - Static VRAM upload + Dynamic OAM update
 * This tests if the issue is with VRAM upload or OAM handling
 */

#include <snes.h>

extern u8 spr16_tiles[], spr16_tiles_end[];
extern u8 spr16_properpal[];

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Use static tile upload like oamInitGfxSet */
    dmaCopyVram(spr16_tiles, 0x0000, spr16_tiles_end - spr16_tiles);
    dmaCopyCGram(spr16_properpal, 128, 32);

    /* But use dynamic sprite init for OAM handling */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Set up sprite using oambuffer */
    oambuffer[0].oamx = 120;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 0;  /* Don't queue VRAM upload - tiles already there */
    OAM_SET_GFX(0, spr16_tiles);

    /* Draw using dynamic system (but no VRAM queue since oamrefresh=0) */
    oamDynamic16Draw(0);
    oamInitDynamicSpriteEndFrame();

    /* Enable display */
    REG_BGMODE = 0x01;
    REG_TM = TM_OBJ;
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();
        oamDynamic16Draw(0);
        oamInitDynamicSpriteEndFrame();
    }

    return 0;
}
