/**
 * @file main.c
 * @brief SuperFX 3D — static wireframe cube (validates full pipeline)
 */

#include <snes.h>
#include <snes/superfx.h>

extern void launchGSU(void);
extern void dmaBitmapToVRAM(void);
extern void setupBitmapTilemap(void);
extern void writeEdgesToSRAM(void);
extern u8 gsu_scbr;
extern u8 gsu_dma_src_hi;
extern u8 edge_buffer[48];

static const u16 cube_pal[] = {
    RGB(0,0,4), RGB(0,0,10), RGB(0,0,16), RGB(0,0,24),
    RGB(0,10,31), RGB(0,20,31), RGB(0,31,31), RGB(0,31,16),
    RGB(16,31,0), RGB(31,31,0), RGB(31,16,0), RGB(31,0,0),
    RGB(31,0,16), RGB(31,0,31), RGB(20,20,20), RGB(31,31,31),
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

    setMainScreen(LAYER_BG1);
    gsu_scbr = 0x00;
    gsu_dma_src_hi = 0x00;

    /* Static cube — hardcoded projected vertices (no rotation)
     * Front face at z=-40: offset by 20px from center
     * Back face at z=+40: offset by -10px (slight perspective illusion)
     * Center: (128, 64)
     */

    /* Front face: 4 edges */
    /* (88,24) → (168,24) */
    edge_buffer[0] = 88;  edge_buffer[1] = 24;
    edge_buffer[2] = 168; edge_buffer[3] = 24;
    /* (168,24) → (168,104) */
    edge_buffer[4] = 168; edge_buffer[5] = 24;
    edge_buffer[6] = 168; edge_buffer[7] = 104;
    /* (168,104) → (88,104) */
    edge_buffer[8] = 168; edge_buffer[9] = 104;
    edge_buffer[10] = 88; edge_buffer[11] = 104;
    /* (88,104) → (88,24) */
    edge_buffer[12] = 88; edge_buffer[13] = 104;
    edge_buffer[14] = 88; edge_buffer[15] = 24;

    /* Back face: 4 edges (smaller, offset) */
    /* (108,34) → (148,34) */
    edge_buffer[16] = 108; edge_buffer[17] = 34;
    edge_buffer[18] = 148; edge_buffer[19] = 34;
    /* (148,34) → (148,94) */
    edge_buffer[20] = 148; edge_buffer[21] = 34;
    edge_buffer[22] = 148; edge_buffer[23] = 94;
    /* (148,94) → (108,94) */
    edge_buffer[24] = 148; edge_buffer[25] = 94;
    edge_buffer[26] = 108; edge_buffer[27] = 94;
    /* (108,94) → (108,34) */
    edge_buffer[28] = 108; edge_buffer[29] = 94;
    edge_buffer[30] = 108; edge_buffer[31] = 34;

    /* Connecting edges: 4 */
    edge_buffer[32] = 88;  edge_buffer[33] = 24;
    edge_buffer[34] = 108; edge_buffer[35] = 34;
    edge_buffer[36] = 168; edge_buffer[37] = 24;
    edge_buffer[38] = 148; edge_buffer[39] = 34;
    edge_buffer[40] = 168; edge_buffer[41] = 104;
    edge_buffer[42] = 148; edge_buffer[43] = 94;
    edge_buffer[44] = 88;  edge_buffer[45] = 104;
    edge_buffer[46] = 108; edge_buffer[47] = 94;

    writeEdgesToSRAM();
    launchGSU();

    WaitForVBlank();
    setScreenOff();
    dmaBitmapToVRAM();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
