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
 * @brief Default tilemap byte address — 32×32 tilemap at VRAM byte $7000.
 *
 * Convenience constant for the most common text setup (BG3 in Mode 0/1
 * with text drawn at the standard tile-map slot). Pass to textInit() if
 * you don't have a project-specific layout. textInit() expects a *byte*
 * VRAM address — the value $7000 is what you'd read off a VRAM map.
 */
#define TEXT_DEFAULT_TILEMAP_ADDR  0x7000
/** @brief Default first font tile (zero — font occupies tiles 0-95). */
#define TEXT_DEFAULT_FONT_TILE     0
/** @brief Default palette slot (palette 0). */
#define TEXT_DEFAULT_PALETTE       0

/**
 * @brief Initialize the text rendering system.
 *
 * Configures the tilemap address, the first font tile, and the palette
 * slot used for text glyphs. Replaces the v1 textInit/textInitEx pair —
 * pass TEXT_DEFAULT_* constants for the previous textInit(void) defaults.
 *
 * @param tilemap_addr VRAM **byte** address for the tilemap
 *                     (use TEXT_DEFAULT_TILEMAP_ADDR for the standard $7000)
 * @param font_tile    Tile number of the first font glyph in VRAM
 *                     (use TEXT_DEFAULT_FONT_TILE for tile 0)
 * @param palette      Palette slot 0-7 (use TEXT_DEFAULT_PALETTE for 0)
 *
 * @warning UNIT MISMATCH (the SDK's one documented inconsistency): this
 *          `tilemap_addr` is a **byte** address, whereas the rest of the SDK —
 *          `dmaCopyVram`, `bgSetGfxPtr`/`bgSetMapPtr`, the `oam*` calls, and even
 *          `textLoadFont()` just below — all take **word** addresses. If you have
 *          a word address `W` (e.g. from a `vram_map.h` or a VRAM map), pass
 *          `W * 2` here. Prefer `TEXT_DEFAULT_TILEMAP_ADDR` when you don't need a
 *          custom slot.
 */
void textInit(u16 tilemap_addr, u16 font_tile, u8 palette);

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
 * @brief Load font as 4bpp tiles for Mode 1 BGs
 *
 * Bitplanes 2-3 are zero; font uses colors 0-3 of its palette slot.
 * Use this instead of textLoadFont() when text is on a 4bpp BG layer
 * (e.g., BG1 or BG2 in Mode 1).
 *
 * Requires 'text4bpp' in LIB_MODULES.
 *
 * @param vram_addr VRAM word address for tiles
 */
void textLoadFont4bpp(u16 vram_addr);

/**
 * @brief Set cursor position
 *
 * @param x Column (0-31 or 0-63)
 * @param y Row (0-31 or 0-63)
 * Inlined for zero-call-overhead access (wave 4 retrofit; uses the
 * relaxed CC_INLINE_MAX_INSTR=16 default).
 */
extern u8 cursor_x;
extern u8 cursor_y;
inline void textSetPos(u8 x, u8 y) {
    cursor_x = x;
    cursor_y = y;
}

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
 * @brief Clear a rectangular region (fills with spaces)
 *
 * @param x Start column
 * @param y Start row
 * @param w Width in tiles
 * @param h Height in tiles
 */
void textClearRect(u8 x, u8 y, u8 w, u8 h);

/**
 * @brief Request tilemap DMA transfer to VRAM (rarely needed).
 *
 * As of chantier T.4 every text writer (`textPutChar`, `textPrint`,
 * `textPrintAt`, `textPrintU16`, `textPrintHex`, `textClear`,
 * `textClearRect`) sets the dirty flag itself, so the NMI handler
 * always flushes the tilemap on the next VBlank without an explicit
 * call.
 *
 * Keep using this only when you wrote to `tilemapBuffer` directly,
 * outside of the `text*` API — for example, if you patched a tile
 * by hand for a custom UI element. Calling it redundantly after a
 * regular `textPrint*` is a no-op.
 */
void textFlush(void);

/**
 * @brief Initialize text display mode — one-call setup for text-based examples
 *
 * Sets up Mode 0, BG1, white-on-black palette, font at VRAM $0000,
 * tilemap at $3800 (32x32). Replaces the 8-line init sequence found
 * in most text-based examples.
 *
 * Call setScreenOn() after this to display.
 *
 * Equivalent to:
 * @code
 * consoleInit();
 * setMode(BG_MODE0, 0);
 * setColor(0, 0x0000);
 * setColor(1, RGB(31, 31, 31));
 * textInit(TEXT_DEFAULT_TILEMAP_ADDR, TEXT_DEFAULT_FONT_TILE, TEXT_DEFAULT_PALETTE);
 * textLoadFont(0x0000);
 * bgSetGfxPtr(0, 0x0000);
 * bgSetMapPtr(0, 0x3800, BG_MAP_32x32);
 * setMainScreen(LAYER_BG1);
 * @endcode
 */
void textModeInit(void);

#endif /* OPENSNES_TEXT_H */
