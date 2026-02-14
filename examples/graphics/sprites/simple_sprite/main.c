/**
 * @file main.c
 * @brief Simple Sprite Example
 *
 * Port of pvsneslib SimpleSprite example.
 * Demonstrates how to display a static 32x32 sprite.
 *
 * This example shows:
 * - Loading sprite tiles to VRAM using oamInitGfxSet()
 * - Loading sprite palette to CGRAM
 * - Setting up a sprite with oamSet
 * - Basic sprite display in Mode 1
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 sprite_tiles[], sprite_tiles_end[];
extern u8 sprite_pal[], sprite_pal_end[];

int main(void) {
    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Initialize sprite system with graphics:
     * - Load tiles to VRAM $0000
     * - Load palette to CGRAM 128 (sprite palette 0)
     * - Configure 32x32 small / 64x64 large sprites
     */
    oamInitGfxSet(sprite_tiles, sprite_tiles_end - sprite_tiles,
                  sprite_pal, sprite_pal_end - sprite_pal,
                  0, 0x0000, OBJ_SIZE32_L64);

    /* Note: No explicit background color set to match PVSnesLib behavior.
     * The background color will be whatever the library initializes CGRAM 0 to.
     */

    /* Set up sprite 0:
     * Position: (100, 100)
     * Tile: 0 (first tile)
     * Palette: 0
     * Priority: 3 (highest)
     * Flags: 0 (no flip)
     */
    oamSet(0, 100, 100, 0, 0, 3, 0);

    /* Set sprite 0 to use small size (32x32) and visible */
    oamSetSize(0, 0);  /* 0 = small size */
    oamSetVisible(0, 1);

    /* Hide all other sprites */
    for (u8 i = 1; i < 128; i++) {
        oamHide(i);
    }

    /* Update OAM buffer to hardware */
    oamUpdate();

    /* Set Mode 1 (needed for sprites) */
    REG_BGMODE = 0x01;

    /* Enable sprites on main screen only (no backgrounds) */
    REG_TM = TM_OBJ;

    /* Enable display at full brightness */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
