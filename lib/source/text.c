/**
 * @file text.c
 * @brief Text rendering implementation for OpenSNES
 *
 * License: CC0 (Public Domain)
 */

#include "snes/text.h"
#include "snes/registers.h"
#include "snes/dma.h"

/* Font data defined in text_data.asm */
extern const unsigned char opensnes_font_2bpp[];

/* Font size constants */
#define FONT_SIZE 1536  /* 96 chars * 16 bytes per tile */

/* Global text configuration */
TextConfig text_config;

/* Current cursor position */
static u8 cursor_x = 0;
static u8 cursor_y = 0;

/**
 * @brief Write a word to VRAM at the specified address
 */
static void vram_write_word(u16 addr, u16 data) {
    REG_VMAIN = 0x80;       /* Increment after high byte write */
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;
    REG_VMDATAL = data & 0xFF;
    REG_VMDATAH = data >> 8;
}

/**
 * @brief Calculate tilemap address for position
 */
static u16 calc_tilemap_addr(u8 x, u8 y) {
    return text_config.tilemap_addr + (y * text_config.map_width) + x;
}

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

void textInit(void) {
    text_config.tilemap_addr = 0x3800;  /* $7000 in byte address / 2 */
    text_config.font_tile = 0;
    text_config.palette = 0;
    text_config.priority = 0;
    text_config.map_width = 32;

    cursor_x = 0;
    cursor_y = 0;
}

void textInitEx(u16 tilemap_addr, u16 font_tile, u8 palette) {
    text_config.tilemap_addr = tilemap_addr >> 1;  /* Convert to word address */
    text_config.font_tile = font_tile;
    text_config.palette = palette & 0x07;
    text_config.priority = 0;
    text_config.map_width = 32;

    cursor_x = 0;
    cursor_y = 0;
}

void textLoadFont(u16 vram_addr) {
    u16 i;

    /* Set VRAM address (word address) */
    REG_VMAIN = 0x80;
    REG_VMADDL = vram_addr & 0xFF;
    REG_VMADDH = vram_addr >> 8;

    /* Write font data
     * 2bpp format: 16 bytes per tile
     * 96 characters = 1536 bytes = 768 words
     */
    for (i = 0; i < FONT_SIZE; i += 2) {
        REG_VMDATAL = opensnes_font_2bpp[i];
        REG_VMDATAH = opensnes_font_2bpp[i + 1];
    }
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
    u16 addr;
    u16 entry;

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

    /* Calculate tilemap address */
    addr = calc_tilemap_addr(cursor_x, cursor_y);

    /* Build and write tile entry */
    entry = build_tile_entry(c);
    vram_write_word(addr, entry);

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
    textClearRect(0, 0, text_config.map_width, 32);
}

void textClearRect(u8 x, u8 y, u8 w, u8 h) {
    textFillRect(x, y, w, h, ' ');
}

void textFillRect(u8 x, u8 y, u8 w, u8 h, char c) {
    u8 row, col;
    u16 entry = build_tile_entry(c);

    for (row = 0; row < h; row++) {
        u16 addr = calc_tilemap_addr(x, y + row);

        REG_VMAIN = 0x80;
        REG_VMADDL = addr & 0xFF;
        REG_VMADDH = addr >> 8;

        for (col = 0; col < w; col++) {
            REG_VMDATAL = entry & 0xFF;
            REG_VMDATAH = entry >> 8;
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
