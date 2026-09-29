/**
 * @file main.c
 * @brief Super FX game skeleton — a game loop at 60 fps while the GSU renders
 * @ingroup examples
 *
 * The shape of a Super FX game: the CPU runs the game every frame (here, a
 * crosshair steered with the D-pad), the GSU renders the 3D view on its own
 * clock (here, superfx_3d's rotating cube), and the NMI moves each finished
 * frame to VRAM. Nothing waits for anything else.
 *
 * - The renderer runs from the GSU's code cache (gsuCacheLoad /
 *   gsuStartCached): the ROM stays the CPU's, so the game loop runs during
 *   the job.
 * - Two framebuffers in Game Pak RAM, two char blocks in VRAM: gsuPresent()
 *   queues the frame the job just drew, the next job draws in the other
 *   buffer, and the NMI moves the queued one a piece per VBlank, taking
 *   Game Pak RAM from the GSU for each piece. BG1 switches to the new block
 *   only once all of it has landed.
 * - A 40 + 40 line letterbox (gsuSetupHdmaBlanking) lengthens the blank the
 *   NMI transfers in: a 16 KB frame lands in two VBlanks, 30 fps.
 *
 * @par SNES Concepts
 * - Super FX code cache: a job that leaves the ROM to the CPU
 * - Game Pak RAM time-sharing (SCMR RAN) between the GSU and the NMI's DMA
 * - Double buffering by BG1 char base (BG12NBA), swapped after a whole frame
 * - HDMA letterbox on INIDISP to extend the VBlank transfer window
 * - OBJ layer over a GSU bitmap BG
 *
 * @par What to Observe
 * - The cube rotates at 30 fps; the crosshair follows the D-pad at 60 fps,
 *   whatever the GSU is doing.
 * - No torn or half-drawn cube: a frame is shown only once complete.
 *
 * @par Modules Used
 * console, sprite, dma, background, input, superfx
 *
 * @see superfx.h (gsuPresent, gsuStartCached), docs/tutorials/superfx.md
 */

#include <snes.h>
#include <snes/superfx.h>

/* gsu_loader.asm */
extern const u8 gsu_cube[], gsu_cube_end[];   /**< the cache-resident renderer */
extern void writeEdgesToSRAM(void);
extern u8 edge_buffer[48];

/** @brief Letterbox bands (lines): the NMI moves frames in VBlank + top band */
#define LETTERBOX_TOP    40
/** @brief Bottom band: at least one line keeps force blank across line 0 */
#define LETTERBOX_BOTTOM 40

/** @brief VRAM word addresses: two BG1 char blocks, the map, the sprite */
#define VRAM_BG1_A   0x0000
#define VRAM_BG1_B   0x2000
#define VRAM_BG1_MAP 0x4000
#define VRAM_OBJ     0x6000

/** @brief The crosshair: one 8x8 4bpp tile, colour 1 (original art) */
static const u8 crosshair_tile[32] = {
    0x18, 0x00, 0x18, 0x00, 0x18, 0x00, 0xE7, 0x00,   /* planes 0/1, rows 0-3 */
    0xE7, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00,   /* rows 4-7 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* planes 2/3 */
};

/** @brief OBJ palette 0: transparent, then the crosshair's yellow */
static const u16 crosshair_pal[16] = {
    RGB(0,0,0), RGB(31,31,0),
};

/** @brief Crosshair position (the game's state) */
u16 r_ship_x, r_ship_y;
/** @brief Game-loop frames (one per WaitForVBlank) */
u16 r_game_frames;
/** @brief GSU jobs started */
u16 r_jobs;

/** @brief Sine table, 256 steps per turn, amplitude 127 */
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

/** @brief Cube vertices, x */
static const s16 cvx[] = {-25, 25, 25,-25,-25, 25, 25,-25};
/** @brief Cube vertices, y */
static const s16 cvy[] = {-25,-25, 25, 25,-25,-25, 25, 25};
/** @brief Cube vertices, z */
static const s16 cvz[] = {-25,-25,-25,-25, 25, 25, 25, 25};

/** @brief The 12 edges, as vertex index pairs */
static const u8 ce[] = {
    0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7,
};

/** @brief BG palette 0: the GSU bitmap colours */
static const u16 cube_pal[] = {
    RGB(0,0,4), RGB(0,0,10), RGB(0,0,16), RGB(0,0,24),
    RGB(0,10,31), RGB(0,20,31), RGB(0,31,31), RGB(0,31,16),
    RGB(16,31,0), RGB(31,31,0), RGB(31,16,0), RGB(31,0,0),
    RGB(31,0,16), RGB(31,0,31), RGB(20,20,20), RGB(31,31,31),
};

/** @brief Projected vertices, x (0-255) */
u8 g_px[8];
/** @brief Projected vertices, y (0-127) */
u8 g_py[8];
/** @brief Sines and cosines of the two rotation angles */
s16 g_say, g_cay, g_sax, g_cax;

/** @brief Rotate one vertex, store projected screen coords (clamped) */
void rotateVertex(u16 idx) {
    s16 x, y, z, rx, rz, ry2, px_val, py_val;
    x = cvx[idx];
    y = cvy[idx];
    z = cvz[idx];

    /* Y-axis rotation */
    rx = (x * g_cay - z * g_say) / 128;
    rz = (x * g_say + z * g_cay) / 128;

    /* X-axis rotation — ry2 can reach ±64 due to rz amplification */
    ry2 = (y * g_cax - rz * g_sax) / 128;

    /* Project + clamp to framebuffer bounds (0-255 X, 0-127 Y) */
    px_val = 128 + rx;
    if (px_val < 1) px_val = 1;
    if (px_val > 254) px_val = 254;
    g_px[idx] = (u8)px_val;

    /* Y=64: center of framebuffer. BG scroll handles screen centering. */
    py_val = 64 + ry2;
    if (py_val < 1) py_val = 1;
    if (py_val > 126) py_val = 126;
    g_py[idx] = (u8)py_val;
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

/** @brief Rotate the cube to the given angles and upload its edges */
static void prepareFrame(u8 angle_y, u8 angle_x) {
    u16 i;

    g_say = sin_tab[angle_y];
    g_cay = sin_tab[(u8)(angle_y + 64)];
    g_sax = sin_tab[angle_x];
    g_cax = sin_tab[(u8)(angle_x + 64)];
    for (i = 0; i < 8; i++)
        rotateVertex(i);
    buildEdges();
    writeEdgesToSRAM();          /* the CPU owns Game Pak RAM: no job running */
}

/** @brief Start the renderer from the code cache; returns at once */
static void startJob(void) {
    gsuCacheLoad(gsu_cube, (u16)(gsu_cube_end - gsu_cube));
    gsuStartCached(0);
    r_jobs++;
}

int main(void) {
    u8 angle_y = 0, angle_x = 0;
    u16 pad;

    consoleInit();
    setMode(BG_MODE1, 0);
    dmaCopyCGram((u8 *)cube_pal, 0, 32);
    dmaCopyCGram((u8 *)crosshair_pal, OBJ_CGRAM_BASE, 32);
    dmaCopyVram(crosshair_tile, VRAM_OBJ, 32);
    oamInit(OBJ_SIZE8_L16, VRAM_OBJ >> 13);
    r_ship_x = 124;
    r_ship_y = 108;
    oamSet(0, r_ship_x, r_ship_y, 0, 0, 3, 0);
    bgSetGfxPtr(0, VRAM_BG1_A);
    bgSetMapPtr(0, VRAM_BG1_MAP, BG_MAP_32x32);

    if (!gsuInit()) {
        setScreenOn();
        while (1) { WaitForVBlank(); }
    }
    /* gsuInit's defaults: 4bpp, 128 lines, RAN + RON (gsuStartCached drops
     * RON). Buffer A at $70:0000, B right after it at $70:4000. */
    gsuSetupBitmapTilemap(VRAM_BG1_MAP);
    gsu_scbr = 0x00;
    gsuPresentInit(VRAM_BG1_A, VRAM_BG1_B, 0);
    /* the 128-line bitmap in the middle of the 144 visible lines */
    bgSetScroll(0, 0, 208);

    /* First frame, drawn and landed before the screen comes on */
    prepareFrame(angle_y, angle_x);
    startJob();
    gsuWait();
    gsuPresent();
    gsuPresentWait();

    gsuSetupHdmaBlanking(LETTERBOX_TOP, LETTERBOX_BOTTOM);
    setMainScreen(LAYER_BG1 | LAYER_OBJ);
    setScreenOn();

    while (1) {
        /* The GSU side: when the job is done and the previous frame is on
         * screen, queue the new one and start the next job. Otherwise the
         * game just goes on. */
        if (!gsuBusy() && !gsuPresentBusy()) {
            gsuWait();               /* Game Pak RAM back to the CPU */
            gsuPresent();            /* the NMI moves it; gsu_scbr flips */
            angle_y += 4;
            angle_x += 2;
            prepareFrame(angle_y, angle_x);
            startJob();
        }

        /* The game side: every frame, whatever the GSU does */
        pad = padHeld(0);
        if ((pad & KEY_LEFT) && r_ship_x > 0)
            r_ship_x--;
        if ((pad & KEY_RIGHT) && r_ship_x < 248)
            r_ship_x++;
        if ((pad & KEY_UP) && r_ship_y > LETTERBOX_TOP + 1)
            r_ship_y--;
        if ((pad & KEY_DOWN) && r_ship_y < 224 - LETTERBOX_BOTTOM - 8)
            r_ship_y++;
        oamSetXY(0, r_ship_x, r_ship_y);
        r_game_frames++;
        WaitForVBlank();
    }
    return 0;
}
