/**
 * @file main.c
 * @brief SuperFX Bitmap — GSU fills framebuffer, DMA to VRAM
 * @ingroup examples
 *
 * The GSU fills 16KB of SRAM with a tile pattern (unrolled, no branches).
 * The SNES CPU then DMAs the result to VRAM for display.
 *
 * @par SNES Concepts
 * - SuperFX (GSU) PLOT instruction for pixel rendering in bitplane format
 * - GSU-to-SRAM-to-DMA-to-VRAM rendering pipeline
 * - Identity tilemap for framebuffer-style display
 * - SCMR configuration (color depth, screen height, bus ownership)
 *
 * @par What to Observe
 * - Top 128 pixels should be solid green (color 5 from palette)
 * - Proves the full GSU-SRAM-DMA-VRAM pipeline works
 *
 * @par Modules Used
 * console, sprite, dma, background, text, superfx
 *
 * @see examples/memory/superfx_hello for GSU boot diagnostic
 */

#include <snes.h>
#include <snes/superfx.h>

extern void launchGSU(void);
extern void dmaBitmapToVRAM(void);
extern void setupBitmapTilemap(void);

static const u16 rainbow_pal[] = {
    RGB( 0, 0, 0),   RGB(31, 0, 0),   RGB(31,16, 0),   RGB(31,31, 0),
    RGB(16,31, 0),   RGB( 0,31, 0),   RGB( 0,31,16),   RGB( 0,31,31),
    RGB( 0,16,31),   RGB( 0, 0,31),   RGB(16, 0,31),   RGB(31, 0,31),
    RGB(31, 0,16),   RGB(20,20,20),   RGB(10,10,10),   RGB(31,31,31),
};

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    dmaCopyCGram((u8*)rainbow_pal, 0, 32);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x4000, BG_MAP_32x32);
    setupBitmapTilemap();

    if (!gsuInit()) {
        setScreenOn();
        while (1) { WaitForVBlank(); }
    }

    /* GSU fills 16KB of SRAM with $00FF (green tiles) */
    launchGSU();

    /* DMA from SRAM $70:0000 to VRAM $0000 */
    WaitForVBlank();
    setScreenOff();
    dmaBitmapToVRAM();

    setMainScreen(LAYER_BG1);
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
