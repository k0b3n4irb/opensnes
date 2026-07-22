/**
 * @file main.c
 * @brief "3840 colors" — RGB channel-split blend across two backgrounds
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s HiColor3840 demo (PeterLemon/SNES,
 * PPU/Blend/HiColor/HiColor3840). The trick: the image's green+blue
 * channels live on BG1 as a 240-color Mode 3 layer (main screen); the
 * red channel lives on BG2 as a 16-level 4bpp layer (sub screen); color
 * math ADDs them back together per pixel — 240 x 16 = "3840 colors"
 * from a static screen, no HDMA and no IRQ.
 *
 * This is the corpus's first pure colormath dogfood: the audit rated
 * colormath.h "good primitives" on inspection; this port proves
 * SetSource/SetOp/Enable against a reference.
 *
 * @par SNES Concepts
 * - RGB channel splitting across main/sub screens
 * - colorMathSetSource(SUBSCREEN) + colorMathSetOp(ADD) +
 *   colorMathEnable(BG1|BACKDROP) — CGWSEL $02 / CGADSUB $21
 * - Mode 3: 8bpp BG1 + 4bpp BG2 sharing VRAM
 *
 * @par What to Observe
 * A shaded color wheel with full-RGB depth. Mentally split it: BG1
 * alone would have no red at all, BG2 alone is a pure red ramp.
 *
 * @par Modules Used
 * console, dma, background, colormath
 *
 * @see https://github.com/PeterLemon/SNES — original demo
 * @see colormath.h
 */

#include <snes.h>

extern u8 gb_tiles[], gb_tiles_end[], gb_map[], gb_map_end[];
extern u8 r_tiles[], r_tiles_end[], r_map[], r_map_end[];
extern u8 blend_pal[], blend_pal_end[];

/** @brief VRAM plan: BG1 8bpp tiles at $0000w, BG2 4bpp at $4000w,
 *  maps at $7800w / $7C00w */
#define VRAM_BG1_GFX 0x0000
#define VRAM_BG2_GFX 0x4000
#define VRAM_BG1_MAP 0x7800
#define VRAM_BG2_MAP 0x7C00

int main(void) {
    consoleInit();

    /* krom: Mode 3, priority — BG1 8bpp (the GB layer), BG2 4bpp (R) */
    setMode(BG_MODE3, 0x08);

    bgSetGfxPtr(0, VRAM_BG1_GFX);
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    bgSetGfxPtr(1, VRAM_BG2_GFX);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);

    /* One 512-byte palette: the red ramp at CGRAM 0-15 (BG2's 4bpp
     * palette 0) and the 240 GB colors at 16-255 (the 8bpp tile
     * indices are pre-offset by +16 in gen_assets.py). */
    dmaCopyCGram(blend_pal, 0, (u16)(blend_pal_end - blend_pal));

    dmaCopyVram(gb_tiles, VRAM_BG1_GFX, (u16)(gb_tiles_end - gb_tiles));
    dmaCopyVram(r_tiles, VRAM_BG2_GFX, (u16)(r_tiles_end - r_tiles));
    dmaCopyVram(gb_map, VRAM_BG1_MAP, (u16)(gb_map_end - gb_map));
    dmaCopyVram(r_map, VRAM_BG2_MAP, (u16)(r_map_end - r_map));

    setMainScreen(LAYER_BG1);
    setSubScreen(LAYER_BG2);

    /* krom: CGWSEL $02 / CGADSUB $21 — ADD the subscreen into BG1 and
     * the backdrop, full strength (no half) */
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetHalf(0);
    colorMathEnable(COLORMATH_BG1 | COLORMATH_BACKDROP);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
