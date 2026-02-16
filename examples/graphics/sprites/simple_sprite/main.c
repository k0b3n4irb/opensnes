/**
 * @file main.c
 * @brief Simple Sprite — Display a Single 32x32 Sprite
 *
 * Displays a single 32x32 sprite at center screen.
 *
 * VRAM Layout:
 *   $2000 = OBSEL name base (tileBase=1 means $2000 word addr)
 *   $2100 = Sprite tile data loaded here
 *
 * Tile number: (0x2100 - 0x2000) / 16 = 0x10
 */

#include <snes.h>

extern u8 sprite32[], sprite32_end[], palsprite32[];

int main(void) {
    consoleInit();

    /* Load sprite tiles to VRAM $2100 */
    WaitForVBlank();
    dmaCopyVram(sprite32, 0x2100, sprite32_end - sprite32);

    /* Load palette to CGRAM 128 (sprite palette 0) */
    dmaCopyCGram(palsprite32, 128, 32);

    /* Set OBJ size: small=8, large=32, name base=1 ($2000) */
    oamInitEx(OBJ_SIZE8_L32, 1);

    /* Place one sprite at center screen
     * tile = (0x2100 - 0x2000) / 16 = 0x10
     * palette=0, priority=3 (front), no flip
     */
    oamSet(0, 112, 96, 0x0010, 0, 3, 0);
    oamSetEx(0, OBJ_LARGE, OBJ_SHOW);

    /* Enable Mode 1 with sprites only */
    setMode(BG_MODE1, 0);
    REG_TM = TM_OBJ;
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
