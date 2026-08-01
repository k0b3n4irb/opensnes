/**
 * @file main.c
 * @brief Game math & UI, rung — panel HUD: a status box that survives a dialog
 * @ingroup examples
 *
 * Every game needs furniture: a status bar with hearts and a coin count, a
 * dialog box that opens over the world and closes again. Both are the same
 * primitive — a **9-slice panel**, a bordered box stamped from a 3x3 tile
 * sheet (four corners, four edges, one fill) onto a background layer. The
 * `panel` module does the stamping; you own the tilemap.
 *
 * Its whole point is on show here: the HUD and the dialog live in ONE
 * tilemap on ONE layer. Opening the dialog (panelDraw + panelFlush) and
 * closing it (panelClear + panelFlush) never disturbs the HUD.
 *
 * panelFlush() uploads the whole 2 KB map under FORCED BLANK, which briefly
 * blanks the screen — fine for a structural change like a dialog box, but
 * far too heavy to run every time a heart changes. So the hearts, which
 * change often, upload only their own 64-byte tilemap row with an ordinary
 * VBlank DMA (flush_hud_row) — no forced blank, no flicker. The rule:
 * panelFlush for structure, a small VBlank DMA for frequent HUD tweaks.
 *
 * The border and heart icons are generated in C — zero assets. Tile 0 is
 * left transparent (that is what panelInit() stamps everywhere), so the
 * colourful BG1 shows through wherever no panel has been drawn.
 *
 * ROM mode: LoROM (project default).
 *
 * @par Controls
 * - B: lose a heart · A: gain one (small VBlank DMA of the heart row)
 * - START: toggle the dialog box (panelFlush; the HUD stays put)
 *
 * @par SNES Concepts
 * - 9-slice stamping: panelDraw() picks corner/edge/fill from the sheet by
 *   position; panelPut() drops a single sheet tile (an icon) into the box
 * - Several panels, one layer, one upload — the module's reason to exist
 * - panelFlush() uploads under real forced blank (INIDISP bit 7): safe for
 *   2 KB but it blanks briefly, so use it for structural changes; push a
 *   frequently-updated HUD row with a small ordinary VBlank DMA instead
 *
 * @par What to Observe
 * A HUD box with hearts sits over a colourful background. A/B change the
 * hearts; START opens/closes a dialog box lower down without touching the
 * HUD. `hero_hp` and `dialog_shown` are the probe oracles.
 *
 * @par Modules Used
 * console, dma, background, panel, input
 *
 * @see lib/include/snes/panel.h — panelInit/Draw/Put/Clear/Flush
 */

#include <snes.h>
#include <snes/panel.h>
#include <snes/input.h>

/* --- VRAM layout (word addresses) --- */
#define BG1_CHR 0x0000    /**< BG1 scene tiles           */
#define BG1_MAP 0x1000    /**< BG1 tilemap               */
#define BG2_CHR 0x2000    /**< BG2 panel sheet tiles      */
#define BG2_MAP 0x4400    /**< BG2 panel tilemap (uploaded by panelFlush) */

/* --- The panel sheet, laid out in BG2's tile space ---
 * Tile 0 MUST be transparent: panelInit() fills the map with entry 0, and
 * that is what shows the background through the layer. The 9-slice sheet
 * therefore starts at tile 1 (stride 3 → tiles 1..9), icons follow. */
#define TILE_BLANK        0
#define SHEET_BASE        1     /* top-left corner of the 3x3 border      */
#define SHEET_STRIDE      3
#define TILE_HEART        10    /* icons live past the 9-slice            */
#define TILE_HEART_EMPTY  11

/* --- Palette 1 (BOX_PAL) pixel values inside the sheet tiles --- */
#define BOX_PAL     1
#define FILL_IDX    1          /* box interior            */
#define BORDER_IDX  2          /* box border              */
#define HEART_IDX   3          /* heart red               */
#define EMPTY_IDX   4          /* empty-heart grey        */

/* --- HUD and dialog geometry (tiles) --- */
#define HUD_X 1
#define HUD_Y 1
#define HUD_W 14
#define HUD_H 3
#define DLG_X 4
#define DLG_Y 20
#define DLG_W 24
#define DLG_H 7

#define MAX_HP 5

/** @brief Probe oracle: current hearts shown (0..MAX_HP) */
u8 hero_hp;
/** @brief Probe oracle: 1 while the dialog box is open */
u8 dialog_shown;

/** @brief The panel tilemap — caller-owned (the module never allocates 2 KB). */
static u16 panel_map[32 * 32];

/** @brief The panel: HUD + dialog share this one layer and one upload. */
static const Panel ui = {
    panel_map,        /* map        */
    BG2_MAP,          /* vram_addr  */
    SHEET_BASE,       /* base_tile — 9-slice starts at tile 1 (tile 0 blank) */
    SHEET_STRIDE,     /* stride     */
    BOX_PAL,          /* palette    */
    1,                /* priority — in front of the BG1 scene */
};

/** @brief Scratch: one 8x8 tile as pixel indices, then packed to 4bpp. */
static u8 px[64];
static u8 tilebuf[32];

/** @brief Pack px[] (8x8 palette indices) into a 4bpp planar tile. */
static void encode_4bpp(void) {
    u8 pair, row, col;
    u16 o = 0;
    for (pair = 0; pair < 4; pair += 2) {
        for (row = 0; row < 8; row++) {
            u8 lo = 0, hi = 0;
            for (col = 0; col < 8; col++) {
                u8 v = px[row * 8 + col];
                if (v & (1 << pair))       lo |= (u8)(0x80 >> col);
                if (v & (1 << (pair + 1))) hi |= (u8)(0x80 >> col);
            }
            tilebuf[o++] = lo;
            tilebuf[o++] = hi;
        }
    }
}

/** @brief Upload the just-encoded tile to a tile slot in BG2's char space. */
static void put_tile(u16 tile) {
    encode_4bpp();
    dmaCopyVram(tilebuf, (u16)(BG2_CHR + tile * 16), 32);
}

/**
 * @brief Build one 9-slice border tile: border on the named edges, fill else.
 *
 * @param top,bottom,left,right 1 if this tile carries that edge.
 */
static void build_border(u8 top, u8 bottom, u8 left, u8 right) {
    u8 r, c;
    for (r = 0; r < 8; r++) {
        for (c = 0; c < 8; c++) {
            u8 edge = (u8)((top && r == 0) || (bottom && r == 7) ||
                           (left && c == 0) || (right && c == 7));
            px[r * 8 + c] = edge ? BORDER_IDX : FILL_IDX;
        }
    }
}

/** @brief Build a heart icon on the box-fill background (filled or outline). */
static void build_heart(u8 filled) {
    /* 8x8 heart bitmap, one byte per row (bit 7 = leftmost pixel). */
    static const u8 shape[8]   = {0x00,0x66,0xFF,0xFF,0x7E,0x3C,0x18,0x00};
    static const u8 outline[8] = {0x00,0x66,0x99,0x81,0x42,0x24,0x18,0x00};
    u8 r, c;
    for (r = 0; r < 8; r++) {
        u8 bits = filled ? shape[r] : outline[r];
        for (c = 0; c < 8; c++) {
            u8 on = (u8)((bits >> (7 - c)) & 1);
            px[r * 8 + c] = on ? (filled ? HEART_IDX : EMPTY_IDX) : FILL_IDX;
        }
    }
}

/** @brief Build the whole BG2 sheet: blank, 9-slice, two heart icons. */
static void build_sheet(void) {
    u8 i;
    /* tile 0: fully transparent */
    for (i = 0; i < 64; i++) px[i] = 0;
    put_tile(TILE_BLANK);
    /* tiles 1..9: the 3x3 border, raster order (matches panelDraw) */
    build_border(1,0,1,0); put_tile(1);   /* top-left     */
    build_border(1,0,0,0); put_tile(2);   /* top          */
    build_border(1,0,0,1); put_tile(3);   /* top-right    */
    build_border(0,0,1,0); put_tile(4);   /* left         */
    build_border(0,0,0,0); put_tile(5);   /* centre fill  */
    build_border(0,0,0,1); put_tile(6);   /* right        */
    build_border(0,1,1,0); put_tile(7);   /* bottom-left  */
    build_border(0,1,0,0); put_tile(8);   /* bottom       */
    build_border(0,1,0,1); put_tile(9);   /* bottom-right */
    /* icons */
    build_heart(1); put_tile(TILE_HEART);
    build_heart(0); put_tile(TILE_HEART_EMPTY);
}

/** @brief A colourful BG1 scene so the HUD clearly floats over content. */
static void build_scene(void) {
    u16 pal[16];
    u8  i;
    u16 row, col;

    /* 16 vivid colours (a simple HSV wheel) into palette 0 */
    for (i = 0; i < 16; i++) {
        u16 hue = (u16)(i * 12);
        u8 seg = (u8)(hue / 32), up = (u8)(hue % 32), dn = (u8)(31 - up);
        u8 r = 0, g = 0, b = 0;
        switch (seg) {
            case 0: r = 31; g = up; break;
            case 1: r = dn; g = 31; break;
            case 2: g = 31; b = up; break;
            case 3: g = dn; b = 31; break;
            case 4: r = up; b = 31; break;
            default:r = 31; b = dn; break;
        }
        pal[i] = RGB(r, g, b);
    }
    dmaCopyCGram((u8 *)pal, 0, 32);

    /* 16 solid tiles at BG1_CHR, then a diagonal map so the scene is busy */
    for (i = 0; i < 16; i++) {
        u8 j;
        for (j = 0; j < 64; j++) px[j] = i;
        encode_4bpp();
        dmaCopyVram(tilebuf, (u16)(BG1_CHR + i * 16), 32);
    }
    for (row = 0; row < 32; row++) {
        u16 rowbuf[32];
        for (col = 0; col < 32; col++) rowbuf[col] = (u16)((row + col) & 15);
        dmaCopyVram((u8 *)rowbuf, (u16)(BG1_MAP + row * 32), 64);
    }
}

/** @brief Redraw the heart row inside the HUD from hero_hp. */
static void draw_hearts(void) {
    u8 i;
    for (i = 0; i < MAX_HP; i++) {
        panelPut(&ui, (u8)(HUD_X + 1 + i), (u8)(HUD_Y + 1),
                 (i < hero_hp) ? TILE_HEART : TILE_HEART_EMPTY);
    }
}

/**
 * @brief Push ONLY the HUD's heart row to VRAM — a small VBlank-safe DMA.
 *
 * The hearts change often (every hit), so they must not pay panelFlush()'s
 * 2 KB forced-blank cost — that would blank the top of the screen for a
 * frame on every change. One tilemap row is 64 bytes, which fits the VBlank
 * budget with room to spare, so we upload just that row with no forced
 * blank. panelFlush() stays for structural changes (opening the dialog).
 */
static void flush_hud_row(void) {
    u16 row = (u16)(HUD_Y + 1);
    dmaCopyVram((const u8 *)&panel_map[row * 32],
                (u16)(BG2_MAP + row * 32), 32 * 2);
}

int main(void) {
    u16 keys;

    consoleInit();

    build_scene();          /* BG1: palette + tiles + map */
    build_sheet();          /* BG2: the panel sheet        */

    /* BG2 palette (palette 1): fill, border, heart, empty */
    setColor(BOX_PAL * 16 + FILL_IDX,   RGB( 3,  6, 16));
    setColor(BOX_PAL * 16 + BORDER_IDX, RGB(28, 28, 31));
    setColor(BOX_PAL * 16 + HEART_IDX,  RGB(31,  4,  8));
    setColor(BOX_PAL * 16 + EMPTY_IDX,  RGB(10, 10, 12));

    bgSetGfxPtr(0, BG1_CHR);
    bgSetMapPtr(0, BG1_MAP, SC_32x32);
    bgSetGfxPtr(1, BG2_CHR);
    bgSetMapPtr(1, BG2_MAP, SC_32x32);

    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1 | LAYER_BG2);

    /* Stamp the permanent HUD, then upload once (panelFlush ends screen-on) */
    hero_hp = 3;
    dialog_shown = 0;
    panelInit(&ui);
    panelDraw(&ui, HUD_X, HUD_Y, HUD_W, HUD_H);
    draw_hearts();
    panelFlush(&ui);

    while (1) {
        WaitForVBlank();
        keys = padPressed(0);

        /* Hearts change often -> upload just their row in VBlank (no blank). */
        if ((keys & KEY_B) && hero_hp > 0) {
            hero_hp--;
            draw_hearts();
            flush_hud_row();
        }
        if ((keys & KEY_A) && hero_hp < MAX_HP) {
            hero_hp++;
            draw_hearts();
            flush_hud_row();
        }
        /* The dialog is a structural change -> panelFlush (forced-blank, 2 KB).
         * It happens on a deliberate open/close, not every frame, and never
         * disturbs the HUD row. */
        if (keys & KEY_START) {
            if (dialog_shown) {
                panelClear(&ui, DLG_X, DLG_Y, DLG_W, DLG_H);
                dialog_shown = 0;
            } else {
                panelDraw(&ui, DLG_X, DLG_Y, DLG_W, DLG_H);
                dialog_shown = 1;
            }
            panelFlush(&ui);
        }
    }

    return 0;
}
