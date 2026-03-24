/**
 * @file main.c
 * @brief SuperFX 3D — wireframe cube (step 1: static square)
 * @ingroup examples
 */

#include <snes.h>
#include <snes/superfx.h>

extern void launchGSU(void);
extern void dmaBitmapToVRAM(void);
extern void setupBitmapTilemap(void);
extern u8 gsu_scbr;
extern u8 gsu_dma_src_hi;

static const u16 cube_pal[] = {
    RGB( 0, 0, 4),   /* 0: dark blue background */
    RGB( 0, 0,10),   RGB( 0, 0,16),   RGB( 0, 0,24),
    RGB( 0,10,31),   RGB( 0,20,31),   RGB( 0,31,31),
    RGB( 0,31,16),   RGB(16,31, 0),   RGB(31,31, 0),
    RGB(31,16, 0),   RGB(31, 0, 0),   RGB(31, 0,16),
    RGB(31, 0,31),   RGB(20,20,20),
    RGB(31,31,31),   /* 15: white — wireframe color */
};

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    dmaCopyCGram((u8*)cube_pal, 0, 32);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x4000, BG_MAP_32x32);
    setupBitmapTilemap();

    if (!gsuInit()) {
        setScreenOn();
        while (1) { WaitForVBlank(); }
    }

    /* Render static square */
    gsu_scbr = 0x00;
    gsu_dma_src_hi = 0x00;
    launchGSU();

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
