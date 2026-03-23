/**
 * @file main.c
 * @brief SuperFX Bitmap — GSU renders rainbow gradient via PLOT
 * @ingroup examples
 *
 * The GSU renders 16 horizontal color bars across a 256x128 pixel
 * framebuffer using the hardware PLOT instruction. The SNES CPU
 * DMAs the result from SRAM to VRAM for display.
 *
 * @par SNES Concepts
 * - SuperFX PLOT instruction for pixel rendering in bitplane format
 * - Column-major tile layout (PLOT stores tiles column-by-column)
 * - GSU-to-SRAM-to-DMA-to-VRAM rendering pipeline
 * - SCMR configuration (4bpp, height 128, bus ownership)
 *
 * @par What to Observe
 * - 16 horizontal rainbow color bars (black, red, orange, ... white)
 * - Each bar is 8 pixels tall, spanning the full 256-pixel width
 * - Rendered entirely by the SuperFX GSU at 10.74 MHz
 *
 * @par Modules Used
 * console, sprite, dma, background, superfx
 *
 * @see examples/memory/superfx_hello for GSU boot diagnostic
 */

#include <snes.h>
#include <snes/superfx.h>

extern void launchGSU(void);
extern void dmaBitmapToVRAM(void);
extern void setupBitmapTilemap(void);

static const u16 rainbow_pal[] = {
    RGB( 0, 0, 0), RGB(31, 0, 0), RGB(31,16, 0), RGB(31,31, 0),
    RGB(16,31, 0), RGB( 0,31, 0), RGB( 0,31,16), RGB( 0,31,31),
    RGB( 0,16,31), RGB( 0, 0,31), RGB(16, 0,31), RGB(31, 0,31),
    RGB(31, 0,16), RGB(20,20,20), RGB(10,10,10), RGB(31,31,31),
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

    /* GSU renders gradient to SRAM */
    launchGSU();

    /* DMA from SRAM to VRAM */
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
