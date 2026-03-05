# Mixed Scroll

Two Mode 1 background layers displayed simultaneously with independent scroll
behavior: BG1 (a repeating shader pattern) auto-scrolls diagonally every frame,
while BG2 (a logo image) remains fixed in place. This demonstrates how the SNES
PPU can scroll each background layer independently, which is the foundation for
parallax effects, title screens with animated backgrounds, and layered game worlds.

![Screenshot](screenshot.png)

## What You'll Learn

- How to load two background layers with separate tilesets and palettes
- How to assign different CGRAM palette slots to different layers
- How to scroll one background while keeping another stationary
- How the SNES VRAM layout must be planned to avoid tile/tilemap overlap

## SNES Concepts

### Mode 1 with Multiple Backgrounds

Mode 1 provides three background layers: BG1 and BG2 are 4bpp (16 colors each),
and BG3 is 2bpp (4 colors). This example uses BG1 and BG2, disabling BG3 via
`REG_TM` ($212C). Each layer has its own tilemap and tile data in VRAM, and each
can reference a different 16-color palette slot from CGRAM.

### VRAM Layout Planning

With two active backgrounds, you must ensure their tile data and tilemaps do not
overlap in VRAM. This example uses:

| VRAM Address | Content | Size |
|-------------|---------|------|
| $1400 | BG2 tilemap (32x32) | 2048 bytes |
| $1800 | BG1 tilemap (32x32) | 2048 bytes |
| $4000 | BG1 tile data (shader) | variable |
| $5000 | BG2 tile data (logo) | variable |

The tilemaps are placed in the lower VRAM region ($1400-$1FFF) and tile character
data in the upper region ($4000+), leaving a safe gap between them.

### Palette Slots

CGRAM holds 256 colors organized as 16 slots of 16 colors each for 4bpp modes.
Each tilemap entry includes a 3-bit palette number that selects which 16-color
slot to use. In this example, BG2 uses palette slot 0 (colors 0-15) and BG1 uses
palette slot 1 (colors 16-31). The `-e 1` flag in gfx4snes tells the tool to
encode palette slot 1 into BG1's tilemap entries.

### Independent Background Scrolling

The SNES PPU has per-layer scroll registers: `BG1HOFS`/`BG1VOFS` ($210D/$210E)
for BG1 and `BG2HOFS`/`BG2VOFS` ($210F/$2110) for BG2. By calling
`bgSetScroll()` only for BG1 and never for BG2, the shader pattern scrolls
diagonally while the logo stays fixed at position (0, 0).

## Controls

This example has no interactive controls. BG1 auto-scrolls diagonally at one
pixel per frame.

## How It Works

### 1. Load Both Tilesets and Palettes

Each background gets its own tileset, palette, and palette slot. `bgInitTileSet()`
loads tile graphics to VRAM and palette colors to CGRAM in a single call:

```c
/* BG2: logo at VRAM $5000, palette slot 0 */
bgInitTileSet(1, &bg2_tiles, &bg2_pal, 0,
              &bg2_tiles_end - &bg2_tiles,
              &bg2_pal_end - &bg2_pal,
              BG_16COLORS, 0x5000);

/* BG1: shader at VRAM $4000, palette slot 1 */
bgInitTileSet(0, &bg1_tiles, &bg1_pal, 1,
              &bg1_tiles_end - &bg1_tiles,
              &bg1_pal_end - &bg1_pal,
              BG_16COLORS, 0x4000);
```

### 2. Load Tilemaps to VRAM

Tilemap data is DMA'd to the addresses configured in `bgSetMapPtr()`. This must
happen during VBlank or force blank since the PPU ignores VRAM writes during
active display:

```c
bgSetMapPtr(1, 0x1400, SC_32x32);
bgSetMapPtr(0, 0x1800, SC_32x32);

WaitForVBlank();
dmaCopyVram(&bg2_map, 0x1400, &bg2_map_end - &bg2_map);
dmaCopyVram(&bg1_map, 0x1800, &bg1_map_end - &bg1_map);
```

### 3. Enable Mode 1 with BG1 + BG2

Mode 1 is set and only BG1 and BG2 are enabled on the main screen. BG3 and
sprites are not used:

```c
setMode(BG_MODE1, 0);
REG_TM = TM_BG1 | TM_BG2;
```

### 4. Auto-Scroll BG1

Each frame, the scroll offset is incremented by 1 in both X and Y, producing
diagonal movement. BG2 has no scroll calls, so it stays fixed:

```c
while (1) {
    scrX++;
    scrY++;
    bgSetScroll(0, scrX, scrY);
    WaitForVBlank();
}
```

## Project Structure

```
mixed_scroll/
├── main.c        — Initialization, tileset loading, scroll loop
├── data.asm      — ROM data: tiles, tilemaps, and palettes for both layers
├── Makefile      — Build configuration (gfx4snes rules for both PNGs)
└── res/
    ├── shader.png    — Repeating pattern for BG1 (palette slot 1)
    └── pvsneslib.png — Logo image for BG2 (palette slot 0)
```

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/graphics/backgrounds/mixed_scroll
```

Then open `mixed_scroll.sfc` in your emulator (Mesen2 recommended). You should
see a logo image with a colorful pattern scrolling diagonally behind/over it.
