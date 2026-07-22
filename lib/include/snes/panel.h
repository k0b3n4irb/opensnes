/**
 * @file panel.h
 * @brief Opt-in 9-slice panels — bordered boxes on a background layer.
 * @ingroup ui
 *
 * Every SNES RPG draws the same thing: a bordered box over the map, with
 * text in it. So does every status bar. Both are a **9-slice panel** —
 * corners, edges and a fill stamped from a 3x3 tile sheet — and both were
 * ~60 lines of stamping loop and priority wiring in each game that wanted
 * one.
 *
 * @par Why it is not called `window`
 * `<snes/window.h>` is the PPU's window/masking registers. This is
 * unrelated furniture.
 *
 * @par The tilemap is yours
 * `panelInit()` takes the 32x32 entry buffer rather than owning one. A
 * panel tilemap is 2 KB and C RAM is an 8 KB band (KNOWN_LIMITATIONS.md,
 * defect B2) — a module that allocated it silently would be spending a
 * quarter of a game's RAM without saying so. Declaring it yourself keeps
 * the cost where you can see it, and lets two layers each have one.
 *
 * @par Several panels, one layer, one upload
 * Panels are stamped into that buffer, not uploaded individually. A HUD
 * at the top and a dialog box at the bottom are two `panelDraw()` calls
 * into the same map and a single `panelFlush()`. Opening the dialog is
 * `panelDraw` + `panelFlush`; closing it is `panelClear` + `panelFlush`,
 * and the HUD is untouched either way.
 *
 * @par The upload is forced blank
 * `panelFlush()` pushes 2 KB, which does not fit the ~4 KB VBlank budget
 * alongside anything else, so it wraps the DMA in `setScreenOff()` /
 * `setScreenOn()` — real forced blank, INIDISP bit 7. Doing that by hand
 * with `setBrightness(0)` is the trap documented on `setBrightness()`:
 * the screen goes black but the PPU keeps fetching and the write is
 * dropped. The module removes the chance to get it wrong.
 *
 * @par Example
 * @code
 * static u16 ui_map[32 * 32];
 * static Panel ui = {
 *     .map = ui_map, .vram_addr = 0x4400, .base_tile = 0,
 *     .stride = 4, .palette = 2, .priority = 1,
 * };
 *
 * panelInit(&ui);
 * panelDraw(&ui, 0, 0, 16, 3);            // a HUD, permanently
 * panelPut(&ui, 1, 1, ICON_HEART);        // an icon inside it
 * panelFlush(&ui);
 * ...
 * panelDraw(&ui, 2, 22, 28, 6);           // the dialog box, on demand
 * panelFlush(&ui);
 * ...
 * panelClear(&ui, 2, 22, 28, 6);          // and away again
 * panelFlush(&ui);
 * @endcode
 *
 * @par Module
 * `panel` (depends on `dma`, `console`)
 */
#ifndef SNES_PANEL_H
#define SNES_PANEL_H

#include <snes/types.h>

/**
 * @brief A 9-slice panel layer: one BG tilemap and the sheet to stamp from.
 *
 * The sheet is read in raster order from @ref base_tile:
 *
 *     base+0        base+1        base+2         <- top-left, top, top-right
 *     base+stride   base+stride+1 base+stride+2  <- left, centre, right
 *     base+2*stride …                            <- bottom-left, bottom, bottom-right
 *
 * @ref stride is the sheet's width in tiles, which is 3 for a bare
 * 9-slice and more when the sheet carries other art in later columns —
 * `examples/games/rpg` uses a 4-wide sheet whose fourth column holds HUD
 * icons.
 */
typedef struct {
    u16 *map;        /**< 32x32 tilemap entries in RAM. Caller-owned. */
    u16 vram_addr;   /**< VRAM **word** address the map is uploaded to. */
    u16 base_tile;   /**< Tile number of the sheet's top-left corner. */
    u8  stride;      /**< Sheet width in tiles (3 for a bare 9-slice). */
    u8  palette;     /**< BG palette slot 0-7. */
    u8  priority;    /**< Per-tile priority bit (1 = in front of BG-lo). */
} Panel;

/**
 * @brief Blank the whole tilemap. Call once before the first draw.
 *
 * Entry 0 is written everywhere, which shows the backdrop through the
 * layer. It does not upload — pair it with panelFlush().
 *
 * @param p The panel layer.
 */
void panelInit(const Panel *p);

/**
 * @brief Stamp a bordered box of @p w x @p h tiles at (@p x, @p y).
 *
 * Coordinates and size are in tiles, in the 32x32 map. A box smaller
 * than 2x2 has no interior and is ignored; anything crossing the map's
 * edge is clipped.
 *
 * Does not upload — call panelFlush() when the map is how you want it.
 *
 * @param p The panel layer.
 * @param x Left column (0-31).
 * @param y Top row (0-31).
 * @param w Width in tiles (>= 2).
 * @param h Height in tiles (>= 2).
 */
void panelDraw(const Panel *p, u8 x, u8 y, u8 w, u8 h);

/**
 * @brief Blank a rectangle of the tilemap, leaving the rest alone.
 *
 * The counterpart of panelDraw() for closing one box while others stay.
 *
 * @param p The panel layer.
 * @param x Left column (0-31).
 * @param y Top row (0-31).
 * @param w Width in tiles.
 * @param h Height in tiles.
 */
void panelClear(const Panel *p, u8 x, u8 y, u8 w, u8 h);

/**
 * @brief Put one tile of the sheet into the map, with the layer's
 *        palette and priority.
 *
 * For art that lives in the same sheet as the border — HUD icons, a
 * cursor, a portrait frame. @p tile is a tile number, not a sheet
 * coordinate.
 *
 * @param p    The panel layer.
 * @param x    Column (0-31).
 * @param y    Row (0-31).
 * @param tile Tile number (absolute, not relative to base_tile).
 */
void panelPut(const Panel *p, u8 x, u8 y, u16 tile);

/**
 * @brief Upload the tilemap to VRAM under forced blank.
 *
 * One 2 KB DMA covering every panel in the layer. Wraps itself in
 * setScreenOff() / setScreenOn() because 2 KB does not reliably fit
 * VBlank — see the file header.
 *
 * @warning Restores full brightness on exit. If you are mid-fade, upload
 *          before starting the fade rather than during it.
 *
 * @param p The panel layer.
 */
void panelFlush(const Panel *p);

#endif /* SNES_PANEL_H */
