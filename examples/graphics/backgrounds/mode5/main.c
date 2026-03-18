/**
 * @file main.c
 * @brief Mode 5 hi-resolution background display
 * @ingroup examples
 *
 * Demonstrates SNES BG Mode 5 (512x256, 4bpp, 16 colors). Mode 5 doubles
 * the horizontal resolution from 256 to 512 pixels by using interlaced
 * tile data and requiring BG1 on both the main and sub screens. The PPU
 * alternates even/odd pixel columns between the two screens each frame,
 * producing a true 512-pixel-wide output on compatible displays. Tile data
 * must be converted with the special `-M 5` interleaving flag.
 *
 * @par SNES Concepts
 * - BG Mode 5 hi-res rendering (512px horizontal)
 * - Main screen + sub screen cooperation for interlaced output
 * - Mode 5 tile data interleaving (`gfx4snes -M 5`)
 * - DMA tilemap transfer to VRAM
 *
 * @par What to Observe
 * - A single full-screen image rendered at 512-pixel horizontal resolution
 * - Text and fine detail should appear sharper than standard 256px modes
 *
 * @par Modules Used
 * console, dma, background
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>

/** @brief Mode 5 interleaved 4bpp tile data (converted with gfx4snes -M 5).
 *
 * Mode 5 tile data is interleaved: even pixel columns go to the main screen
 * and odd pixel columns go to the sub screen. The PPU composites both screens
 * each frame to produce the 512-pixel-wide output. Standard (non-interleaved)
 * tile data would display incorrectly in Mode 5. */
extern u8 tiles[], tiles_end[];

/** @brief Tilemap data mapping tile indices to BG1's 32x32 grid.
 *
 * Each tilemap entry is 2 bytes: low byte = tile index, high byte = attributes
 * (palette, priority, flip). Loaded to VRAM $6000 to avoid overlapping the
 * tile data region at $0000. */
extern u8 tilemap[], tilemap_end[];

/** @brief 15-bit BGR palette for the Mode 5 background (up to 16 colors). */
extern u8 palette[], palette_end[];

/**
 * @brief Entry point -- sets up Mode 5 hi-res display and loops forever.
 *
 * The setup sequence is:
 * 1. Load interleaved tile data and palette via bgInitTileSet (tiles at VRAM
 *    $0000, palette at CGRAM slot 0).
 * 2. DMA the tilemap to VRAM $6000 (chosen to avoid overlap with tile data).
 * 3. Enable BG Mode 5 and put BG1 on both the main AND sub screens -- this is
 *    the critical step that makes 512px hi-res work. The PPU interleaves even
 *    columns from the main screen with odd columns from the sub screen.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    /* Load tiles + palette to VRAM/CGRAM */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x0000);

    /* Load tilemap to VRAM -- must wait for VBlank because VRAM writes are
     * silently ignored by the PPU during active display (rendering period). */
    WaitForVBlank();
    dmaCopyVram(tilemap, 0x6000, tilemap_end - tilemap);
    bgSetMapPtr(0, 0x6000, SC_32x32);

    /* Mode 5: hi-res, BG1 on both main+sub (required for 512px).
     * Unlike Modes 0-4 where only the main screen is needed, Mode 5
     * REQUIRES BG1 on both screens. Without setSubScreen(LAYER_BG1),
     * every other pixel column would be blank (black). */
    setMode(BG_MODE5, 0);
    setMainScreen(LAYER_BG1);
    setSubScreen(LAYER_BG1);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
