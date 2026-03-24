/**
 * @file main.c
 * @brief SuperFX Mandelbrot Set — GSU computes fractal via FMULT + PLOT
 * @ingroup examples
 *
 * The GSU computes the Mandelbrot set using 4.12 fixed-point arithmetic
 * and the FMULT instruction (signed 16x16 multiply). Each pixel is colored
 * by its escape iteration count, producing a classic fractal image.
 *
 * @par SNES Concepts
 * - SuperFX FMULT for fixed-point multiplication
 * - PLOT hardware for pixel rendering
 * - Fractal computation on a RISC coprocessor
 *
 * @par What to Observe
 * - Classic Mandelbrot set shape with 16-color palette
 * - Renders in ~1-2 seconds on the GSU at 21.47 MHz
 *
 * @par Modules Used
 * console, sprite, dma, background, superfx
 *
 * @see examples/memory/superfx_hello for FMULT validation
 */

#include <snes.h>
#include <snes/superfx.h>

extern void launchGSU(void);
extern void dmaBitmapToVRAM(void);
extern void setupBitmapTilemap(void);

/** @brief 16-color fractal palette (black → blue → cyan → white → yellow → red) */
static const u16 fractal_pal[] = {
    RGB( 0, 0, 0),   /* 0: black (inside set) */
    RGB( 0, 0,10),   /* 1: dark blue */
    RGB( 0, 0,20),   /* 2: blue */
    RGB( 0, 0,31),   /* 3: bright blue */
    RGB( 0,16,31),   /* 4: azure */
    RGB( 0,31,31),   /* 5: cyan */
    RGB( 0,31,16),   /* 6: spring */
    RGB( 0,31, 0),   /* 7: green */
    RGB(16,31, 0),   /* 8: chartreuse */
    RGB(31,31, 0),   /* 9: yellow */
    RGB(31,16, 0),   /* 10: orange */
    RGB(31, 0, 0),   /* 11: red */
    RGB(31, 0,16),   /* 12: rose */
    RGB(31, 0,31),   /* 13: magenta */
    RGB(20,20,20),   /* 14: light gray */
    RGB(31,31,31),   /* 15: white */
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

    /* GSU computes Mandelbrot (takes ~1-2 seconds) */
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
