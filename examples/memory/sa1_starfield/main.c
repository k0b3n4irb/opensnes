/**
 * @file main.c
 * @brief SA-1 Murmuration Demo — 128 dots in Lissajous sine patterns
 * @ingroup examples
 *
 * The SA-1 computes 128 "bird" positions using overlapping Lissajous
 * sine patterns at 10.74 MHz. The main CPU reads positions from I-RAM
 * and displays them as pixel sprites. No assets — generated in code.
 *
 * Each bird's position is the sum of two sine harmonics per axis,
 * with prime-number phase multipliers creating organic flock spread:
 *   X = sin[bird*3 + frame]/2 + sin[bird*7 + frame*2]/4 + 32
 *   Y = sin[bird*5 + frame+64]/2 + sin[bird*11 + frame*3]/4 + 16
 *
 * @par What to Observe
 * - 128 dots moving in smooth, coordinated flock-like patterns
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

/** 2x2 pixel dot tile (4bpp, 8x8, pixels 3-4 on rows 3-4) */
static const u8 dot_tile[] = {
    /* Bitplanes 0+1 (interleaved, 2 bytes per row) */
    0x00, 0x00,  /* row 0 */
    0x00, 0x00,  /* row 1 */
    0x00, 0x00,  /* row 2 */
    0x18, 0x18,  /* row 3: pixels 3,4 lit */
    0x18, 0x18,  /* row 4: pixels 3,4 lit */
    0x00, 0x00,  /* row 5 */
    0x00, 0x00,  /* row 6 */
    0x00, 0x00,  /* row 7 */
    /* Bitplanes 2+3 */
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x18, 0x18,  /* same pixels, all 4 bitplanes = color 15 */
    0x18, 0x18,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
};

/** Sprite palettes: 4 brightness levels for depth illusion */
static const u16 pal[] = {
    /* Palette 0 (CGRAM 128): bright white */
    0x0000, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
    0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
    /* Palette 1 (CGRAM 144): light gray */
    0x0000, 0x6318, 0x6318, 0x6318, 0x6318, 0x6318, 0x6318, 0x6318,
    0x6318, 0x6318, 0x6318, 0x6318, 0x6318, 0x6318, 0x6318, 0x6318,
    /* Palette 2 (CGRAM 160): medium gray */
    0x0000, 0x4210, 0x4210, 0x4210, 0x4210, 0x4210, 0x4210, 0x4210,
    0x4210, 0x4210, 0x4210, 0x4210, 0x4210, 0x4210, 0x4210, 0x4210,
    /* Palette 3 (CGRAM 176): dim gray */
    0x0000, 0x2108, 0x2108, 0x2108, 0x2108, 0x2108, 0x2108, 0x2108,
    0x2108, 0x2108, 0x2108, 0x2108, 0x2108, 0x2108, 0x2108, 0x2108,
};

int main(void) {
    u16 i;
    u8 sx, sy, pal_bits;

    consoleInit();
    setMode(BG_MODE1, 0);
    setColor(0, RGB(0, 0, 4));  /* dark blue background */

    /* Sprite setup: 4 palettes for depth, tile at VRAM $4000 */
    dmaCopyCGram((u8*)pal, OBJ_CGRAM_BASE, 4 * PALETTE_16_SIZE);
    dmaCopyVram((u8*)dot_tile, 0x4000, 32);
    oamInitEx(OBJ_SIZE8_L16, 0x4000 >> 13);

    oamClear();

    /* High table: all 128 sprites = small size, X high bit clear */
    for (i = 0; i < 32; i++) {
        oamMemory[512 + i] = 0x00;
    }

    setMainScreen(LAYER_OBJ);

    if (!sa1Init()) {
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
