/**
 * @file main.c
 * @brief SuperFX 3D — auto-rotating wireframe cube (2-axis rotation)
 * @ingroup examples
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

static const s8 sin_tab[256] = {
      0,   3,   6,   9,  12,  16,  19,  22,  25,  28,  31,  34,  37,  40,  43,  46,
     49,  51,  54,  57,  60,  63,  65,  68,  71,  73,  76,  78,  81,  83,  85,  88,
     90,  92,  94,  96,  98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
    117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127,
    127, 127, 127, 127, 126, 126, 126, 125, 125, 124, 123, 122, 122, 121, 120, 118,
    117, 116, 115, 113, 112, 111, 109, 107, 106, 104, 102, 100,  98,  96,  94,  92,
     90,  88,  85,  83,  81,  78,  76,  73,  71,  68,  65,  63,  60,  57,  54,  51,
     49,  46,  43,  40,  37,  34,  31,  28,  25,  22,  19,  16,  12,   9,   6,   3,
      0,  -3,  -6,  -9, -12, -16, -19, -22, -25, -28, -31, -34, -37, -40, -43, -46,
    -49, -51, -54, -57, -60, -63, -65, -68, -71, -73, -76, -78, -81, -83, -85, -88,
    -90, -92, -94, -96, -98,-100,-102,-104,-106,-107,-109,-111,-112,-113,-115,-116,
   -117,-118,-120,-121,-122,-122,-123,-124,-125,-125,-126,-126,-126,-127,-127,-127,
   -127,-127,-127,-127,-126,-126,-126,-125,-125,-124,-123,-122,-122,-121,-120,-118,
   -117,-116,-115,-113,-112,-111,-109,-107,-106,-104,-102,-100, -98, -96, -94, -92,
    -90, -88, -85, -83, -81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51,
    -49, -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16, -12,  -9,  -6,  -3,
};

static const s16 cvx[] = {-25, 25, 25,-25,-25, 25, 25,-25};
static const s16 cvy[] = {-25,-25, 25, 25,-25,-25, 25, 25};
static const s16 cvz[] = {-25,-25,-25,-25, 25, 25, 25, 25};

static const u8 ce[] = {
    0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7,
};

static const u16 cube_pal[] = {
    RGB(0,0,4), RGB(0,0,10), RGB(0,0,16), RGB(0,0,24),
    RGB(0,10,31), RGB(0,20,31), RGB(0,31,31), RGB(0,31,16),
    RGB(16,31,0), RGB(31,31,0), RGB(31,16,0), RGB(31,0,0),
    RGB(31,0,16), RGB(31,0,31), RGB(20,20,20), RGB(31,31,31),
};

/* Globals for rotation (avoid stack overflow) */
u8 g_px[8];
u8 g_py[8];
s16 g_say, g_cay, g_sax, g_cax;

/** @brief Rotate one vertex, store projected screen coords */
void rotateVertex(u16 idx) {
    s16 x, y, z, rx, rz, ry2;
    x = cvx[idx];
    y = cvy[idx];
    z = cvz[idx];

    /* Y-axis: rx = (x*cos - z*sin) / 128 */
    rx = (x * g_cay - z * g_say) / 128;
    rz = (x * g_say + z * g_cay) / 128;

    /* X-axis: ry2 = (y*cos - rz*sin) / 128 */
    ry2 = (y * g_cax - rz * g_sax) / 128;

    g_px[idx] = (u8)(128 + rx);
    g_py[idx] = (u8)(64 + ry2);
}

/** @brief Build edge buffer from projected vertices */
void buildEdges(void) {
    u16 i;
    u8 v0, v1;
    for (i = 0; i < 12; i++) {
        v0 = ce[i * 2];
        v1 = ce[i * 2 + 1];
        edge_buffer[i * 4 + 0] = g_px[v0];
        edge_buffer[i * 4 + 1] = g_py[v0];
        edge_buffer[i * 4 + 2] = g_px[v1];
        edge_buffer[i * 4 + 3] = g_py[v1];
    }
}

int main(void) {
    u8 angle_y, angle_x;
    u16 i;

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
    angle_y = 0;
    angle_x = 0;
    setScreenOn();

    while (1) {
        g_say = sin_tab[angle_y];
        g_cay = sin_tab[(angle_y + 64) & 0xFF];
        g_sax = sin_tab[angle_x];
        g_cax = sin_tab[(angle_x + 64) & 0xFF];

        for (i = 0; i < 8; i++) {
            rotateVertex(i);
        }

        buildEdges();
        writeEdgesToSRAM();
        launchGSU();

        WaitForVBlank();
        setScreenOff();
        dmaBitmapToVRAM();
        setScreenOn();

        angle_y += 2;
        angle_x += 1;
    }
    return 0;
}
