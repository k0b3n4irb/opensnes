/**
 * @file main.c
 * @brief HiColor × pseudo-hires — 1792 palette slots at 512 pixels
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s HiColor64PerTileRowPseudoHiRes demo
 * (PeterLemon/SNES, PPU/HDMA/HiColor64PerTileRowPseudoHiRes) — the
 * integration of the two surfaces the effects arc landed separately:
 * the per-scanline H-IRQ CGRAM stream (hicolor_1792's technique) runs
 * UNDER pseudo-hires (SETINI bit 3). A 512x224 image is split by column
 * parity — odd columns on BG1 (main screen), even columns on BG2 (sub
 * screen) — and the PPU re-interleaves them into 512 horizontal pixels
 * while the palettes reload per tile row.
 *
 * Color math is part of krom's recipe too: ADD-half of the subscreen
 * into BG1+backdrop softens the column fringing (CGWSEL $02,
 * CGADSUB $61) — the first colormath dogfood in the corpus.
 *
 * @par SNES Concepts
 * - videoSetPseudoHires(): 512px in ANY mode by main/sub interleaving
 * - H-timer IRQ CGRAM streaming (see hicolor_1792) at 512px
 * - Shared tilemap for two BGs whose tiles differ (column split)
 * - colorMath* composition: ADD + half, source = subscreen
 *
 * @par What to Observe
 * The hicolor sunset, now 512 pixels wide — smooth gradients AND
 * doubled horizontal detail. Fringing on hard edges is the pseudo-hires
 * physics (see hires_text's README explainer).
 *
 * @par Modules Used
 * console, dma, background, colormath
 *
 * @see https://github.com/PeterLemon/SNES — original demo & converter
 * @see devtools/hicolor64hires.py
 */

#include <snes.h>

extern u8 bg1_tiles[], bg1_tiles_end[];
extern u8 bg2_tiles[], bg2_tiles_end[];
extern u8 hicolor_pal[], hicolor_pal_end[];
extern void hicolorIrqStream(void);

#define VRAM_BG1_GFX  0x0000
#define VRAM_BG2_GFX  0x4000
/** @brief Shared map: base word $3C00, loaded at row 4 (VOFS=31) */
#define VRAM_MAP_BASE 0x3C00
#define VRAM_MAP_LOAD 0x3C80

static u16 pal_offset;
static u16 tilemap[896];

/** @brief Same generated map as hicolor_1792 (even rows pal 0-3, odd
 *  rows 4-7, sub-palette per 128px hi-res quarter — both BGs share it) */
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

/** @brief VBlank rewind — identical to hicolor_1792 */
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

    /* krom: BGMODE = %00001001 — Mode 1, priority, BG1+BG2 4bpp */
    setMode(BG_MODE1, 0x08);

    bgSetGfxPtr(0, VRAM_BG1_GFX);
    bgSetGfxPtr(1, VRAM_BG2_GFX);
    bgSetMapPtr(0, VRAM_MAP_BASE, SC_32x32);
    bgSetMapPtr(1, VRAM_MAP_BASE, SC_32x32);

    dmaCopyVram(bg1_tiles, VRAM_BG1_GFX, (u16)(bg1_tiles_end - bg1_tiles));
    dmaCopyVram(bg2_tiles, VRAM_BG2_GFX, (u16)(bg2_tiles_end - bg2_tiles));

    buildTilemap();
    dmaCopyVram((u8 *)tilemap, VRAM_MAP_LOAD, sizeof(tilemap));

    /* krom: both BGs scrolled 31 up — row boundaries meet the CGADD
     * reset cadence of the IRQ stream */
    bgSetScroll(0, 0, 31);
    bgSetScroll(1, 0, 31);

    /* Odd columns through main, even through sub — the interleave */
    setMainScreen(LAYER_BG1);
    setSubScreen(LAYER_BG2);
    videoSetPseudoHires(1);

    /* krom: CGWSEL $02 / CGADSUB $61 — ADD half the subscreen into
     * BG1 + backdrop, softening the column transitions */
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetHalf(1);
    colorMathEnable(COLORMATH_BG1 | COLORMATH_BACKDROP);

    /* DMA ch0 static config for the CGRAM stream (after the last
     * dmaCopyVram — the channel is shared) */
    REG_DMAP(0) = 0x00;
    REG_BBAD(0) = 0x22;
    REG_A1B(0) = (u8)((u32)(void *)hicolor_pal >> 16);
    pal_offset = (u16)(u32)(void *)hicolor_pal;

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
