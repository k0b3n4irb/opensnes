/**
 * @file main.c
 * @brief Mode 7 EXTBG: one plane split in two layers, a sprite between them
 * @ingroup examples
 *
 * Mode 7 has a single layer — unless SETINI bit 6 (EXTBG) is set. Then BG2
 * shows the same plane as BG1 (same tilemap, same pixels, same transform),
 * but reads each pixel's bit 7 as a priority bit and bits 0-6 as its colour.
 * Front to back: sprites 3, sprites 2, BG2 pixels with bit 7 set, sprites 1,
 * BG1, sprites 0, BG2 pixels with bit 7 clear. A priority-1 sprite therefore
 * slides over the floor (bit 7 clear) and under the pillars (bit 7 set) of
 * one and the same Mode 7 plane.
 *
 * The ball crosses a row of pillars on its own; press A to switch EXTBG off
 * (BG1 shown instead: one layer, the ball in front of everything) and on.
 *
 * @par SNES Concepts
 * - SETINI ($2133) bit 6 through `mode7SetExtBg()`
 * - Mode 7 EXTBG priority: BG2's high pixels above priority-1 sprites, its
 *   low pixels below every sprite and below BG1 (hence BG2 alone on screen)
 * - One pixel, two colours: 0x83 is colour 3 on BG2 (7-bit) and colour 131
 *   on BG1 (8-bit)
 * - Mode 7 VRAM interleave (`dmaCopyVramMode7`), a plane and its tiles built
 *   at run time in a `FAR` buffer
 *
 * @par What to Observe
 * - The yellow ball passes in front of the green floor and behind the grey
 *   pillars — the same Mode 7 plane, split by bit 7 of its pixels.
 * - Press A: EXTBG off, BG1 on screen, the ball now covers the pillars too.
 *
 * @par Modules Used
 * console, dma, sprite, input, mode7, tile
 *
 * @see mode7.h (mode7SetExtBg), docs/tutorials/mode7.md
 */

#include <snes.h>
#include <snes/mode7.h>
#include <snes/tile.h>   /* tileEncode4bpp, for the ball */

/** @brief Sprite tiles in VRAM (words): name base slot 2, past Mode 7's 32 KB */
#define VRAM_OBJ   0x4000
/** @brief The pillar row and the ball's height on screen */
#define PILLAR_ROW 12

/** @brief Probe oracles: EXTBG on, the ball's x */
u8 extbg_on;
u16 ball_x;

/** @brief The 128 x 128 Mode 7 tilemap, built at run time (16 KB: FAR) */
static FAR u8 m7map[128 * 128];
/** @brief Three 8bpp Mode 7 tiles: floor A, floor B, pillar */
static u8 m7tiles[3 * 64];
/** @brief The ball, 16 x 16 pixels, and one encoded 8 x 8 tile */
static u8 ballpx[256];
static u8 px[64];
static u8 tilebuf[32];

/** @brief CGRAM 0-4 (BG2 and BG1), 131-132 (the pillar as BG1 reads it) */
static const u16 bg_pal[5] = {
    RGB(2, 2, 6), RGB(6, 20, 6), RGB(3, 12, 3), RGB(26, 26, 28), RGB(14, 14, 16),
};
/** @brief OBJ palette 1 (CGRAM 144-159): transparent, yellow, orange */
static const u16 ball_pal[3] = { 0, RGB(31, 28, 4), RGB(31, 16, 2) };

/** @brief Fill the plane: checkered floor, 2 x 2-tile pillars on one row */
static void buildPlane(void) {
    u16 tx, ty, i;

    for (i = 0; i < 64; i++) {
        m7tiles[i] = 1;                        /* floor A: colour 1, bit 7 clear */
        m7tiles[64 + i] = 2;                   /* floor B: colour 2 */
        /* pillar: colour 3 with a colour-4 rim, bit 7 SET (high priority) */
        m7tiles[128 + i] = ((i & 7) == 0 || (i & 7) == 7 || i < 8 || i >= 56)
                           ? 0x84 : 0x83;
    }
    for (ty = 0; ty < 128; ty++)
        for (tx = 0; tx < 128; tx++) {
            u8 t = (u8)(((tx ^ ty) & 1) ? 1 : 0);
            if ((ty == PILLAR_ROW || ty == PILLAR_ROW + 1) && (tx & 4) && (tx & 2) == 0)
                t = 2;
            m7map[ty * 128 + tx] = t;
        }
}

/** @brief Draw a 16 x 16 ball and upload it as sprite tiles 0, 1, 16, 17 */
static void buildBall(void) {
    s16 x, y, dx, dy, d;
    u8 q, r, c;
    static const u16 slot[4] = { 0, 1, 16, 17 };

    for (y = 0; y < 16; y++)
        for (x = 0; x < 16; x++) {
            dx = (s16)(2 * x - 15);
            dy = (s16)(2 * y - 15);
            d = (s16)(dx * dx + dy * dy);       /* (2r)^2: 15^2 = 225 at the edge */
            ballpx[y * 16 + x] = (u8)(d > 225 ? 0 : (d > 140 ? 2 : 1));
        }
    for (q = 0; q < 4; q++) {
        for (r = 0; r < 8; r++)
            for (c = 0; c < 8; c++)
                px[r * 8 + c] = ballpx[((q >> 1) * 8 + r) * 16 + (q & 1) * 8 + c];
        tileEncode4bpp(px, tilebuf);
        dmaCopyVram(tilebuf, (u16)(VRAM_OBJ + slot[q] * 16), 32);
    }
}

/** @brief EXTBG on: BG2 (split by bit 7) on screen; off: BG1 */
static void applyExtBg(void) {
    mode7SetExtBg(extbg_on);
    setMainScreen(extbg_on ? (LAYER_BG2 | LAYER_OBJ) : (LAYER_BG1 | LAYER_OBJ));
}

int main(void) {
    s16 vx = 1;

    consoleInit();
    setScreenOff();

    buildPlane();
    dmaCopyVramMode7(m7map, sizeof(m7map), m7tiles, sizeof(m7tiles));
    dmaCopyCGram((u8 *)bg_pal, 0, sizeof(bg_pal));
    dmaCopyCGram((u8 *)&bg_pal[3], 131, 4);   /* 0x83 / 0x84 as BG1 reads them */
    dmaCopyCGram((u8 *)ball_pal, OBJ_CGRAM_BASE + 16, sizeof(ball_pal));

    buildBall();
    oamInit(OBJ_SIZE8_L16, VRAM_OBJ >> 13);
    ball_x = 8;
    oamSet(0, ball_x, PILLAR_ROW * 8, 0, 1, 1, 0);   /* palette 1, priority 1 */
    oamSetSize(0, OBJ_LARGE);

    setMode(BG_MODE7, 0);
    mode7Init();
    mode7SetScale(0x0100, 0x0100);   /* 1:1 */
    mode7SetAngle(0);
    mode7SetScroll(0, 0);            /* texel row 0 on the first line (mode7Init centres the view) */

    extbg_on = 1;
    applyExtBg();
    setScreenOn();

    while (1) {
        WaitForVBlank();
        if (padPressed(0) & KEY_A) {
            extbg_on ^= 1;
            applyExtBg();
        }
        ball_x = (u16)(ball_x + vx);
        if (ball_x >= 232 || ball_x <= 8)
            vx = (s16)-vx;
        oamSetXY(0, ball_x, PILLAR_ROW * 8);
    }
    return 0;
}
