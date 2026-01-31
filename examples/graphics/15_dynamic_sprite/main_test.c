/**
 * @file main_test.c
 * @brief Simplified test - single static sprite to verify data
 */

#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_tiles_end[];
extern u8 spr16_pal[];

int main(void) {
    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Configure OBJSEL: 8x8/16x16 sprites, tiles at VRAM $0000 */
    REG_OBJSEL = OBJ_SIZE8_L16;  /* Small=8x8, Large=16x16, base at 0 */

    /* Upload sprite tiles to VRAM $0000 */
    /* For 16x16 sprites, need to upload in VRAM format */
    dmaCopyVram(spr16_tiles, 0x0000, spr16_tiles_end - spr16_tiles);

    /* Upload sprite palette to CGRAM 128 (sprite palette 0) */
    dmaCopyCGram(spr16_pal, 128, 32);

    /* Initialize OAM */
    oamInit();

    /* Set up a simple sprite at position (100, 100) */
    /* Tile 0 = first 16x16 sprite in the sheet */
    oamSet(0, 100, 100, 0, 0, 2, 0);  /* id, x, y, tile, pal, prio, flags */
    oamSetEx(0, OBJ_LARGE, OBJ_SHOW);  /* Large size, visible */

    /* Enable sprites on main screen */
    REG_TM = TM_OBJ;

    /* Enable display */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop - just wait */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
