# Mode 1 BG3 High Priority — HUD Layer Over Backgrounds

## What This Example Shows

How to use **BG3 as a high-priority HUD overlay** in Mode 1. Normally BG3 is the
lowest-priority background layer, but the `BG3_MODE1_PRIORITY_HIGH` flag promotes
it to the highest priority — rendering on top of BG1 and BG2.

This is the standard SNES technique for status bars, health displays, and score
overlays in Mode 1 games (Zelda ALttP, Streets of Rage, Final Fantasy, etc.).

## How It Works

### 1. Set up three background layers

```c
bgSetMapPtr(0, 0x0000, SC_32x32);  /* BG1 tilemap */
bgSetMapPtr(1, 0x0400, SC_32x32);  /* BG2 tilemap */
bgSetMapPtr(2, 0x0800, SC_32x32);  /* BG3 tilemap */
```

Each BG gets its own tilemap region in VRAM. BG1 and BG2 are 4bpp (16 colors),
BG3 is 2bpp (4 colors) — that's how Mode 1 works on the SNES.

### 2. Load tiles and palettes

```c
bgInitTileSet(0, &bg1_tiles, &bg1_pal, 2, ..., BG_16COLORS, 0x2000);
bgInitTileSet(1, &bg2_tiles, &bg2_pal, 4, ..., BG_16COLORS, 0x3000);
bgInitTileSet(2, &bg3_tiles, &bg3_pal, 0, ..., BG_16COLORS, 0x4000);
```

Each BG has its own palette slot (2, 4, 0). The palette slot is encoded in
the tilemap entries, so tiles reference the correct colors automatically.

### 3. Enable BG3 high priority

```c
setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
REG_TM = TM_BG1 | TM_BG2 | TM_BG3;
```

The `BG3_MODE1_PRIORITY_HIGH` flag (bit 3 of the BGMODE register) changes
BG3's rendering order from lowest to highest. Without this flag, the HUD
would be hidden behind the other two layers.

## SNES Concepts

- **Mode 1 layer priorities**: Normally BG1 > BG2 > BG3. With the high-priority
  flag, BG3 tiles with priority bit set render above everything else.
- **BG3 is 2bpp**: Only 4 colors per palette in Mode 1. Enough for simple
  HUD elements (text, health bars, icons) but not detailed artwork.
- **Tilemap palette bits**: Each tilemap entry encodes a palette number
  (bits 12-10). The converter bakes this in, so `bgInitTileSet` palette slot
  must match what's in the map data.

## VRAM Layout

| Address | Content | Size |
|---------|---------|------|
| `$0000` | BG1 tilemap | 2048 bytes |
| `$0400` | BG2 tilemap | 2048 bytes |
| `$0800` | BG3 tilemap | 2048 bytes |
| `$2000` | BG1 tiles (4bpp) | ~2.5 KB |
| `$3000` | BG2 tiles (4bpp) | ~1 KB |
| `$4000` | BG3 tiles (2bpp) | 128 bytes |

## Next Steps

- `backgrounds/continuous_scroll` — Parallax scrolling with multiple BG layers
- `effects/hdma_wave` — Per-scanline effects on backgrounds
