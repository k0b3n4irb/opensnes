/**
 * @file main.c
 * @brief HiColor — 1792 colors on screen from a 4bpp background
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s HiColor64PerTileRow demo
 * (SNES/PPU/HDMA/HiColor64PerTileRow), the canonical "beat the palette
 * ceiling" technique. An H-timer IRQ fires on EVERY scanline and DMAs
 * 16 bytes (8 colors) into CGRAM while the PPU is drawing; over one
 * 8-line tile row that streams a full fresh 64-color set. Even tile
 * rows render from CGRAM colors 0-63 while the stream refills 64-127
 * for the odd row, and vice versa — 28 rows x 64 = 1792 colors from a
 * background mode that nominally allows 128.
 *
 * @par SNES Concepts
 * - H-timer IRQ ($4207/$4200 bit 4) — per-scanline CPU interrupts
 * - Raw IRQ handlers via irqSet() (see irq_stream.asm — ASM only)
 * - General DMA to CGRAM ($2122) during active display
 * - CGRAM double-banking via tilemap palette bits (rows alternate 0-3 / 4-7)
 * - The NMITIMEN shadow: nmiSet/irqEnable compose instead of clobbering
 *
 * @par What to Observe
 * A sunset over water with perfectly smooth gradients — 357 distinct
 * colors on screen (measure any static 4bpp screen: max 128). The top
 * of each 8-pixel band is where the palette race lives; krom's HTIME
 * of 190 places the writes in H-blank.
 *
 * @par Modules Used
 * console, dma, background
 *
 * @see https://github.com/PeterLemon/SNES — original demo & converter
 * @see docs/tutorials/graphics.md
 */

#include <snes.h>

/** @brief 4bpp planar tiles, 896 sequential tiles (28 rows x 32), no dedup */
extern u8 sunset_pic[], sunset_pic_end[];
/** @brief 28 rows x 4 segments x 16 colors BGR555 — streamed by the IRQ */
extern u8 sunset_pal[], sunset_pal_end[];
/** @brief Per-scanline CGRAM streaming IRQ handler (irq_stream.asm) */
extern void hicolorIrqStream(void);

/** @brief BG2 tiles at VRAM word $0000 (28672 bytes) */
#define VRAM_GFX      0x0000
/** @brief BG2 32x32 map base (word address) */
#define VRAM_MAP_BASE 0x3C00
/** @brief Map loads at row 4: BG2VOFS=31 makes screen line 0 show map row 4 */
#define VRAM_MAP_LOAD 0x3C80

/** @brief 16-bit offset of sunset_pal within its bank (IRQ stream source) */
static u16 pal_offset;

/** @brief Generated 32x28 tilemap (krom's fixed pattern — see buildTilemap) */
static u16 tilemap[896];

/**
 * @brief Build krom's tilemap pattern: sequential tiles; even rows use
 * palettes 0-3, odd rows 4-7 (the CGRAM ping-pong); one sub-palette per
 * 64-pixel screen quarter (matches the converter's 64x8 segments).
 */
static void buildTilemap(void) {
    u16 row, col, i = 0;
    for (row = 0; row < 28; row++) {
        u16 palbase = (row & 1) ? 4 : 0;
        for (col = 0; col < 32; col++) {
            tilemap[i] = (u16)(((palbase + (col >> 3)) << 10) | i);
            i++;
        }
    }
}

/**
 * @brief VBlank: rewind the palette stream for the new frame.
 *
 * Reloads CGRAM 0-63 with row 0's colors (128 bytes) and resets the DMA
 * source to the table start. The per-scanline IRQ then streams the
 * remaining 27 rows during active display, the source address
 * auto-advancing across transfers. Mirrors krom's VBLANKIRQ verbatim.
 */
static void hicolorVblank(void) {
    REG_CGADD = 0;
    REG_A1TL(0) = (u8)pal_offset;
    REG_A1TH(0) = (u8)(pal_offset >> 8);
    REG_DASL(0) = 128;
    REG_DASH(0) = 0;
    REG_MDMAEN = 0x01;
}

int main(void) {
    consoleInit();

    /* krom: BGMODE = %00001011 — Mode 3, priority bit set, 8x8 tiles */
    setMode(BG_MODE3, 0x08);

    /* BG2 is the 4bpp layer in Mode 3 (BG1 would be 8bpp) */
    bgSetGfxPtr(1, VRAM_GFX);
    bgSetMapPtr(1, VRAM_MAP_BASE, SC_32x32);

    dmaCopyVram(sunset_pic, VRAM_GFX, (u16)(sunset_pic_end - sunset_pic));

    buildTilemap();
    dmaCopyVram((u8 *)tilemap, VRAM_MAP_LOAD, sizeof(tilemap));

    /* krom: scroll BG2 31 pixels up — aligns tile-row boundaries with the
     * CGADD-reset cadence of the IRQ stream ((scanline & 15) == 8). */
    bgSetScroll(1, 0, 31);

    setMainScreen(LAYER_BG2);

    /* Static DMA channel 0 config (krom's init): write 1 byte to $2122,
     * increment source. The IRQ/VBlank handlers only touch A1T0/DAS0 —
     * MUST come after the last dmaCopyVram, which shares the channel. */
    REG_DMAP(0) = 0x00;
    REG_BBAD(0) = 0x22;
    REG_A1B(0) = (u8)((u32)(void *)sunset_pal >> 16);
    pal_offset = (u16)(u32)(void *)sunset_pal;

    /* Rewind each frame in VBlank; stream each scanline via H-IRQ.
     * BOTH handlers live in SUPERFREE sections that may land outside
     * bank 0 — always pass the real bank (nmiSet/irqSet assume 0). */
    nmiSetBank(hicolorVblank, (u8)((u32)(void *)hicolorVblank >> 16));
    irqSetBank((void *)hicolorIrqStream,
               (u8)((u32)(void *)hicolorIrqStream >> 16));
    irqSetHTimer(190);
    irqEnable(IRQ_HTIMER);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
