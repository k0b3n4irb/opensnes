/**
 * @file main.c
 * @brief SA-1 Murmuration Demo — 128 neon moths in Lissajous sine patterns
 * @ingroup examples
 *
 * The SA-1 computes 128 positions using overlapping Lissajous sine
 * patterns at 10.74 MHz. The main CPU reads them from I-RAM and displays
 * them as a cloud of neon moths over the city's far skyline in the rain.
 *
 * Each bird's position is the sum of two sine harmonics per axis,
 * with prime-number phase multipliers creating organic flock spread:
 *   X = sin[bird*3 + frame]/2 + sin[bird*7 + frame*2]/4 + 32
 *   Y = sin[bird*5 + frame+64]/2 + sin[bird*11 + frame*3]/4 + 16
 *
 * @par SNES Concepts
 * - The SA-1 computes while the main CPU displays: positions written to
 *   shared I-RAM, read by the 65816 each frame
 * - A sync byte in I-RAM as the handshake between the two CPUs
 * - One tiny sprite (a 4x3 moth in an 8x8 tile) for all 128: the cloud
 *   bunches up, and a line shows at most 32 sprites
 * - oamMemory[] written directly, the escape hatch beside oamSet(): 128
 *   positions copied from I-RAM each frame in one loop
 *
 * @par What to Observe
 * - 128 moths moving in smooth, coordinated flock-like patterns over the
 *   skyline
 * - The pattern resembles a murmuration (starling flock)
 * - All math computed by SA-1 coprocessor at 10.74 MHz
 *
 * @par Modules Used
 * console, sprite, dma, background, input, sa1
 */

#include <snes.h>
#include <snes/sa1.h>

#define SA1_SYNC      (*(volatile u8*)0x3001)
#define SA1_STAR_BUF  ((volatile u8*)0x3010)
#define NBIRDS 128

#include "res/mote.inc"      /* generated: mote_tiles[] (a moth far away), mote_pal[], each with _end */
#include "res/skyline.inc"   /* generated: skyline_tiles[], skyline_map[], skyline_pal[], each with _end */

/** Sprite palettes: 4 brightness levels for depth illusion — the wings (1)
 *  in a cyan that dims with distance, the body (2) a touch brighter */
static const u16 pal[] = {
    0x0000, RGB(14, 28, 30), RGB(29, 31, 31), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* near  */
    0x0000, RGB(10, 22, 26), RGB(22, 28, 30), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x0000, RGB( 7, 16, 20), RGB(16, 22, 26), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x0000, RGB( 5, 11, 15), RGB(10, 16, 20), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* far   */
};

int main(void) {
    u16 i;
    u8 sx, sy, pal_bits;

    consoleInit();
    setMode(BG_MODE1, 0);
    /* The far skyline in the rain: BG1, tiles at $0000, map at $3800 */
    bgSetMapPtr(0, 0x3800, SC_32x32);
    bgInitTileSet(0, skyline_tiles, skyline_pal, 0,
                  skyline_tiles_end - skyline_tiles,
                  PALETTE_16_SIZE, BG_16COLORS, 0x0000);
    dmaCopyVram(skyline_map, 0x3800, skyline_map_end - skyline_map);

    /* Sprite setup: 4 palettes for depth, the moth's tile at VRAM $4000 */
    dmaCopyCGram((u8*)pal, OBJ_CGRAM_BASE, 4 * PALETTE_16_SIZE);
    dmaCopyVram(mote_tiles, 0x4000, 32);
    oamInit(OBJ_SIZE8_L16, 0x4000 >> 13);

    oamClear();

    /* High table: all 128 sprites = small size, X high bit clear */
    for (i = 0; i < 32; i++) {
        oamMemory[512 + i] = 0x00;
    }

    setMainScreen(LAYER_BG1 | LAYER_OBJ);

    if (!sa1IsReady()) {
        setScreenOn();
        while (1) { WaitForVBlank(); }
    }

    setScreenOn();

    while (1) {
        while (!SA1_SYNC) { }

        /* Read 128 bird positions from I-RAM → OAM */
        for (i = 0; i < NBIRDS; i++) {
            sx = SA1_STAR_BUF[i * 4];
            sy = SA1_STAR_BUF[i * 4 + 1];

            /* Cycle through 4 brightness palettes for depth effect */
            pal_bits = (i >> 5) & 0x03;

            oamMemory[i * 4 + 0] = sx;
            oamMemory[i * 4 + 1] = sy;
            oamMemory[i * 4 + 2] = 0;                      /* tile 0 */
            oamMemory[i * 4 + 3] = 0x30 | (pal_bits << 1); /* prio 3 */
        }

        SA1_SYNC = 0;

        oam_update_flag = 1;
        WaitForVBlank();
    }
    return 0;
}
