/**
 * @file text.c
 * @brief Text rendering implementation for OpenSNES
 *
 * Text is written to a RAM buffer (tilemapBuffer at $7E:3000), then
 * DMA-transferred to VRAM during VBlank. This avoids VRAM write timing
 * issues and removes the need for forced blank during text updates.
 *
 * License: CC0 (Public Domain)
 */

#include "snes/text.h"
#include "snes/registers.h"
#include "snes/dma.h"

/* Font data defined in text_data.asm */
extern const unsigned char opensnes_font_2bpp[];

/* Assembly helpers for performance-critical operations (text.asm) */
extern void asm_textDMAFont(void);
extern void asm_textFillBuffer(u16 value);

/* Tilemap DMA function (defined in crt0.asm) */
extern void tilemapFlush(void);

/* Tilemap RAM buffer — placed by compiler in bank $00 WRAM mirror.
 * The compiler generates `sta.l $0000,x` for array access, which always
 * hits bank $00. Only addresses $0000-$1FFF in bank $00 are WRAM.
 * 2048 bytes = 32×32 tilemap entries × 2 bytes each. */
u8 tilemapBuffer[2048];

/* DMA control variables (defined in crt0.asm .system RAMSECTION) */
extern volatile u8 tilemap_update_flag;
extern u16 tilemap_vram_addr;
extern u16 tilemap_src_addr;

/* Font size constants */
#define FONT_SIZE 1536  /* 96 chars * 16 bytes per tile */

/* Global text configuration */
TextConfig text_config;

/* Current cursor position */
static u8 cursor_x = 0;
static u8 cursor_y = 0;

/**
 * @brief Build tilemap entry for a character
 */
static u16 build_tile_entry(char c) {
    u16 tile;

    /* Map ASCII to tile number */
    if (c < 32 || c > 127) {
        tile = text_config.font_tile;  /* Space for invalid chars */
    } else {
        tile = text_config.font_tile + (c - 32);
    }

    /* Build tilemap entry: VH0P PPP0 TTTT TTTT */
    /* V=vflip, H=hflip, P=priority, PPP=palette, T=tile */
    return tile |
           ((u16)text_config.palette << 10) |
           ((u16)text_config.priority << 13);
}

/**
 * @brief Write a tilemap entry to the RAM buffer
 *
 * @param x Column position (0-31)
 * @param y Row position (0-31)
 * @param entry 16-bit tilemap entry (tile + attributes)
 */
static void buffer_write_entry(u8 x, u8 y, u16 entry) {
    u16 buf_offset = ((u16)y * text_config.map_width + x) * 2;
    tilemapBuffer[buf_offset]     = entry & 0xFF;
    tilemapBuffer[buf_offset + 1] = entry >> 8;
}

void textInit(void) {
    text_config.tilemap_addr = 0x3800;  /* $7000 in byte address / 2 */
    text_config.font_tile = 0;
    text_config.palette = 0;
    text_config.priority = 0;
    text_config.map_width = 32;

    /* Set DMA source and target for NMI handler */
    tilemap_vram_addr = 0x3800;
    tilemap_src_addr = (u16)tilemapBuffer;

    cursor_x = 0;
    cursor_y = 0;

    /* Fill buffer with spaces and DMA to VRAM (clears garbage tiles) */
    textClear();
    tilemapFlush();
}

void textInitEx(u16 tilemap_addr, u16 font_tile, u8 palette) {
    text_config.tilemap_addr = tilemap_addr >> 1;  /* Convert to word address */
    text_config.font_tile = font_tile;
    text_config.palette = palette & 0x07;
    text_config.priority = 0;
    text_config.map_width = 32;

    /* Set DMA source and target for NMI handler */
    tilemap_vram_addr = tilemap_addr >> 1;
    tilemap_src_addr = (u16)tilemapBuffer;

    cursor_x = 0;
    cursor_y = 0;

    /* Fill buffer with spaces and DMA to VRAM (clears garbage tiles) */
    textClear();
    tilemapFlush();
}

void textLoadFont(u16 vram_addr) {
    /* Set VRAM address (word address) */
    REG_VMAIN = 0x80;
    REG_VMADDL = vram_addr & 0xFF;
    REG_VMADDH = vram_addr >> 8;

    /* DMA font data from ROM to VRAM (assembly helper — ~1500 cycles vs ~180000) */
    asm_textDMAFont();
}

void textSetPos(u8 x, u8 y) {
    cursor_x = x;
    cursor_y = y;
}

u8 textGetX(void) {
    return cursor_x;
}

u8 textGetY(void) {
    return cursor_y;
}

void textPutChar(char c) {
    /* Handle newline */
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }

    /* Handle carriage return */
    if (c == '\r') {
        cursor_x = 0;
        return;
    }

    /* Write tile entry to RAM buffer */
    buffer_write_entry(cursor_x, cursor_y, build_tile_entry(c));

    /* Advance cursor */
    cursor_x++;
    if (cursor_x >= text_config.map_width) {
        cursor_x = 0;
        cursor_y++;
    }
}

void textPrint(const char *str) {
    while (*str) {
        textPutChar(*str++);
    }
}

void textPrintAt(u8 x, u8 y, const char *str) {
    textSetPos(x, y);
    textPrint(str);
}

void textPrintU16(u16 value) {
    char buf[6];  /* Max 65535 = 5 digits + null */
    char *p = buf + 5;
    *p = '\0';

    /* Handle zero case */
    if (value == 0) {
        textPutChar('0');
        return;
    }

    /* Convert digits (reverse order) */
    while (value > 0) {
        p--;
        *p = '0' + (value % 10);
        value /= 10;
    }

    /* Print the string */
    textPrint(p);
}

void textPrintS16(s16 value) {
    if (value < 0) {
        textPutChar('-');
        value = -value;
    }
    textPrintU16((u16)value);
}

void textPrintHex(u16 value, u8 digits) {
    static const char hex_chars[] = "0123456789ABCDEF";
    char buf[5];
    u8 i;

    if (digits > 4) digits = 4;
    buf[digits] = '\0';

    for (i = digits; i > 0; i--) {
        buf[i - 1] = hex_chars[value & 0x0F];
        value >>= 4;
    }

    textPrint(buf);
}

void textClear(void) {
    u16 entry = build_tile_entry(' ');
    /* Assembly fill loop — ~14000 cycles vs ~238000 from compiled C */
    asm_textFillBuffer(entry);
}

void textClearRect(u8 x, u8 y, u8 w, u8 h) {
    textFillRect(x, y, w, h, ' ');
}

void textFillRect(u8 x, u8 y, u8 w, u8 h, char c) {
    u8 row, col;
    u16 entry = build_tile_entry(c);
    u8 lo = entry & 0xFF;
    u8 hi = entry >> 8;
    u16 row_offset;

    /* Compute offset once per row, increment in inner loop */
    for (row = 0; row < h; row++) {
        row_offset = ((u16)(y + row) * text_config.map_width + x) * 2;
        for (col = 0; col < w; col++) {
            tilemapBuffer[row_offset]     = lo;
            tilemapBuffer[row_offset + 1] = hi;
            row_offset += 2;
        }
    }
}

void textDrawBox(u8 x, u8 y, u8 w, u8 h) {
    u8 i;

    /* Corners and edges using ASCII */
    /* Using + for corners, - for horizontal, | for vertical */

    /* Top edge */
    textSetPos(x, y);
    textPutChar('+');
    for (i = 0; i < w - 2; i++) {
        textPutChar('-');
    }
    textPutChar('+');

    /* Side edges */
    for (i = 1; i < h - 1; i++) {
        textSetPos(x, y + i);
        textPutChar('|');
        textSetPos(x + w - 1, y + i);
        textPutChar('|');
    }

    /* Bottom edge */
    textSetPos(x, y + h - 1);
    textPutChar('+');
    for (i = 0; i < w - 2; i++) {
        textPutChar('-');
    }
    textPutChar('+');
}

void textFlush(void) {
    /* Set flag for NMI handler to DMA during next VBlank.
     * This is safe to call anytime — the actual VRAM write
     * happens during VBlank when VRAM access is allowed. */
    tilemap_update_flag = 1;
}
