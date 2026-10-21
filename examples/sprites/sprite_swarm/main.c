/**
 * @file main.c
 * @brief Sprites showcase — a swarm of neon moths, and the OAM throughput ceiling
 * @ingroup examples
 *
 * The finale of the sprites family, and an honest one. The SNES has 128
 * hardware sprites and one OAM DMA per frame to move them — but *computing*
 * and *writing* 128 positions in C every frame does not fit a 60 fps
 * budget on the base CPU. The OAM buffer lives in bank $7E (every write is
 * a slow long address) and cc65816 spills registers in a busy loop, so the
 * smooth ceiling for per-sprite C motion is around three dozen sprites.
 *
 * So this swarm runs a comfortable 32 neon moths at a rock-steady 60 fps,
 * each bouncing independently through a dark alley — and that number *is*
 * the lesson:
 *
 *   moving sprites is cheap in principle (one OAM DMA), but touching each
 *   one from C is not free. For all 128 you either move the work off the
 *   main CPU — `chips/sa1_starfield` runs 128 moths of Lissajous trig on
 *   the SA-1 at 10.74 MHz — or hand-write the update loop in assembly.
 *
 * Motion is deliberately cheap (integer add + edge bounce, no multiply or
 * trig) so the cost you see is the OAM update itself, not the maths.
 * Positions go straight into `oamMemory[]`; one `oam_update_flag` lets the
 * NMI's DMA push them all. Four palettes tint the cloud. The moths flap
 * their wings at no per-sprite cost: every one of them is tile 0, and every
 * 8 frames one 32-byte DMA writes the other wing frame over that tile.
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
 * - Animating many sprites at once by rewriting their shared tile in VRAM
 *   (one DMA), not their 32 OAM entries
 *
 * @par What to Observe
 * 32 moths in four colours bounce around the alley at a steady 60 fps, all
 * flapping together. `swarm_frame` (probe oracle) advances every frame.
 *
 * @par Modules Used
 * console, dma, background, sprite, math
 *
 * @see lib/include/snes/sprite.h — oamSetFast, oamMemory, oam_update_flag
 * @see examples/chips/sa1_starfield — 128 sprites of trig, offloaded to the SA-1
 */

#include <snes.h>

#include "res/moth.inc"    /* generated: moth_tiles[] (two 8x8 frames), moth_pal[], each with _end */
#include "res/alley.inc"   /* generated: alley_tiles[], alley_map[], alley_pal[], each with _end */

/** @brief Sprite count — the smooth-60fps ceiling for per-sprite C motion. */
#define NBIRDS 32

/** @brief Sprite tile VRAM word address (after the alley's tiles, $1000-$478F). */
#define SPR_VRAM 0x6000

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

int main(void) {
    u16 pal[64];
    u16 i;
    /* Four velocity choices, no multiply needed to pick a lively spread. */
    static const s8 vtab[4] = {-2, -1, 1, 2};

    consoleInit();
    setMode(BG_MODE1, 0);

    /* The alley behind them: BG1, tiles at $1000, map at $0000, palette 0 */
    bgSetMapPtr(0, 0x0000, SC_32x32);
    bgInitTileSet(0, alley_tiles, alley_pal, 0,
                  alley_tiles_end - alley_tiles,
                  PALETTE_16_SIZE, BG_16COLORS, 0x1000);
    dmaCopyVram(alley_map, 0x0000, alley_map_end - alley_map);

    /* Four sprite palettes (CGRAM 128+): a moth's colours 1 (wing), 2 (its
     * glow) and 3 (the body) in four tints. */
    for (i = 0; i < 64; i++) pal[i] = 0;
    pal[0 * 16 + 1] = RGB(28, 10, 18); pal[0 * 16 + 2] = RGB(31, 22, 27);   /* magenta */
    pal[1 * 16 + 1] = RGB( 9, 26, 28); pal[1 * 16 + 2] = RGB(24, 31, 31);   /* cyan    */
    pal[2 * 16 + 1] = RGB(30, 22, 10); pal[2 * 16 + 2] = RGB(31, 30, 24);   /* amber   */
    pal[3 * 16 + 1] = RGB(18, 12, 28); pal[3 * 16 + 2] = RGB(27, 24, 31);   /* violet  */
    for (i = 0; i < 4; i++) pal[i * 16 + 3] = RGB(3, 2, 5);                /* the body */
    dmaCopyCGram((u8 *)pal, OBJ_CGRAM_BASE, 128);

    /* Tile 0, shared by every moth: wings open (the sheet's first frame) */
    dmaCopyVram(moth_tiles, SPR_VRAM, 32);

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

    setMainScreen(LAYER_BG1 | LAYER_OBJ);
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
            /* Direct OAM buffer write: X, then Y. */
            oamMemory[off + 0] = nx;
            oamMemory[off + 1] = ny;
        }
        oam_update_flag = 1;
        swarm_frame = (u16)(swarm_frame + 1);
        WaitForVBlank();
        /* Every 8 frames, the other wing frame over tile 0: one 32-byte DMA,
         * in VBlank, flaps all 32 moths */
        if ((swarm_frame & 7) == 0)
            dmaCopyVram(moth_tiles + (((swarm_frame >> 3) & 1) << 5), SPR_VRAM, 32);
    }

    return 0;
}
