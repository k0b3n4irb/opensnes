/*
 * getcolors - Ported from PVSnesLib to OpenSNES
 *
 * Simple mode 1 example with palette management
 * -- alekmaul (original PVSnesLib example)
 *
 * Demonstrates reading palette colors from CGRAM using direct register access.
 *
 * PVSnesLib original layout:
 *   - Text on BG1: tiles at $3000, tilemap at $6800
 *   - Image on BG2: tiles at $4000, tilemap at $1000, palette entry 1
 *
 * Our layout (image only, no text):
 *   - Image on BG1: tiles at $2000, tilemap at $6800, palette entry 0
 *   - PNG fixed: color index 255 remapped to 15 for clean 16-color conversion
 */

#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

int main(void) {
    /* Force blank for safe PPU setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Enable NMI + auto-joypad */
    REG_NMITIMEN = NMITIMEN_NMI_ENABLE | NMITIMEN_JOY_ENABLE;

    /* Mode 1 */
    REG_BGMODE = BGMODE_MODE1;

    /* BG1 tilemap at VRAM $6800, 32x32 */
    REG_BG1SC = 0x68;

    /* BG1 char base at $2000 (nibble 2), BG2 char base at $0000 */
    REG_BG12NBA = 0x02;

    /* Clear CGRAM first */
    REG_CGADD = 0;
    u16 i;
    for (i = 0; i < 256; i++) {
        REG_CGDATA = 0;
        REG_CGDATA = 0;
    }

    /* Load palette to CGRAM entry 0 (color index 0) */
    dmaCopyCGram(palette, 0, palette_end - palette);

    /* DMA tiles to VRAM $2000 */
    dmaCopyVram(tiles, 0x2000, tiles_end - tiles);

    /* DMA tilemap to VRAM $6800 */
    dmaCopyVram(tilemap, 0x6800, tilemap_end - tilemap);

    /* Enable BG1 only */
    REG_TM = TM_BG1;

    /* Screen on: brightness 15 */
    REG_INIDISP = 0x0F;

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
