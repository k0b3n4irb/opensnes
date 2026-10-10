/**
 * @file text.c
 * @brief Text rendering implementation for OpenSNES
 *
 * Text is written to a RAM buffer (tilemapBuffer, in the plain C RAM band), then
 * DMA-transferred to VRAM during VBlank. This avoids VRAM write timing
 * issues and removes the need for forced blank during text updates.
 *
 * License: CC0 (Public Domain)
 */

#include "snes.h"
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

/* Current cursor position. Prefixed and out of text.h since 2026-10-03
 * (API decision D4): the names were `cursor_x` / `cursor_y`, which a game
 * would collide with at link time. Read them with textGetX() / textGetY(). */
u8 text_cursor_x = 0;
u8 text_cursor_y = 0;

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

void textInit(u16 tilemap_addr, u16 font_tile, u8 palette) {
    /* tilemap_addr is a VRAM WORD address — same unit as bgSetMapPtr,
     * bgSetGfxPtr, dmaCopyVram, textLoadFont and the rest of the SDK.
     * (Before v0.23.0 this was a byte address; the >>1 conversion is gone.) */
    text_config.tilemap_addr = tilemap_addr;
    text_config.font_tile    = font_tile;
    text_config.palette      = palette & 0x07;
    text_config.priority     = 0;
    text_config.map_width    = 32;

    /* DMA target for NMI handler is the same word address. */
    tilemap_vram_addr = tilemap_addr;
    tilemap_src_addr  = (u16)tilemapBuffer;

    text_cursor_x = 0;
    text_cursor_y = 0;

    /* Fill buffer with spaces and DMA to VRAM (clears garbage tiles). */
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
    text_cursor_x = x & 31;
    text_cursor_y = y & (TEXT_MAP_ROWS - 1);
}

u8 textGetX(void) {
    return text_cursor_x;
}

u8 textGetY(void) {
    return text_cursor_y;
}

void textPutChar(char c) {
    /* Handle newline */
    if (c == '\n') {
        text_cursor_x = 0;
        text_cursor_y++;
        if (text_cursor_y >= TEXT_MAP_ROWS) text_cursor_y = 0;   /* wrap: buffer is 32 rows */
        return;
    }

    /* Handle carriage return */
    if (c == '\r') {
        text_cursor_x = 0;
        return;
    }

    /* Write tile entry to RAM buffer */
    buffer_write_entry(text_cursor_x, text_cursor_y, build_tile_entry(c));

    /* Advance cursor */
    text_cursor_x++;
    if (text_cursor_x >= text_config.map_width) {
        text_cursor_x = 0;
        text_cursor_y++;
        if (text_cursor_y >= TEXT_MAP_ROWS) text_cursor_y = 0;   /* wrap: buffer is 32 rows */
    }

    /* Auto-flush: NMI handler will DMA the tilemap during the next
     * VBlank. Removes the "you must call textFlush" footgun. The flag
     * is a single byte; setting it on every glyph is essentially free
     * (NMI does at most one tilemap DMA per frame regardless of how
     * many text writes piled the flag back to 1). */
    tilemap_update_flag = 1;
}

void textPrint(const char *str) {
    /* A run of printable characters that stays on its row is written here,
     * with the buffer position computed once; a newline, a carriage return,
     * a character the font does not have and the last column (which wraps)
     * go through textPutChar(). Until 2026-10-09 every character did: three
     * calls and a multiply each. The map is 32 columns wide (text.h). */
    u16 attr = ((u16)text_config.palette << 10) | ((u16)text_config.priority << 13);
    u16 font = text_config.font_tile - 32;
    u16 *cell;
    u8 x;
    u8 c = (u8)*str;

    while (c) {
        x = text_cursor_x;
        if (c < 32 || c > 127 || x >= 31) {
            textPutChar((char)c);
            c = (u8)*++str;
            continue;
        }
        cell = (u16 *)(tilemapBuffer + ((((u16)text_cursor_y << 5) + x) << 1));
        do {
            *cell++ = (font + c) | attr;
            x++;
            c = (u8)*++str;
        } while (c >= 32 && c <= 127 && x < 31);
        text_cursor_x = x;
        tilemap_update_flag = 1;
    }
}

void textPrintAt(u8 x, u8 y, const char *str) {
    textSetPos(x, y);
    textPrint(str);
}

void textPrintU16(u16 value) {
    /* Print most-significant digit first, no reversed stack buffer / pointer.
     * (The reversed-buffer form miscompiled under some link layouts — see
     * .claude/notes/tech/textprintu16_codegen_hang.md.) */
    u16 place = 10000;
    u8 started = 0;

    if (value == 0) {
        textPutChar('0');
        return;
    }
    while (place > 0) {
        u8 digit = (u8)(value / place);
        if (digit != 0 || started) {
            textPutChar((char)('0' + digit));
            started = 1;
        }
        value = (u16)(value % place);
        place = (u16)(place / 10);
    }
}

void textPrintHex(u16 value, u8 digits) {
    /* RAM, not const: lib C modules carry no ROM const data (a spilled
     * .rodata section reads as garbage from bank $01+ — symmap ratchet). */
    static char hex_chars[] = "0123456789ABCDEF";
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
    tilemap_update_flag = 1;   /* auto-flush */
}

/* Fill a rectangle with character c. Internal — only `textClearRect`
 * uses it. Removed from the public header in chantier T.1: applications
 * with non-trivial rectangle painting needs are better served by
 * iterating textPrintAt themselves than by a one-purpose helper that
 * was unused outside the lib. */
static void textFillRect(u8 x, u8 y, u8 w, u8 h, char c) {
    u8 row, col;
    u16 entry = build_tile_entry(c);
    u8 lo = entry & 0xFF;
    u8 hi = entry >> 8;
    u16 row_offset;

    /* Clamp to the 32×32 buffer — a rectangle reaching past the edge must
     * not write outside tilemapBuffer. */
    if (x >= text_config.map_width || y >= TEXT_MAP_ROWS) return;
    if ((u16)x + w > text_config.map_width) w = text_config.map_width - x;
    if ((u16)y + h > TEXT_MAP_ROWS)         h = TEXT_MAP_ROWS - y;

    /* Compute offset once per row, increment in inner loop */
    for (row = 0; row < h; row++) {
        row_offset = ((u16)(y + row) * text_config.map_width + x) * 2;
        for (col = 0; col < w; col++) {
            tilemapBuffer[row_offset]     = lo;
            tilemapBuffer[row_offset + 1] = hi;
            row_offset += 2;
        }
    }

    tilemap_update_flag = 1;   /* auto-flush */
}

void textClearRect(u8 x, u8 y, u8 w, u8 h) {
    textFillRect(x, y, w, h, ' ');
}

void textFlush(void) {
    /* Set flag for NMI handler to DMA during next VBlank.
     * This is safe to call anytime — the actual VRAM write
     * happens during VBlank when VRAM access is allowed. */
    tilemap_update_flag = 1;
}

void textModeInit(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    setColor(0, 0x0000);
    setColor(1, RGB(31, 31, 31));
    textInit(TEXT_DEFAULT_TILEMAP_ADDR, TEXT_DEFAULT_FONT_TILE, TEXT_DEFAULT_PALETTE);
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);
    setMainScreen(LAYER_BG1);
}
