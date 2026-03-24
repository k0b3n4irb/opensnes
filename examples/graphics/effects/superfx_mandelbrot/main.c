/**
 * @file main.c
 * @brief SuperFX Mandelbrot Set — fractal computed by GSU via FMULT + PLOT
 * @ingroup examples
 *
 * The GSU computes the Mandelbrot set using 4.12 fixed-point arithmetic
 * and renders it via PLOT. Static render (no animation).
 * Renders in ~1-2 seconds at 21.47 MHz.
 *
 * @par SNES Concepts
 * - SuperFX FMULT for 16x16 signed fixed-point multiply
 * - PLOT hardware for pixel rendering
 * - LOOP instruction for long-range loops (>128 byte body)
 * - CACHE for ~6x instruction speedup
 *
 * @par What to Observe
 * - Classic Mandelbrot set shape with 16-color fractal palette
 * - Computed entirely by the GSU RISC coprocessor
 *
 * @par Modules Used
 * console, sprite, dma, background, superfx
 */

#include <snes.h>
#include <snes/superfx.h>

extern void launchGSU(void);
extern void dmaBitmapToVRAM(void);
extern void setupBitmapTilemap(void);
extern u8 gsu_scbr;
extern u8 gsu_dma_src_hi;

static const u16 fractal_pal[] = {
    RGB( 0, 0, 0),   RGB( 0, 0,10),   RGB( 0, 0,20),   RGB( 0, 0,31),
    RGB( 0,16,31),   RGB( 0,31,31),   RGB( 0,31,16),   RGB( 0,31, 0),
    RGB(16,31, 0),   RGB(31,31, 0),   RGB(31,16, 0),   RGB(31, 0, 0),
    RGB(31, 0,16),   RGB(31, 0,31),   RGB(20,20,20),   RGB(31,31,31),
};

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    dmaCopyCGram((u8*)fractal_pal, 0, 32);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x4000, BG_MAP_32x32);
    setupBitmapTilemap();

    if (!gsuInit()) {
        setScreenOn();
        while (1) { WaitForVBlank(); }
    }

    /* Render Mandelbrot (takes ~1-2 seconds) */
    gsu_scbr = 0x00;
    gsu_dma_src_hi = 0x00;
    launchGSU();

    /* DMA result to VRAM */
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
