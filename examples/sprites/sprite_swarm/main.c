/**
 * @file main.c
 * @brief Sprites showcase — a bouncing swarm, and the OAM throughput ceiling
 * @ingroup examples
 *
 * The finale of the sprites family, and an honest one. The SNES has 128
 * hardware sprites and one OAM DMA per frame to move them — but *computing*
 * and *writing* 128 positions in C every frame does not fit a 60 fps
 * budget on the base CPU. The OAM buffer lives in bank $7E (every write is
 * a slow long address) and cc65816 spills registers in a busy loop, so the
 * smooth ceiling for per-sprite C motion is around three dozen sprites.
 *
 * So this swarm runs a comfortable 32 dots at a rock-steady 60 fps, each
 * bouncing independently — and that number *is* the lesson:
 *
 *   moving sprites is cheap in principle (one OAM DMA), but touching each
 *   one from C is not free. For all 128 you either move the work off the
 *   main CPU — `chips/sa1_starfield` runs 128 birds of Lissajous trig on
 *   the SA-1 at 10.74 MHz — or hand-write the update loop in assembly.
 *
 * Motion is deliberately cheap (integer add + edge bounce, no multiply or
 * trig) so the cost you see is the OAM update itself, not the maths.
 * Positions go straight into `oamMemory[]`; one `oam_update_flag` lets the
 * NMI's DMA push them all. Four palettes tint the cloud. Zero assets.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - 128 hardware sprites; OAM is a RAM buffer (`oamMemory[]`) DMA'd to the
 *   PPU once per frame — set `oam_update_flag`, the NMI does the transfer
 * - Direct `oamMemory[]` writes: the fastest per-sprite update, two bytes
 *   (X, Y) when tile/palette/attributes were set once at init
 * - The per-sprite update budget: ~32 sprites of C motion fit 60 fps; the
 *   ceiling is OAM writes (bank $7E long addressing) + loop overhead, not
 *   the sprite hardware. sa1_starfield shows the coprocessor way past it
 *
 * @par What to Observe
 * 32 colourful dots bounce around at a steady 60 fps. `swarm_frame` (probe
 * oracle) advances every frame.
 *
 * @par Modules Used
 * console, dma, background, sprite
 *
 * @see lib/include/snes/sprite.h — oamSetFast, oamMemory, oam_update_flag
 * @see examples/chips/sa1_starfield — 128 sprites of trig, offloaded to the SA-1
 */

#include <snes.h>

/** @brief Sprite count — the smooth-60fps ceiling for per-sprite C motion. */
#define NBIRDS 32

/** @brief Sprite tile VRAM word address. */
#define SPR_VRAM 0x4000

/* Bounce bounds (pixels), kept inside 0..255 / 0..223 so positions fit u8. */
#define XMIN 8
#define XMAX 248
#define YMIN 16
#define YMAX 208

/** @brief Probe oracle: frames elapsed (advances while the flock animates). */
u16 swarm_frame;

/* Per-sprite state. */
static u8 bx[NBIRDS];   /**< X position (u8, always within bounds) */
static u8 by[NBIRDS];   /**< Y position */
static s8 bvx[NBIRDS];  /**< X velocity */
static s8 bvy[NBIRDS];  /**< Y velocity */

/** @brief Scratch 8x8 tile as pixel indices, packed to 4bpp. */
static u8 px[64];
static u8 tilebuf[32];

/** @brief Pack px[] (8x8 palette indices) into a 4bpp planar tile. */
static void encode_4bpp(void) {
    u8 pair, row, col;
    u16 o = 0;
    for (pair = 0; pair < 4; pair += 2) {
        for (row = 0; row < 8; row++) {
            u8 lo = 0, hi = 0;
            for (col = 0; col < 8; col++) {
                u8 v = px[row * 8 + col];
                if (v & (1 << pair))       lo |= (u8)(0x80 >> col);
                if (v & (1 << (pair + 1))) hi |= (u8)(0x80 >> col);
            }
            tilebuf[o++] = lo;
            tilebuf[o++] = hi;
        }
    }
}

/** @brief Build the 8x8 dot (a filled ball), pixel index 1, rest transparent. */
static void build_dot(void) {
    static const u8 ball[8] = {0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};
    u8 r, c;
    for (r = 0; r < 8; r++)
        for (c = 0; c < 8; c++)
            px[r * 8 + c] = (u8)(((ball[r] >> (7 - c)) & 1) ? 1 : 0);
}

int main(void) {
    u16 pal[64];
    u16 i;
    /* Four velocity choices, no multiply needed to pick a lively spread. */
    static const s8 vtab[4] = {-2, -1, 1, 2};

    consoleInit();
    setMode(BG_MODE1, 0);

    /* Four sprite palettes (CGRAM 128+): a colourful cloud. Only colour 1. */
    for (i = 0; i < 64; i++) pal[i] = 0;
    pal[0 * 16 + 1] = RGB(31,  8,  8);   /* red    */
    pal[1 * 16 + 1] = RGB( 8, 28, 31);   /* cyan   */
    pal[2 * 16 + 1] = RGB(31, 28,  8);   /* yellow */
    pal[3 * 16 + 1] = RGB(31, 31, 31);   /* white  */
    dmaCopyCGram((u8 *)pal, OBJ_CGRAM_BASE, 128);

    build_dot();
    encode_4bpp();
    dmaCopyVram(tilebuf, SPR_VRAM, 32);

    oamInit(OBJ_SIZE8_L16, SPR_VRAM >> 13);
    oamClear();

    /* Spread the flock out, give each a velocity, and set its constant OAM
     * attributes once (tile 0, one of four palettes, prio 3). Per frame we
     * then touch only the two position bytes. */
    for (i = 0; i < NBIRDS; i++) {
        bx[i]  = (u8)(((i & 7) << 5) + 12);
        by[i]  = (u8)(((i >> 3) << 5) + 20);
        bvx[i] = vtab[i & 3];
        bvy[i] = vtab[(i >> 2) & 3];
        oamSetFast(i, bx[i], by[i], 0, (u8)(i & 3), 3, 0);
    }

    setMainScreen(LAYER_OBJ);
    setScreenOn();

    swarm_frame = 0;

    while (1) {
        for (i = 0; i < NBIRDS; i++) {
            u16 off = i << 2;
            u8 nx = (u8)(bx[i] + bvx[i]);
            u8 ny = (u8)(by[i] + bvy[i]);
            if (nx < XMIN || nx > XMAX) { bvx[i] = (s8)(-bvx[i]); nx = (u8)(bx[i] + bvx[i]); }
            if (ny < YMIN || ny > YMAX) { bvy[i] = (s8)(-bvy[i]); ny = (u8)(by[i] + bvy[i]); }
            bx[i] = nx;
            by[i] = ny;
            /* Direct OAM buffer write: X, then Y-1 (PPU +1 scanline quirk). */
            oamMemory[off + 0] = nx;
            oamMemory[off + 1] = (u8)(ny - 1);
        }
        oam_update_flag = 1;
        swarm_frame = (u16)(swarm_frame + 1);
        WaitForVBlank();
    }

    return 0;
}
