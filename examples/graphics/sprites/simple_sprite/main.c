/**
 * @file main.c
 * @brief Display a single static 32x32 sprite
 * @ingroup examples
 *
 * Minimal example showing how to load and display one hardware sprite on
 * the SNES. Sprite tile data is DMA'd to VRAM, a 16-color palette is loaded
 * to CGRAM at address 128 (sprite palette 0), and the OBJ size register
 * (OBJSEL, $2101) is configured for small=8x8 / large=32x32 mode.
 *
 * The sprite is placed at screen center using oamSet(), which writes to the
 * 544-byte OAM buffer that the NMI handler DMAs to the PPU each VBlank.
 * The tile number is calculated from the offset between the OBJSEL name
 * base and the VRAM address where the tile data was loaded.
 *
 * @par SNES Concepts
 * - OAM (Object Attribute Memory): 128 sprite entries, 544 bytes total
 * - OBJSEL register ($2101): sprite size mode and name base address
 * - Sprite tile numbering relative to the OBJSEL name base
 * - DMA transfer of tile data to VRAM and palette data to CGRAM
 *
 * @par What to Observe
 * - A single 32x32 sprite displayed at the center of the screen
 * - No input required; the sprite is static
 *
 * @par Modules Used
 * console, dma, sprite
 *
 * @see sprite.h, dma.h, video.h
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
    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
