# 9-slice panels {#tutorial_panel}

This tutorial covers the `panel` module — bordered boxes on a background
layer, the building block of dialog windows and HUDs. Add `panel` to
`LIB_MODULES` (it pulls in `dma` and `console`).

## What a panel is

Every SNES RPG draws the same thing: a bordered box over the map with text
in it. Every status bar is the same shape. Both are a **9-slice panel** —
four corners, four edges and a fill, stamped from a small 3×3 tile sheet so
a box of any size reuses nine tiles:

```
  ┌───────┐     TL  T   TR
  │       │  =  L   C   R
  └───────┘     BL  B   BR
```

The module owns the stamping and, importantly, the VRAM upload — which is
the part that is easy to get wrong (see the forced-blank gotcha below).

> Not to be confused with `window` — that's the PPU's masking registers.
> `panel` is unrelated furniture.

## The `Panel` struct — the tilemap is yours

```c
typedef struct {
    u16 *map;        // 32x32 tilemap entries in RAM. Caller-owned.
    u16 vram_addr;   // VRAM word address the map is uploaded to.
    u16 base_tile;   // tile number of the sheet's top-left (TL) corner.
    u8  stride;      // sheet width in tiles (3 for a bare 9-slice).
    u8  palette;     // BG palette slot 0-7.
    u8  priority;    // per-tile priority bit (1 = in front of BG-lo).
} Panel;
```

You declare the 32×32 tilemap buffer, not the module. That is deliberate: a
panel tilemap is 2 KB and C RAM is only an 8 KB band, so a module that
allocated one silently would spend a quarter of a game's RAM without saying
so. Declaring it yourself keeps the cost visible — and lets two BG layers
each have their own panel.

`stride` is the sheet's width in tiles. It is 3 for a bare 9-slice, but a
sheet often carries other art in later columns — the RPG's is 4 wide, with
HUD icons in the fourth column reached via `panelPut()`.

## The minimum viable dialog box

```c
#include <snes/panel.h>

static u16 ui_map[32 * 32];
static const Panel ui = {
    ui_map,        // map
    0x4400,        // vram_addr — where BG2's tilemap lives
    0,             // base_tile — the 9-slice starts at tile 0
    3,             // stride — a bare 3-wide sheet
    2,             // palette — BG palette 2
    1,             // priority — in front of the opaque background
};

panelInit(&ui);                       // blank the whole map once
panelDraw(&ui, 2, 22, 28, 6);         // a box: x, y, w, h in tiles
panelFlush(&ui);                      // upload it
textPrintAt(4, 24, "WELCOME, TRAVELER!");
```

- **`panelInit`** blanks the 32×32 map (entry 0 everywhere → backdrop shows
  through). Call once before the first draw.
- **`panelDraw(p, x, y, w, h)`** stamps a `w`×`h` bordered box at tile
  `(x, y)`. A box smaller than 2×2 has no interior and is ignored; anything
  crossing the map edge is clipped.
- **`panelClear(p, x, y, w, h)`** blanks a rectangle — the counterpart of
  draw for closing one box while others stay.
- **`panelPut(p, x, y, tile)`** places one arbitrary sheet tile (an icon)
  with the layer's palette/priority.
- **`panelFlush(p)`** uploads the whole tilemap to VRAM.

## Several panels, one layer, one upload

Panels are stamped into the buffer, not uploaded individually. So a HUD at
the top and a dialog box at the bottom are two `panelDraw` calls into the
same map and a *single* `panelFlush`:

```c
panelInit(&ui);
panelDraw(&ui, 0, 0, 16, 3);          // the HUD, permanent
panelPut(&ui, 1, 1, ICON_HEART);      // an icon in it
panelDraw(&ui, 2, 22, 28, 6);         // the dialog box, on demand
panelFlush(&ui);                      // one DMA covers both
```

Closing just the dialog leaves the HUD untouched:

```c
panelClear(&ui, 2, 22, 28, 6);
panelFlush(&ui);
```

This is exactly how `examples/games/rpg` draws its HUD (hearts + a purse)
and dialog box on one BG2 tilemap.

## Gotchas

### 🔴 `panelFlush` uploads under forced blank — do not do it yourself with `setBrightness(0)`

A 32×32 tilemap is 2 KB, which does not reliably fit the ~4 KB VBlank DMA
budget alongside a game's own transfers. So `panelFlush` wraps its DMA in
`setScreenOff()` / `setScreenOn()` — real forced blank (INIDISP bit 7).

If you were tempted to hand-roll the upload with `setBrightness(0)` around a
`dmaCopyVram`, that is the trap the module exists to remove:
`setBrightness(0)` blacks the screen but leaves the PPU fetching, so the
VRAM write is **still dropped** — the tail of the transfer never lands. Only
`setScreenOff()` opens the write window. Let `panelFlush` do it.

### 🟡 Text goes on a *different* layer, above the panel

The panel is a BG layer; the text sits on another BG in front of it
(`text_config.priority` + the mode's BG-priority bit). Layers stack
*scene < panel < text*. Drawing text into the panel's own tilemap is not
how it works — draw the box on BG2, the text on BG3.

### 🟡 Match `base_tile`/`stride` to how the sheet was converted

The nine slices are read in raster order from `base_tile` across `stride`
columns: `base+0/1/2` (top row), `base+stride/+1/+2` (middle), and so on. If
the sheet is 4 wide (icons in column 4), `stride` is 4, not 3, or the middle
row reads the wrong tiles.

## See also

- [Text rendering](sprites.md) and the `text` module — the text that goes in
  the box.
- [DMA & VBlank](dma.md) — why `setScreenOff()` and not `setBrightness(0)`.
- `examples/games/rpg` — the module in a real dialog box + HUD.
- @ref panel.h "Panel API reference"
