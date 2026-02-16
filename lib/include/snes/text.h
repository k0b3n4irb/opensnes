/**
 * @file text.h
 * @brief Text rendering functions for OpenSNES
 *
 * Simple text rendering system using 8x8 tile fonts.
 * Text is rendered to a background layer tilemap.
 *
 * License: CC0 (Public Domain)
 */

#ifndef OPENSNES_TEXT_H
#define OPENSNES_TEXT_H

#include <snes/types.h>

/**
 * @brief Text rendering configuration
 */
typedef struct {
    u16 tilemap_addr;   /**< VRAM address of tilemap (word address) */
    u16 font_tile;      /**< First tile number of font in VRAM */
    u8  palette;        /**< Palette number (0-7) */
    u8  priority;       /**< Priority bit (0 or 1) */
    u8  map_width;      /**< Tilemap width (32 or 64 tiles) */
} TextConfig;

/**
 * @brief Default text configuration
 *
 * Assumes font at tile 0, tilemap at $7000, palette 0
 */
extern TextConfig text_config;

/**
 * @brief Initialize text system with default configuration
 *
 * Sets up text_config with reasonable defaults:
 * - Tilemap at $7000 (BG3 default)
 * - Font starting at tile 0
 * - Palette 0, no priority
 * - 32-tile wide tilemap
 */
void textInit(void);

/**
 * @brief Initialize text system with custom configuration
 *
 * @param tilemap_addr VRAM word address for tilemap
 * @param font_tile First tile number of font
 * @param palette Palette number (0-7)
 */
void textInitEx(u16 tilemap_addr, u16 font_tile, u8 palette);

/**
 * @brief Load font tiles to VRAM
 *
 * Loads the built-in OpenSNES font to VRAM.
 * Call during forced blank.
 *
 * @param vram_addr VRAM word address for tiles
 */
void textLoadFont(u16 vram_addr);

/**
 * @brief Set cursor position
 *
 * @param x Column (0-31 or 0-63)
 * @param y Row (0-31 or 0-63)
 */
void textSetPos(u8 x, u8 y);

/**
 * @brief Get current cursor X position
 * @return Current column
 */
u8 textGetX(void);

/**
 * @brief Get current cursor Y position
 * @return Current row
 */
u8 textGetY(void);

/**
 * @brief Print a single character at cursor position
 *
 * Advances cursor. Handles newlines.
 *
 * @param c Character to print (ASCII 32-127)
 */
void textPutChar(char c);

/**
 * @brief Print a string at cursor position
 *
 * @param str Null-terminated string
 */
void textPrint(const char *str);

/**
 * @brief Print a string at specific position
 *
 * @param x Column
 * @param y Row
 * @param str Null-terminated string
 */
void textPrintAt(u8 x, u8 y, const char *str);

/**
 * @brief Print an unsigned integer
 *
 * @param value Number to print
 */
void textPrintU16(u16 value);

/**
 * @brief Print a signed integer
 *
 * @param value Number to print
 */
void textPrintS16(s16 value);

/**
 * @brief Print an unsigned integer in hexadecimal
 *
 * @param value Number to print
 * @param digits Number of hex digits (1-4)
 */
void textPrintHex(u16 value, u8 digits);

/**
 * @brief Clear entire tilemap with spaces
 */
void textClear(void);

/**
 * @brief Clear a rectangular region
 *
 * @param x Start column
 * @param y Start row
 * @param w Width in tiles
 * @param h Height in tiles
 */
void textClearRect(u8 x, u8 y, u8 w, u8 h);

/**
 * @brief Fill a rectangular region with a character
 *
 * @param x Start column
 * @param y Start row
 * @param w Width in tiles
 * @param h Height in tiles
 * @param c Character to fill with
 */
void textFillRect(u8 x, u8 y, u8 w, u8 h, char c);

/**
 * @brief Request tilemap DMA transfer to VRAM
 *
 * Marks the tilemap buffer as dirty. The actual transfer happens
 * during the next VBlank via DMA, ensuring safe VRAM access.
 * Call after modifying text to make changes visible.
 */
void textFlush(void);

/**
 * @brief Draw a box using box-drawing characters
 *
 * Uses ASCII characters for simple box borders.
 *
 * @param x Start column
 * @param y Start row
 * @param w Width (including border)
 * @param h Height (including border)
 */
void textDrawBox(u8 x, u8 y, u8 w, u8 h);

#endif /* OPENSNES_TEXT_H */
