/**
 * @file text4bpp.c
 * @brief 4bpp font loading for Mode 1 text rendering
 *
 * Separate module to avoid bloating bank $00 ROM space.
 * Add 'text4bpp' to LIB_MODULES when using textLoadFont4bpp().
 *
 * License: CC0 (Public Domain)
 */

#include "snes/text.h"
#include "snes/registers.h"

/* 4bpp font data and DMA helper defined in text4bpp.asm */
extern const unsigned char opensnes_font_4bpp[];
extern void asm_textDMAFont4bpp(void);

void textLoadFont4bpp(u16 vram_addr) {
    /* Set VRAM address (word address) */
    REG_VMAIN = 0x80;
    REG_VMADDL = vram_addr & 0xFF;
    REG_VMADDH = vram_addr >> 8;

    /* DMA 4bpp font data from ROM to VRAM */
    asm_textDMAFont4bpp();
}
