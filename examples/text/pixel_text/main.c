/**
 * @file main.c
 * @brief Text — proportional text composed at the pixel in a 2bpp layer
 * @ingroup examples
 *
 * The menu screen of a noodle bar's terminal in the Rue basse: a bowl over a
 * message panel (BG2, an asset), the bar's lines in a narrow font inside the
 * panel (BG1, composed here).
 *
 * The `text` module puts one glyph per tile, on the tile grid. A game that
 * wants a narrow font at a 6-pixel pitch, or a label at any x and y, composes
 * its text itself: a 2bpp tile layer kept in RAM, each glyph ORed into it at
 * a pixel offset, the layer sent to VRAM when it changes. The primitive is
 * `tileBlitGlyph2bpp()`: it puts rows of a glyph into one tile and the tile to
 * its right, shifted by 0 to 7 pixels. A glyph placed at (x, y) touches up to
 * four tiles — the row of tiles holding `y` and the one below — so two calls
 * cover it, one per tile row. The text is not an asset: a 5x7 font is declared
 * as bits and turned into 2bpp glyphs at boot.
 *
 * @par SNES Concepts
 * - A 2bpp tile is 16 bytes, planes 0 and 1 interleaved per row; a layer of
 *   N tiles is N x 16 bytes, and tile n of the layer is tile n of the sheet
 * - The tilemap only places tiles: the pixels come from the layer, so text at
 *   any position is a matter of writing the right bytes of the right tiles
 * - A glyph at x covers tile x / 8 and, shifted by x % 8, the tile after it;
 *   at y it covers rows y % 8 to 7 of one tile row and 0 to y % 8 - 1 of the next
 * - The layer lives in FAR RAM (2.5 KB) and is cleared with memset() before a
 *   new layout; the upload is split over two VBlanks (1280 bytes each)
 * - Mode 0 gives each BG its own palettes: the layer uses CGRAM 0-3, the
 *   terminal behind it CGRAM 32-35, and colour 0 of both is transparent
 *
 * @par What to Observe
 * - Four lines of amber text at a 6-pixel pitch in the terminal's panel, none
 *   aligned on the tile grid (y = 5, 21, 38, 51 in the layer)
 * - Press A: every line moves one pixel to the right (six steps, then back),
 *   the glyphs crossing tile boundaries as they go
 *
 * @par Modules Used
 * console, dma, background, input, tile, string
 *
 * @see tile.h, string.h, text.h, examples/fundamentals/text_glyphs
 */

#include <snes.h>
#include <snes/tile.h>
#include <snes/string.h>

#include "res/terminal.inc"

#define COLS        20                 /* the layer: 20 x 8 tiles = 160 x 64 pixels */
#define ROWS        8
#define TILES       (COLS * ROWS)
#define PITCH       6                  /* pixels from one glyph to the next */
#define LAYER_VRAM  0x1000             /* word address of the layer's tile 0 */
#define MAP_VRAM    0x0400
#define TERM_MAP    0x0800             /* BG2: the terminal's map and tiles */
#define TERM_TILES  0x2000
#define MAP_COL     6                  /* where the layer sits on the 32 x 32 map */
#define MAP_ROW     10

/** A 5x7 font, A-Z then 0-9: seven rows of five bits, bit 4 the left pixel. */
static const u8 font5x7[36][7] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
};

static const u8 palette[] = {
    0x41, 0x0C,     /* colour 0: the backdrop, round the bezel */
    0x00, 0x00,     /* colour 1: unused */
    0x00, 0x00,     /* colour 2: unused */
    0x3F, 0x33,     /* colour 3: amber (both planes set) */
};

/** The glyphs as 2bpp tiles, built at boot from the bits above. */
static FAR u8 glyphs[36][16];

/** The layer: 160 tiles of 16 bytes, composed here, sent to VRAM. */
static FAR u8 layer[TILES * 16];

u16 x_off;          /**< Pixels every line is shifted by (0-5): A steps it */
u16 presses;        /**< A presses taken */
u16 layer_sum;      /**< A checksum of the layer after each composition */
static u16 upload;  /**< Halves of the layer still to send (2 after a composition) */

static void build_glyphs(void)
{
    u8 g, r;

    for (g = 0; g < 36; g++) {
        for (r = 0; r < 7; r++) {
            u8 bits = (u8)(font5x7[g][r] << 2);    /* columns 1-5 of the 8 */
            glyphs[g][r * 2] = bits;               /* plane 0 */
            glyphs[g][r * 2 + 1] = bits;           /* plane 1: colour 3 */
        }
        glyphs[g][14] = 0;
        glyphs[g][15] = 0;
    }
}

static u8 glyph_of(char c)
{
    if (c >= 'A' && c <= 'Z') return (u8)(c - 'A');
    if (c >= '0' && c <= '9') return (u8)(26 + c - '0');
    return 0xFF;                                   /* space, or anything else */
}

/** Compose a string into the layer at pixel (x, y): two calls per glyph. */
static void draw_text(const char *s, u16 x, u16 y)
{
    for (; *s; s++, x += PITCH) {
        u8 g = glyph_of(*s);
        u16 col = x >> 3, row = y >> 3;
        u8 top = (u8)(y & 7), shift = (u8)(x & 7);
        u8 FAR *tile;
        u8 FAR *right;

        if (g == 0xFF || col >= COLS || row >= ROWS)
            continue;
        tile = layer + ((row * COLS + col) << 4);
        /* the tile to the right is the next slot; none at the layer's edge */
        right = (shift && col + 1 < COLS) ? tile + 16 : (u8 FAR *)0;
        tileBlitGlyph2bpp(tile, right, glyphs[g], 0, top, (u8)(8 - top), shift);
        if (top && row + 1 < ROWS)                 /* the rows that spill below */
            tileBlitGlyph2bpp(tile + COLS * 16, right ? right + COLS * 16 : (u8 FAR *)0,
                              glyphs[g], (u8)(8 - top), 0, top, shift);
    }
}

static void compose(void)
{
    u16 i, acc = 0;

    memset(layer, 0, sizeof(layer));
    draw_text("RUE BASSE NOODLES", 8 + x_off, 5);
    draw_text("RAMEN 12  SOBA 9  TEA 3", 2 + x_off, 21);
    draw_text("OPEN ALL NIGHT", 44 + x_off, 38);
    draw_text("PRESS A", 100 + x_off, 51);
    for (i = 0; i < TILES * 16; i++)
        acc = (u16)(((acc << 1) | (acc >> 15)) ^ layer[i]);
    layer_sum = acc;
    upload = 2;
}

/** Place the layer's tiles on the map, in order, and blank everywhere else. */
static void place_tiles(void)
{
    u16 col, row;

    dmaFillVRAM(TILES, MAP_VRAM, 32 * 32 * 2);     /* tile TILES: all zero */
    for (row = 0; row < ROWS; row++) {
        u16 addr = MAP_VRAM + (MAP_ROW + row) * 32 + MAP_COL;
        REG_VMAIN = 0x80;
        REG_VMADDL = (u8)(addr & 0xFF);
        REG_VMADDH = (u8)(addr >> 8);
        for (col = 0; col < COLS; col++) {
            u16 t = row * COLS + col;
            REG_VMDATAL = (u8)(t & 0xFF);
            REG_VMDATAH = (u8)(t >> 8);
        }
    }
}

int main(void)
{
    consoleInit();
    setMode(BG_MODE0, 0);
    bgSetMapPtr(0, MAP_VRAM, BG_MAP_32x32);
    bgSetGfxPtr(0, LAYER_VRAM);
    dmaCopyCGram((u8 *)palette, 0, sizeof(palette));

    /* BG2: the terminal, four colours at CGRAM 32 (BG_4COLORS0: palette 0 of BG2) */
    bgInitTileSet(1, terminal_tiles, terminal_pal, 0,
                  terminal_tiles_end - terminal_tiles,
                  terminal_pal_end - terminal_pal,
                  BG_4COLORS0, TERM_TILES);
    bgSetMapPtr(1, TERM_MAP, BG_MAP_32x32);
    dmaCopyVram(terminal_map, TERM_MAP, terminal_map_end - terminal_map);

    build_glyphs();
    x_off = 0;
    compose();
    dmaCopyVram(layer, LAYER_VRAM, TILES * 16);   /* force blank: all at once */
    dmaFillVRAM(0, LAYER_VRAM + TILES * 8, 16);    /* the blank tile after the layer */
    upload = 0;
    place_tiles();

    setMainScreen(LAYER_BG1 | LAYER_BG2);
    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
        if (upload) {                              /* one half a VBlank: 1280 bytes */
            u16 half = (u16)(2 - upload);
            dmaCopyVram(layer + half * (TILES * 8), LAYER_VRAM + half * (TILES * 4), TILES * 8);
            upload--;
        }
        if (padPressed(0) & KEY_A) {
            presses++;
            x_off = (u16)((x_off + 1) % PITCH);
            compose();
        }
    }

    return 0;
}
