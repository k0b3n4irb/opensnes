# Graphics & Backgrounds Tutorial {#tutorial_graphics}

This tutorial covers SNES graphics fundamentals including video modes, backgrounds, and tilemaps.

## SNES Video Modes

The SNES has eight background modes (0-7); most games use Mode 1, text and
simple UI use Mode 0, rotation uses Mode 7. What each mode gives each layer
is tabled in @ref hardware_ppu_modes "The PPU: background modes".

## Setting Up Graphics

### Initialize Display

```c
#include <snes.h>

int main(void) {
    // Initialize console (sets up display, enables NMI)
    consoleInit();

    // Set video mode 1 (most common for games)
    setMode(BG_MODE1, 0);

    // Enable BG1 on main screen
    REG_TM = TM_BG1;

    // Turn on display
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
```

## Loading Tiles to VRAM

Tiles are 8x8 pixel graphics stored in VRAM. For 4bpp (16 color) tiles:

```c
// Tile data (32 bytes per 8x8 tile for 4bpp)
const u8 my_tiles[] = {
    // Plane 0-1 (16 bytes)
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    // Plane 2-3 (16 bytes)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void load_tiles(void) {
    u16 i;

    // Set VRAM address (tiles at $1000)
    REG_VMAIN = 0x80;  // Increment on high byte
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x10;  // $1000

    // Copy tile data
    for (i = 0; i < sizeof(my_tiles); i += 2) {
        REG_VMDATAL = my_tiles[i];
        REG_VMDATAH = my_tiles[i + 1];
    }
}
```

The `background` module does the same with DMA, and points the layer at
the tiles in one call. `bgInitTileSetData()` loads tiles only (no
palette), for a second tile set or a layer that shares a palette:

```c
extern u8 hud_tiles[], hud_tiles_end[];
extern u8 extra_tiles[], extra_tiles_end[];

/* DMA the tiles to VRAM $3000 and set BG3's tile base there */
bgInitTileSetData(2, hud_tiles, (u16)(hud_tiles_end - hud_tiles), 0x3000);

/* 0xFF as the layer: load the tiles, leave every tile base alone */
bgInitTileSetData(0xFF, extra_tiles, (u16)(extra_tiles_end - extra_tiles), 0x5000);
```

Like any DMA to VRAM, call it during force blank or VBlank.
`bgInit(bg)` puts a layer's scroll back to (0, 0).

## Setting Up Tilemaps

Tilemaps define which tiles appear where on screen. Each entry is 2 bytes —
tile number, palette, priority and flips; the bit layout is in
@ref hardware_ppu_tilemaps "The PPU: tilemaps".

```c
void setup_tilemap(void) {
    u16 x, y;

    // Set VRAM address for tilemap ($0400)
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;

    // Fill 32x32 tilemap
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            REG_VMDATAL = 0x01;  // Tile number 1
            REG_VMDATAH = 0x00;  // No flip, palette 0
        }
    }
}
```

## Setting Palettes

Colours are 15-bit BGR555, written low byte first; the format and the
split of CGRAM between backgrounds and sprites are in
@ref hardware_ppu_cgram "The PPU: CGRAM and colours".

```c
void setup_palette(void) {
    // Set CGRAM address (palette 0)
    REG_CGADD = 0;

    // Color 0: Black (transparent for BG)
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x00;

    // Color 1: White
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    // Color 2: Red
    REG_CGDATA = 0x1F;
    REG_CGDATA = 0x00;

    // Color 3: Green
    REG_CGDATA = 0xE0;
    REG_CGDATA = 0x03;

    // Color 4: Blue
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x7C;
}
```

## Background Scrolling

`bgSetScroll()` records a layer's position; the NMI handler writes it to the
PPU during VBlank, and applies the vertical one-line correction the hardware
needs (see [Scrolling](scrolling.md)):

```c
s16 scroll_x = 0;
s16 scroll_y = 0;

while (1) {
    scroll_x++;
    bgSetScroll(0, scroll_x, scroll_y);
    WaitForVBlank();
}
```

## Parallax Scrolling

For depth, scroll the layers at different speeds:

```c
scroll_x++;
bgSetScroll(0, scroll_x, 0);          // BG1 (foreground): full speed
bgSetScroll(1, scroll_x >> 1, 0);     // BG2 (background): half speed
```

For a different speed on each band of the screen, see the HDMA parallax in
[Scrolling](scrolling.md).

## Converting pictures

A background picture becomes tiles, a tilemap and a palette with
@ref tools_opensnes_tileset. Put a settings file beside the picture — its
first line names the tool — and the build converts it before compiling
anything; nothing goes in the Makefile:

```toml
# res/opensnes.png.toml (examples/backgrounds/mode1)
tool = "opensnes-tileset"

[convert]
colors = 16
```

The same conversion by hand is `opensnes-tileset convert res/opensnes.png
--colors 16` (`--save` writes the settings file for you). It writes
`opensnes.pic` (the unique tiles), `opensnes.map` (the tilemap),
`opensnes.pal` and `opensnes.inc`, which declares the arrays and a ready
`BgAsset`:

```c
#include "res/opensnes.inc"   /* DECLARE_BG_ASSET(opensnes, BG_16COLORS, SC_32x32) */

bgLoad(0, &opensnes, 0, 0x4000, 0x0000);   /* BG1, palette slot 0, tiles, map */
```

The arrays behind it (`opensnes_tiles`, `opensnes_map`, `opensnes_pal`, each
with `_end`) work with `dmaCopyVram` / `dmaCopyCGram` when you want to place
each part yourself. Other keys of `[convert]` used by the examples: `bpp = 2`
and `colors = 4` for a Mode 0 layer, `bpp = 8` for Mode 3,
`mode = 7` for a Mode 7 plane, `priority = true`, `palette-entry = N` for a
layer that uses a later palette bank, `pages = true` for a 64x64 scrolling
map. Sprite sheets go through @ref tools_opensnes_sprite instead
(@ref tutorial_sprites).

`palette = "master.pal"` imposes a committed palette instead of deriving one
from the image's own colours. The build **fails** — naming the offending
colour — if the image uses any colour not in that palette. This is the fix
for the problem where adding one tile re-derives the whole palette and shifts
every existing tile's hue: author the palette once, commit it, and impose it
on every conversion so a new tile can only reuse existing entries
(`examples/games/rpg/res/tileset.png.toml` does this).

### Tiles made at run time

Art generated by the game — a dot, a bar, a glyph built from a bitmask —
starts as colour indices, one byte per pixel. `tileEncode4bpp()` (module
`tile`, `<snes/tile.h>`; `2bpp` and `8bpp` variants too) turns 64 of them,
row by row, into the planar tile the PPU reads:

```c
#include <snes/tile.h>              /* LIB_MODULES += tile */

static u8 pixels[64];
static u8 tile[32];
u8 i;
for (i = 0; i < 64; i++)
    pixels[i] = (i & 9) ? 3 : 0;    /* any pattern of colours 0-15 */
tileEncode4bpp(pixels, tile);
dmaCopyVram(tile, 0x1000, 32);      /* during VBlank or force blank */
```

A tile costs about 47 000 master cycles, an eighth of a frame: build tiles
while loading, or a few per frame. `backgrounds/mode2`,
`basics/game_skeleton`, `basics/panel_hud` and `sprites/sprite_swarm` build
their art this way.

## Compressed tiles (LZ77)

Tile data compresses well — it is repetitive by construction — and the SDK
can decompress it straight into VRAM, so the ROM carries the small form and
the PPU gets the big one. Add `lz = true` to the picture's settings file
and link the module:

```toml
# res/opensnes.png.toml (examples/backgrounds/mode1_lz77)
tool = "opensnes-tileset"

[convert]
colors = 16
lz = true
```

```makefile
LIB_MODULES := console lzss background sprite
```

```c
#include "res/opensnes.inc"   /* opensnes_tiles[] (compressed), opensnes_pal[], opensnes_map[] */

setScreenOff();            /* VRAM writes need forced blank */
lzssDecodeVram(opensnes_tiles, VRAM_BG_TILES);
setScreenOn();
```

With `lz = true` the `.inc` declares the arrays only, no `BgAsset`:
`bgLoad()` would copy the compressed bytes.

The saving is worth measuring rather than assuming. For the artwork in
`examples/backgrounds/mode1_lz77`, the compressed `.pic` is **652 bytes**
and its LZ77 header declares **2592 bytes** of tile data — the same 2592
bytes that `examples/backgrounds/mode1` stores uncompressed for the same
image. That is a quarter of the ROM footprint for one call at load time.

The format is the familiar GBA-style LZ77: a `0x10` tag byte, a three-byte
little-endian decompressed length, then flag bytes whose bits select a
literal or a back-reference. Any tool that emits that layout will do.

@warning `lzssDecodeVram` writes VRAM directly and disables interrupts
while it runs. Call it during forced blank, with the rest of your setup —
not from a running frame, where the PPU would drop the writes silently.

## Example: Complete Background Setup

See `examples/scrolling/parallax_scroll/` for a complete parallax scrolling example.

## Hi-res and interlace (Modes 5/6, SETINI)

BG Mode 5 renders **512 horizontal pixels** (16x8 tiles, stored as 8x8
character pairs N/N+1); adding screen interlace (`videoSetInterlace(1)`,
SETINI bit 0 through the lib's write-only shadow) doubles vertical to
**448 lines**. Three traps, all demonstrated by
`examples/backgrounds/mode5_hires`:

1. Hi-res content displays through BOTH screens: `setMainScreen(LAYER_BG1)`
   AND `setSubScreen(LAYER_BG1)`. The sub screen draws the even pixel
   columns and the main screen the odd ones (snesdev-wiki, Uncommon
   graphics mode games; its Backgrounds page has it inverted, a documented
   error in the corpus), so main-only leaves the even columns blank.
2. In interlace, tile texel rows map 1:1 to hi-res lines — a full-height
   page needs 56 tile rows (a 32x64 tilemap), not 28.
3. On a modern LCD the alternating columns show as fringing; a period CRT
   blended them. The example's README has the full explainer.

`videoSetObjInterlace`, `videoSetOverscan` and `videoSetPseudoHires`
cover the other SETINI bits — overscan shrinks the VBlank DMA budget by
~15 lines, see the header warning.

## Next Steps

- @ref tutorial_sprites "Sprites & Animation"
- @ref mode7.h "Mode 7 Reference"
