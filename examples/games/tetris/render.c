#include "render.h"
#include "piece.h"
/* Bare-metal HDMA for gradient (avoids hdma module's large iris RAM) */

/* External assets from data.asm */
extern u8 tiles_gfx[], tiles_gfx_end[];
extern u8 tiles_pal[], tiles_pal_end[];
extern u8 font2bpp_gfx[], font2bpp_gfx_end[];
extern u8 border_pal[], border_pal_end[];
extern u8 red_pal[], red_pal_end[];
extern u8 green_pal[], green_pal_end[];
extern u8 purple_pal[], purple_pal_end[];
extern u8 orange_pal[], orange_pal_end[];

/* HDMA gradient table from data.asm */
extern u8 hdma_grad_tbl[];

/* RAM tilemap buffers from data.asm */
extern u16 tilemap_bg1[];  /* BG1: playfield (full 32x32) */
extern u16 tilemap_bg2[];  /* BG2: HUD (full 32x32) */
extern u16 bg3_msgrow[];   /* BG3: single message row (32 entries) */

/* Dirty flags — BG2/BG3 use simple flags, BG1 uses per-row tracking */
u8 bg2_dirty, bg3_dirty;

/* Per-row dirty tracking for BG1 (one byte per tilemap row, 0-27 visible) */
static u8 bg1_dirty_row[32];

/* Tile indices */
#define TILE_EMPTY     0

/* Breakout-style border: 3 base tiles + SNES flip flags + palette 1 */
#define TILE_CORNER    8   /* corner (base = TL) */
#define TILE_BORDER_H  9   /* horizontal bar */
#define TILE_BORDER_V  10  /* vertical bar */

/* Tilemap attribute flags */
#define PAL1     0x0400   /* palette 1 (grey gradient) */
#define HFLIP    0x4000
#define VFLIP    0x8000

/* Composed border entries */
#define BORDER_TL  (TILE_CORNER   | PAL1)
#define BORDER_TR  (TILE_CORNER   | PAL1 | HFLIP)
#define BORDER_BL  (TILE_CORNER   | PAL1 | VFLIP)
#define BORDER_BR  (TILE_CORNER   | PAL1 | HFLIP | VFLIP)
#define BORDER_H_T (TILE_BORDER_H | PAL1)
#define BORDER_H_B (TILE_BORDER_H | PAL1 | VFLIP)
#define BORDER_VL  (TILE_BORDER_V | PAL1)
#define BORDER_VR  (TILE_BORDER_V | PAL1 | HFLIP)

/*
 * Screen layout (32×28 visible tiles) — 8×8 square cells, thick border:
 *
 *   Outer border:     cols 0+13, rows 1+24
 *   Inner border:     cols 1+12, rows 2+23
 *   Play area:        cols 2-11, rows 3-22  (20 visible game rows)
 *   HUD labels:       col 16+ (on BG2)
 *   NEXT box:         cols 16-19, rows 17-20 (on BG2)
 *   BG3 message:      row 13 (centered in play area)
 */

/*
 * Playfield mapping: board row r, column c -> BG1 tilemap index
 * Each game cell = 1 tilemap entry (8×8 tile, square blocks).
 * Hidden rows: board rows 0-3 (spawn area, not displayed)
 * Visible rows: board rows 4-23 -> tilemap rows 3-22
 * Columns: board col c -> tilemap col (c + 2)
 */
#define BOARD_TO_MAP(r, c)  (((r) - 1) * 32 + (c) + 2)

/* Next piece preview: below NEXT label on BG2 (8x8 tiles, no border) */
#define NEXT_BOX_ROW   17
#define NEXT_BOX_COL   16

/*
 * BG3 message row: row 13 in the BG3 tilemap (centered in play area rows 3-22).
 */
#define MSG_ROW  13

/*
 * VRAM layout:
 *   $0000: BG1 tilemap (playfield)
 *   $0800: BG2 tilemap (HUD)
 *   $1000: BG3 tilemap (messages)
 *   $2000: BG1/BG2 shared 4bpp tiles
 *   $3000: BG3 2bpp font tiles
 */
#define VRAM_BG1_MAP  0x0000
#define VRAM_BG2_MAP  0x0800
#define VRAM_BG3_MAP  0x1000
#define VRAM_TILES4   0x2000
#define VRAM_TILES2   0x3000

/* BG3 message row VRAM address */
#define VRAM_BG3_MSGROW  (VRAM_BG3_MAP + MSG_ROW * 32)

static void clearTilemapBG1(void) {
    u16 i;
    for (i = 0; i < 0x400; i++) {
        tilemap_bg1[i] = TILE_EMPTY;
    }
}

static void clearTilemapBG2(void) {
    u16 i;
    for (i = 0; i < 0x400; i++) {
        tilemap_bg2[i] = TILE_EMPTY;
    }
}

static void clearMsgRow(void) {
    u8 i;
    for (i = 0; i < 32; i++) {
        bg3_msgrow[i] = 0;
    }
}

static void drawPlayfieldBorder(void) {
    u8 r, c;

    /* Outer top border: row 1, cols 0-13 */
    tilemap_bg1[1 * 32 + 0] = BORDER_TL;
    for (c = 1; c <= 12; c++) {
        tilemap_bg1[1 * 32 + c] = BORDER_H_T;
    }
    tilemap_bg1[1 * 32 + 13] = BORDER_TR;

    /* Inner top border: row 2 */
    tilemap_bg1[2 * 32 + 0]  = BORDER_VL;
    tilemap_bg1[2 * 32 + 1]  = BORDER_TL;
    for (c = 2; c <= 11; c++) {
        tilemap_bg1[2 * 32 + c] = BORDER_H_T;
    }
    tilemap_bg1[2 * 32 + 12] = BORDER_TR;
    tilemap_bg1[2 * 32 + 13] = BORDER_VR;

    /* Double side borders: rows 3-22 (game area) */
    for (r = 3; r <= 22; r++) {
        tilemap_bg1[r * 32 + 0]  = BORDER_VL;
        tilemap_bg1[r * 32 + 1]  = BORDER_VL;
        tilemap_bg1[r * 32 + 12] = BORDER_VR;
        tilemap_bg1[r * 32 + 13] = BORDER_VR;
    }

    /* Inner bottom border: row 23 */
    tilemap_bg1[23 * 32 + 0]  = BORDER_VL;
    tilemap_bg1[23 * 32 + 1]  = BORDER_BL;
    for (c = 2; c <= 11; c++) {
        tilemap_bg1[23 * 32 + c] = BORDER_H_B;
    }
    tilemap_bg1[23 * 32 + 12] = BORDER_BR;
    tilemap_bg1[23 * 32 + 13] = BORDER_VR;

    /* Outer bottom border: row 24, cols 0-13 */
    tilemap_bg1[24 * 32 + 0] = BORDER_BL;
    for (c = 1; c <= 12; c++) {
        tilemap_bg1[24 * 32 + c] = BORDER_H_B;
    }
    tilemap_bg1[24 * 32 + 13] = BORDER_BR;
}

void renderInit(void) {
    u16 gfx_size, pal_size, font_size, fpal_size;

    setScreenOff();

    /* Upload 4bpp tile graphics (BG1/BG2 shared) */
    gfx_size = (u16)(u32)(tiles_gfx_end - tiles_gfx);
    dmaCopyVram(tiles_gfx, VRAM_TILES4, gfx_size);

    /* Upload 2bpp font tiles (BG3) */
    font_size = (u16)(u32)(font2bpp_gfx_end - font2bpp_gfx);
    dmaCopyVram(font2bpp_gfx, VRAM_TILES2, font_size);

    /* Upload main palette (CGRAM 0-15) */
    pal_size = (u16)(u32)(tiles_pal_end - tiles_pal);
    dmaCopyCGram(tiles_pal, 0, pal_size);

    /* Upload border+font palette (CGRAM 16-31 = palette 1) */
    fpal_size = (u16)(u32)(border_pal_end - border_pal);
    dmaCopyCGram(border_pal, 16, fpal_size);

    /* Upload red font palette (CGRAM 32-47 = palette 2) */
    dmaCopyCGram(red_pal, 32, (u16)(u32)(red_pal_end - red_pal));

    /* Upload green font palette (CGRAM 48-63 = palette 3) */
    dmaCopyCGram(green_pal, 48, (u16)(u32)(green_pal_end - green_pal));

    /* Upload purple font palette (CGRAM 64-79 = palette 4) */
    dmaCopyCGram(purple_pal, 64, (u16)(u32)(purple_pal_end - purple_pal));

    /* Upload orange font palette (CGRAM 80-95 = palette 5) */
    dmaCopyCGram(orange_pal, 80, (u16)(u32)(orange_pal_end - orange_pal));

    /* Clear tilemap buffers */
    clearTilemapBG1();
    clearTilemapBG2();
    clearMsgRow();

    /* Clear BG3 VRAM tilemap (VRAM not zeroed by crt0) */
    dmaCopyVram((u8 *)tilemap_bg1, VRAM_BG3_MAP, 0x800);

    drawPlayfieldBorder();

    /* Configure BG1: tilemap at $0000, 4bpp tiles at $2000 */
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    bgSetGfxPtr(0, VRAM_TILES4);

    /* Configure BG2: tilemap at $0800, 4bpp tiles at $2000 (shared) */
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    bgSetGfxPtr(1, VRAM_TILES4);

    /* Configure BG3: tilemap at $1000, 2bpp tiles at $3000 */
    bgSetMapPtr(2, VRAM_BG3_MAP, SC_32x32);
    bgSetGfxPtr(2, VRAM_TILES2);

    /* Mode 1 with BG3 high priority */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    REG_TM = TM_BG1 | TM_BG2 | TM_BG3;

    bgSetScroll(0, 0, 0);  /* BG1: playfield centered (3-tile padding top+bottom) */
    bgSetScroll(1, 0, 0);  /* BG2: HUD */
    bgSetScroll(2, 0, 0);  /* BG3: messages */

    /* Build HDMA gradient on CGRAM color 0: lighter blue (top) → deep blue (bottom) */
    {
        u8 i;
        u8 *p = hdma_grad_tbl;
        /* Top: R=7, G=12, B=14  Bottom: R=3, G=5, B=7 (5-bit components) */
        s16 r_fp = 7 * 256;   /* 8.8 fixed point */
        s16 g_fp = 12 * 256;
        s16 b_fp = 14 * 256;

        /* 56 chunks of 4 scanlines = 224 scanlines */
        for (i = 0; i < 56; i++) {
            u16 color = (u16)(((u16)(b_fp >> 8) << 10) |
                              ((u16)(g_fp >> 8) << 5) |
                              (u16)(r_fp >> 8));

            *p++ = 4;                   /* 4 scanlines, non-repeat */
            *p++ = 0;                   /* CGADD byte 1: color index 0 */
            *p++ = 0;                   /* CGADD byte 2 (mode 2REG_2X) */
            *p++ = (u8)(color);         /* CGDATA low */
            *p++ = (u8)(color >> 8);    /* CGDATA high */

            r_fp -= 19;  /* (7-3)*256/55 ≈ 19 */
            g_fp -= 33;  /* (12-5)*256/55 ≈ 33 */
            b_fp -= 33;  /* (14-7)*256/55 ≈ 33 */
        }
        *p = 0;  /* HDMA terminator */

        /* Set up HDMA channel 7 registers (enable deferred to after setScreenOn) */
        *(u8 *)0x4370 = 0x03;                          /* Mode 3: AABB → reg,reg,reg+1,reg+1 */
        *(u8 *)0x4371 = 0x21;                          /* B-bus: $2121 (CGADD) */
        *(u16 *)0x4372 = (u16)(u32)hdma_grad_tbl;      /* Table addr low+high */
        *(u8 *)0x4374 = 0x00;                          /* Bank 0 (RAM) */
    }

    /* Mark all visible rows dirty for initial force-blank flush */
    {
        u8 r;
        for (r = 0; r < 28; r++) bg1_dirty_row[r] = 1;
    }
    bg2_dirty = 1;
    bg3_dirty = 0;
    WaitForVBlank();
    renderFlush();
}

void renderBoard(void) {
    u8 r, c;
    u16 idx;
    u8 cell;

    for (r = VISIBLE_TOP; r < BOARD_ROWS; r++) {
        for (c = 0; c < BOARD_W; c++) {
            idx = BOARD_TO_MAP(r, c);
            cell = boardGetCell(r, c);
            tilemap_bg1[idx] = cell ? (u16)cell : TILE_EMPTY;
        }
        bg1_dirty_row[r - 1] = 1;
    }
}

void renderPiece(u8 type, u8 rot, s8 row, s8 col) {
    u8 i;
    s8 r, c;

    for (i = 0; i < CELLS_PER_PIECE; i++) {
        r = row + pieceGetCellRow(type, rot, i);
        c = col + pieceGetCellCol(type, rot, i);

        if (r >= VISIBLE_TOP && r < BOARD_ROWS && c >= 0 && c < BOARD_W) {
            tilemap_bg1[BOARD_TO_MAP(r, c)] = (u16)type;
            bg1_dirty_row[r - 1] = 1;
        }
    }
}

void renderErasePiece(u8 type, u8 rot, s8 row, s8 col) {
    u8 i;
    s8 r, c;
    u8 cell;

    for (i = 0; i < CELLS_PER_PIECE; i++) {
        r = row + pieceGetCellRow(type, rot, i);
        c = col + pieceGetCellCol(type, rot, i);

        if (r >= VISIBLE_TOP && r < BOARD_ROWS && c >= 0 && c < BOARD_W) {
            /* Restore board state (locked blocks or empty) */
            cell = boardGetCell(r, c);
            tilemap_bg1[BOARD_TO_MAP(r, c)] = cell ? (u16)cell : TILE_EMPTY;
            bg1_dirty_row[r - 1] = 1;
        }
    }
}

void renderNextPiece(u8 type) {
    u8 i;
    u16 map_r, map_c;

    renderClearNextPiece();

    /* NEXT preview uses original 8x8 tiles (1-7) on BG2 */
    for (i = 0; i < CELLS_PER_PIECE; i++) {
        map_r = NEXT_BOX_ROW + (u16)pieceGetCellRow(type, 0, i);
        map_c = NEXT_BOX_COL + (u16)pieceGetCellCol(type, 0, i);
        tilemap_bg2[map_r * 32 + map_c] = type;
    }
    bg2_dirty = 1;
}

void renderClearNextPiece(void) {
    u8 r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            tilemap_bg2[(NEXT_BOX_ROW + r) * 32 + (NEXT_BOX_COL + c)] = TILE_EMPTY;
        }
    }
    bg2_dirty = 1;
}

void renderLineClearFlash(LineClearResult *result, u8 frame) {
    u8 i, c, r;
    u8 tile;

    tile = (frame & 1) ? (TILE_BORDER_H | PAL1) : TILE_EMPTY;

    for (i = 0; i < result->count; i++) {
        r = result->rows[i];
        for (c = 0; c < BOARD_W; c++) {
            tilemap_bg1[BOARD_TO_MAP(r, c)] = tile;
        }
        bg1_dirty_row[r - 1] = 1;
    }
}

/* BG3 message text color (CGRAM 25 = 2bpp palette 6, color 1) */
static u16 msg_color;
static u8 msg_color_dirty;

void renderEnableGradient(void) {
    *(u8 *)0x420C = 0x80;  /* Enable HDMA channel 7 */
}

void renderSetMsgColor(u16 color) {
    msg_color = color;
    msg_color_dirty = 1;
}

void renderShake(s8 dx, s8 dy) {
    bgSetScroll(0, (u16)dx, (u16)dy);
}

void renderFlush(void) {
    u8 r, start, count;

    /* BG3 + CGRAM first — tiny, critical for messages */
    if (bg3_dirty) {
        dmaCopyVram((u8 *)bg3_msgrow, VRAM_BG3_MSGROW, 64);
        bg3_dirty = 0;
    }
    if (msg_color_dirty) {
        dmaCopyCGram((u8 *)&msg_color, 25, 2);
        msg_color_dirty = 0;
    }

    /* BG2 before BG1: HUD updates are small (2KB) and must complete in VBlank.
     * BG1 row-granular DMA can be large (~1.3KB on completion frames) and
     * combined with BG2 may push past VBlank if BG2 is last. */
    if (bg2_dirty) {
        dmaCopyVram((u8 *)tilemap_bg2, VRAM_BG2_MAP, 0x800);
        bg2_dirty = 0;
    }

    /* BG1: row-granular DMA — coalesce contiguous dirty rows */
    r = 0;
    while (r < 28) {
        if (bg1_dirty_row[r]) {
            start = r;
            count = 0;
            while (r < 28 && bg1_dirty_row[r]) {
                bg1_dirty_row[r] = 0;
                count++;
                r++;
            }
            dmaCopyVram((u8 *)&tilemap_bg1[(u16)start * 32],
                        VRAM_BG1_MAP + (u16)start * 32,
                        (u16)count * 64);
        } else {
            r++;
        }
    }
}
