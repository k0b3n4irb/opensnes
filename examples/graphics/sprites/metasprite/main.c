/**
 * @file main.c
 * @brief Simple 16x16 Sprite Test
 *
 * Tests basic sprite display with 16x16 sprites.
 * Uses spritehero8.png which is 64x16 = 4 sprites of 16x16.
 *
 * SNES 16x16 sprite tile addressing:
 * For OAM tile N, hardware uses 8x8 tiles: N, N+1, N+16, N+17
 *
 * With VRAM 16 tiles wide (128 pixels):
 * - Sprite 0: tile 0 (uses tiles 0,1,16,17)
 * - Sprite 1: tile 2 (uses tiles 2,3,18,19)
 * - Sprite 2: tile 4 (uses tiles 4,5,20,21)
 * - Sprite 3: tile 6 (uses tiles 6,7,22,23)
 */

#include <snes.h>

/* External assets - 16x16 sprite converted with -s 16 */
extern u8 sprite_tiles[], sprite_tiles_end[];
extern u8 sprite_pal[], sprite_pal_end[];

int main(void) {
    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Load sprite tiles to VRAM $0000 */
    dmaCopyVram(sprite_tiles, 0x0000, sprite_tiles_end - sprite_tiles);

    /* Load palette to CGRAM 128 (sprite palette 0)
     * Sprite palette 0 = CGRAM entries 128-143 = 16 colors = 32 bytes */
    dmaCopyCGram(sprite_pal, 128, 32);

    /* Set OBJSEL for 8x8 small / 16x16 large, base at 0 */
    REG_OBJSEL = (OBJ_SIZE8_L16 << 5) | 0;

    /* Clear OAM */
    oamClear();

    /* Display 4 16x16 sprites - the 4 character poses
     * Source: 64x16 PNG = 4 sprites of 16x16
     * After gfx4snes -s 16: reorganized to 128x16 (8 per row, only 4 used)
     * Sprite tile numbers: 0, 2, 4, 6
     */

    /* DEBUG: Directly write to OAM buffer to test DMA */
    extern u8 oamMemory[];

    /* Display ONE 32x48 metasprite composed of 6 16x16 sprites (2x3 grid)
     *
     * Source: 32x192 PNG = 4 frames of 32x48 characters
     * Each frame = 2 columns × 3 rows of 16x16 blocks = 6 sprites
     *
     * PVSnesLib metasprite format (from spritehero16_meta.inc):
     *   Frame 0: tile indices 0-5   -> OAM tiles 0,2,4,6,8,10
     *   Frame 1: tile indices 6-11  -> OAM tiles 12,14,16,18,20,22
     *   Frame 2: tile indices 12-17 -> OAM tiles 24,26,28,30,32,34
     *   Frame 3: tile indices 18-23 -> OAM tiles 36,38,40,42,44,46
     *
     * Arrangement for 32x48:
     *   [tile N  ][tile N+2 ]   <- Y+0  (top row)
     *   [tile N+4][tile N+6 ]   <- Y+16 (middle row)
     *   [tile N+8][tile N+10]   <- Y+32 (bottom row)
     */

    /* Metasprite position: center of screen */
    u8 meta_x = 112;
    u8 meta_y = 88;
    u8 base_tile = 0;  /* Frame 0: OAM tile 0 (tile index 0 * 2) */

    /* Row 0 (Y+0): tiles 0, 2 (top of character - head) */
    oamMemory[0] = meta_x;
    oamMemory[1] = meta_y;
    oamMemory[2] = base_tile;
    oamMemory[3] = 0x30;

    oamMemory[4] = meta_x + 16;
    oamMemory[5] = meta_y;
    oamMemory[6] = base_tile + 2;
    oamMemory[7] = 0x30;

    /* Row 1 (Y+16): tiles 4, 6 (middle - body) */
    oamMemory[8] = meta_x;
    oamMemory[9] = meta_y + 16;
    oamMemory[10] = base_tile + 4;
    oamMemory[11] = 0x30;

    oamMemory[12] = meta_x + 16;
    oamMemory[13] = meta_y + 16;
    oamMemory[14] = base_tile + 6;
    oamMemory[15] = 0x30;

    /* Row 2 (Y+32): tiles 8, 10 (bottom - feet) */
    oamMemory[16] = meta_x;
    oamMemory[17] = meta_y + 32;
    oamMemory[18] = base_tile + 8;
    oamMemory[19] = 0x30;

    oamMemory[20] = meta_x + 16;
    oamMemory[21] = meta_y + 32;
    oamMemory[22] = base_tile + 10;
    oamMemory[23] = 0x30;

    /* Secondary table: Set all 6 sprites to Large (16x16) */
    /* Byte 512: bits 1,3,5,7 = size for sprites 0,1,2,3 -> 0xAA */
    /* Byte 513: bits 1,3 = size for sprites 4,5 -> 0x0A */
    oamMemory[512] = 0xAA;
    oamMemory[513] = 0x0A;

    /* Hide remaining sprites */
    for (u8 i = 6; i < 128; i++) {
        oamHide(i);
    }

    /* Update OAM */
    oamUpdate();

    /* Set Mode 1 */
    REG_BGMODE = 0x01;

    /* Enable sprites on main screen */
    REG_TM = TM_OBJ;

    /* Set a visible background color (blue) to confirm code runs */
    REG_CGADD = 0;  /* Palette entry 0 = backdrop */
    REG_CGDATA = 0x00;  /* Low byte: 0 */
    REG_CGDATA = 0x7C;  /* High byte: Blue = $7C00 = 0b0_11111_00000_00000 */

    /* Enable display */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
