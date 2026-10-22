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
 * The scene is the noodle bar seen from above (BG1), with Kit at the
 * counter and the raccoon who runs it behind it (sprites). The panel
 * sheet is res/boxes.png, twelve 2bpp tiles in an order the code relies
 * on: tile 0 is left transparent (that is what panelInit() stamps
 * everywhere), so the bar shows through wherever no panel has been drawn;
 * 1-9 are the 9-slice, 10 and 11 the hearts.
 *
 * The panel layer is BG3, the 2bpp layer of Mode 1 — the usual home of a
 * HUD, cheap in VRAM. In Mode 1, BG3 is drawn behind BG1 and BG2 unless the
 * BGMODE bit BG3_MODE1_PRIORITY_HIGH is set and the BG3 tiles carry their
 * priority bit; then it is in front of everything, sprites included. SELECT
 * turns the bit off and on, and the HUD goes behind the scene and comes
 * back. (That lesson was the mode1_bg3_priority example.)
 *
 * ROM mode: LoROM (project default).
 *
 * @par Controls
 * - B: lose a heart · A: gain one (small VBlank DMA of the heart row)
 * - START: toggle the dialog box — the raccoon talking (panelFlush; the
 *   HUD stays put)
 * - SELECT: BG3_MODE1_PRIORITY_HIGH off / on (the panels behind / in front)
 *
 * @par SNES Concepts
 * - 9-slice stamping: panelDraw() picks corner/edge/fill from the sheet by
 *   position; panelPut() drops a single sheet tile (an icon) into the box
 * - Several panels, one layer, one upload — the module's reason to exist
 * - panelFlush() uploads under real forced blank (INIDISP bit 7): safe for
 *   2 KB but it blanks briefly, so use it for structural changes; push a
 *   frequently-updated HUD row with a small ordinary VBlank DMA instead
 * - BG3 in Mode 1: 2bpp tiles (8 words each), 4-colour palettes taken from
 *   CGRAM 0-31; in front of BG1/BG2 only with BG3_MODE1_PRIORITY_HIGH and
 *   the tile priority bit (Panel.priority = 1)
 *
 * @par What to Observe
 * A HUD box with hearts sits over the noodle bar. A/B change the hearts;
 * START opens/closes a dialog box lower down without touching the HUD.
 * SELECT sends both boxes behind the bar (and behind Kit and the raccoon),
 * SELECT again brings them back in front of everything. `hero_hp`, `dialog_shown` and `bg3_high` are the probe oracles.
 *
 * @par Modules Used
 * console, dma, background, panel, input, sprite
 *
 * @see lib/include/snes/panel.h — panelInit/Draw/Put/Clear/Flush
 */

#include <snes.h>
#include <snes/panel.h>
#include <snes/input.h>

#include "res/noodlebar.inc"
#include "res/boxes.inc"
#include "res/residents.inc"
/* generated: noodlebar_tiles/_pal/_map, boxes_tiles/_pal, residents_tiles/_pal (each with _end) */

/* --- VRAM layout (word addresses) --- */
#define BG1_CHR 0x0000    /**< BG1 scene tiles: the bar, 823 tiles up to $3370 */
#define BG1_MAP 0x3800    /**< BG1 tilemap               */
#define BG3_CHR 0x4000    /**< BG3 panel sheet tiles (2bpp) */
#define BG3_MAP 0x4400    /**< BG3 panel tilemap (uploaded by panelFlush) */
#define OBJ_CHR 0x6000    /**< Kit and the raccoon       */

/* --- The panel sheet, laid out in BG3's tile space ---
 * Tile 0 MUST be transparent: panelInit() fills the map with entry 0, and
 * that is what shows the background through the layer. The 9-slice sheet
 * therefore starts at tile 1 (stride 3 → tiles 1..9), icons follow. */
#define TILE_BLANK        0
#define SHEET_BASE        1     /* top-left corner of the 3x3 border      */
#define SHEET_STRIDE      3
#define TILE_HEART        10    /* icons live past the 9-slice            */
#define TILE_HEART_EMPTY  11

/* --- The sheet's palette: palette 4 of BG3 ---
 * A 2bpp palette has 4 colours, colour 0 transparent (the sheet's 1 is the
 * box's fill, 2 its line, 3 the heart); BG3's palette 4 is CGRAM 16-19,
 * which the BG1 scene (palette 0, CGRAM 0-15) leaves free. */
#define BOX_PAL     4

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

/** @name Where Kit and the raccoon stand (top-left of their 32x32 sprites)
 *  @{ */
#define KIT_X 112
#define KIT_Y 132
#define NPC_X 112
#define NPC_Y 60
/** @} */

/** @brief Probe oracle: current hearts shown (0..MAX_HP) */
u8 hero_hp;
/** @brief Probe oracle: 1 while the dialog box is open */
u8 dialog_shown;
/** @brief Probe oracle: 1 while BG3_MODE1_PRIORITY_HIGH is set */
u8 bg3_high;

/** @brief The panel tilemap — caller-owned (the module never allocates 2 KB). */
static u16 panel_map[32 * 32];

/** @brief The panel: HUD + dialog share this one layer and one upload. */
static const Panel ui = {
    panel_map,        /* map        */
    BG3_MAP,          /* vram_addr  */
    SHEET_BASE,       /* base_tile — 9-slice starts at tile 1 (tile 0 blank) */
    SHEET_STRIDE,     /* stride     */
    BOX_PAL,          /* palette    */
    1,                /* priority — with BG3_MODE1_PRIORITY_HIGH: in front */
};

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
                (u16)(BG3_MAP + row * 32), 32 * 2);
}

int main(void) {
    u16 keys;

    consoleInit();

    /* BG1: the noodle bar, palette 0 (CGRAM 0-15) */
    bgInitTileSet(0, noodlebar_tiles, noodlebar_pal, 0,
                  noodlebar_tiles_end - noodlebar_tiles,
                  noodlebar_pal_end - noodlebar_pal,
                  BG_16COLORS, BG1_CHR);
    bgSetMapPtr(0, BG1_MAP, SC_32x32);
    dmaCopyVram(noodlebar_map, BG1_MAP, noodlebar_map_end - noodlebar_map);

    /* BG3: the panel sheet, and its four colours as palette 4 (CGRAM 16-19) */
    dmaCopyVram(boxes_tiles, BG3_CHR, boxes_tiles_end - boxes_tiles);
    dmaCopyCGram(boxes_pal, BOX_PAL * 4, boxes_pal_end - boxes_pal);
    bgSetGfxPtr(2, BG3_CHR);
    bgSetMapPtr(2, BG3_MAP, SC_32x32);

    /* The sprites: Kit at the counter, the raccoon behind it (32x32, the
     * LARGE size of OBJ_SIZE16_L32), OBJ palettes 0 and 1 */
    oamInitGfxSet(residents_tiles, residents_tiles_end - residents_tiles,
                  residents_pal, residents_pal_end - residents_pal,
                  0, OBJ_CHR, OBJ_SIZE16_L32);
    oamSet(0, KIT_X, KIT_Y, 0, 0, 2, 0);
    oamSet(1, NPC_X, NPC_Y, 4, 1, 2, 0);
    oamSetSize(0, OBJ_LARGE);
    oamSetSize(1, OBJ_LARGE);

    /* BG3 in front: the BGMODE bit, and the panel tiles' priority bit */
    bg3_high = 1;
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    setMainScreen(LAYER_BG1 | LAYER_BG3 | LAYER_OBJ);

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
        /* SELECT: the BGMODE bit alone decides whether BG3 is in front */
        if (keys & KEY_SELECT) {
            bg3_high ^= 1;
            setMode(BG_MODE1, bg3_high ? BG3_MODE1_PRIORITY_HIGH : 0);
        }
    }

    return 0;
}
