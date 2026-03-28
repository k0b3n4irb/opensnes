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

/** @brief 4bpp 32x32 sprite tile data (defined in data.asm, stored in ROM) */
extern u8 sprite32[], sprite32_end[];
/** @brief 16-color palette for the 32x32 sprite */
extern u8 palsprite32[];

/**
 * @brief Entry point: display a single static 32x32 sprite at screen center.
 *
 * This is the simplest possible sprite example. It demonstrates the complete
 * pipeline for getting one hardware sprite on screen:
 *
 * 1. DMA tile data to OBJ VRAM
 * 2. DMA palette to CGRAM (sprite palettes start at address 128)
 * 3. Configure OBJSEL for the desired size mode and name base
 * 4. Set OAM entry with position, tile number, and attributes
 * 5. Enable OBJ layer on the main screen
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    consoleInit();

    /* DMA sprite tiles to VRAM word address $2100.
     * WaitForVBlank() ensures we are in VBlank when the DMA runs, because
     * the PPU silently ignores VRAM writes during active display. */
    WaitForVBlank();
    dmaCopyVram(sprite32, 0x2100, sprite32_end - sprite32);

    /* Load the 16-color sprite palette to CGRAM address 128.
     * CGRAM 0-127 = BG palettes, 128-255 = OBJ (sprite) palettes.
     * 32 bytes = 16 colors x 2 bytes/color (15-bit BGR format). */
    dmaCopyCGram(palsprite32, OBJ_CGRAM_BASE, PALETTE_16_SIZE);

    /* Configure OBJSEL ($2101):
     * - Size mode OBJ_SIZE8_L32: small=8x8, large=32x32
     * - Name base 1: tile data base at VRAM $2000 (base = slot * $2000)
     * Individual sprites choose small or large via the OAM high-table size bit. */
    oamInitEx(OBJ_SIZE8_L32, 1);

    /* Set OAM entry 0 to display the sprite:
     * - Position: (112, 96) = roughly center of 256x224 screen
     * - Tile number 0x0010: calculated as (VRAM_addr - name_base) / 16
     *   = ($2100 - $2000) / 16 = $100 / 16 = 0x10
     * - Palette 0 (first sprite palette), priority 3 (in front of all BGs)
     * - No flip flags */
    oamSet(0, 112, 96, 0x0010, 0, 3, 0);
    /* Set this sprite to use the LARGE size (32x32 in this OBJSEL mode) and
     * make it visible. OBJ_SHOW clears the X high bit 8 (X >= 256 hides sprites). */
    oamSetEx(0, OBJ_LARGE, OBJ_SHOW);

    /* Enable Mode 1 with only the OBJ (sprite) layer visible.
     * No background layers are enabled, so the backdrop color (CGRAM 0) fills
     * the rest of the screen. */
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
