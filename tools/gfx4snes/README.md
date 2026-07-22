# gfx4snes

Converts PNG or BMP images to SNES tile, palette, and tilemap formats.

This is the primary asset tool for OpenSNES — every background and sprite in every example was converted with gfx4snes.

## Usage

```bash
gfx4snes [options] -i <input_file>
```

The input file must be a 256-color indexed PNG or BMP.

## Output Files

| Extension | Content |
|-----------|---------|
| `.pic` | Tile graphics (2bpp, 4bpp, or 8bpp bitplane format) |
| `.pal` | Palette (BGR555, 2 bytes per color) |
| `.map` | Tilemap (16-bit entries: tile number + palette + flip bits) |
| `.inc` | Auto-generated C header with size constants |
| `.pc7` | Packed tile data (Mode 7 only) |
| `.mp7` | Mode 7 tilemap |

## Common Flag Patterns

### 4bpp Background (Mode 1) — most common

```bash
gfx4snes -s 8 -o 16 -u 16 -p -m -i background.png
```

### 4bpp Background with palette bank offset

```bash
gfx4snes -a -s 8 -o 16 -u 16 -e 1 -p -m -i background.png
```

`-e 1` puts palette bits in tilemap entries so tiles use palette bank 1. `-a` rearranges colors across banks automatically.

### 2bpp Background (Mode 0, or BG3 in Mode 1)

```bash
gfx4snes -s 8 -o 4 -u 4 -p -m -i overlay.png
```

### 8bpp Background (Mode 3)

```bash
gfx4snes -s 8 -o 256 -u 256 -p -m -i fullcolor.png
```

### Mode 5 Hi-Res

```bash
gfx4snes -m -u 16 -M 5 -i hires.png
```

### Mode 7

```bash
gfx4snes -p -m -M 7 -i mode7bg.png
```

### 16x16 Sprites

```bash
gfx4snes -s 16 -o 16 -u 16 -p -i sprites.png
```

### 32x32 Metasprites with transpose

```bash
gfx4snes -s 32 -o 16 -u 16 -p -T -X 64 -Y 64 -P 2 -i metasprite.png
```

### Scrolling backgrounds (32x32 page interleaving)

```bash
gfx4snes -y -s 8 -o 16 -u 16 -e 2 -p -m -i scrollbg.png
```

### LZ77 compressed tiles

```bash
gfx4snes -z -s 8 -o 16 -u 16 -p -m -i background.png
```

## Imposing a palette (`-c`)

By default the palette is derived from the image, which makes it a
function of the **whole** image: add one tile and all 16 colours are
re-derived, so every tile already drawn shifts hue. Nothing reports it —
you notice when you compare two screenshots. It cost the RPG template its
road colour when a blue-roofed house was added to a 12-tile town sheet.

`-c` imposes a palette you author and commit:

```bash
gfx4snes -s 8 -p -m -c town_fixed.pal -i tileset.png
```

- every colour in the image must exist in `town_fixed.pal`, or the tool
  fails naming the colour, its 5-bit components and the pixel it is at;
- indices come from the file, so they are stable across edits;
- the emitted `.pal` is the imposed palette, unchanged.

The file is raw SNES palette data — 16-bit little-endian BGR555 entries,
the same format `-p` writes. `examples/games/rpg` generates both of its
palettes this way (`gen_assets.py`, `write_pal()`), so a colour that
creeps into the art without being declared is a build error rather than a
silent recolouring of the whole scene.

## Flag Reference

### Tiles

| Flag | Description |
|------|-------------|
| `-s N` | Tile size in pixels: 8, 16, 32, 64 (default: 8) |
| `-b` | Add blank tile 0 (for multi-BG setups) |
| `-k` | Output in packed pixel format |
| `-z` | LZ77 compress tile output |
| `-F` | Deduplicate horizontally/vertically flipped tiles |

### Maps

| Flag | Description |
|------|-------------|
| `-m` | Generate tilemap output (.map) |
| `-f N` | Tile number offset in tilemap (0-2047) |
| `-g` | Set high priority bit in all tilemap entries |
| `-y` | Generate tilemap in 32x32 pages (for scrolling) |
| `-R` | Disable tile reduction (no deduplication) |
| `-M N` | Special mode: 1 (default), 5, 6, or 7 |

### Palettes

| Flag | Description |
|------|-------------|
| `-p` | Generate palette output (.pal) |
| `-c FILE` | Impose FILE as the palette; fail if the image uses a colour that is not in it |
| `-a` | Rearrange palette across banks (preserves tilemap references) |
| `-e N` | Palette entry offset in tilemap (0-7) |
| `-o N` | Number of colors to output (0-256) |
| `-u N` | Number of colors to use per tile: 4, 16, 128, 256 |
| `-d` | Round palette to SNES precision (max 63 per channel) |

### Metasprites

| Flag | Description |
|------|-------------|
| `-T` | Enable metasprite output |
| `-X N` | Metasprite width in pixels |
| `-Y N` | Metasprite height in pixels |
| `-P N` | Metasprite priority (0-3) |
| `-S` | Print sprite tile number map |

### Files

| Flag | Description |
|------|-------------|
| `-i FILE` | Input PNG or BMP file |
| `-t TYPE` | Force input type: `png` or `bmp` |

## Makefile Integration

```makefile
res/bg.pic res/bg.pal res/bg.map: res/bg.png
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<
```

The `$(GFX4SNES)` variable is set by `common.mk` to point to `$(OPENSNES)/bin/gfx4snes`.

## Attribution

Based on gfx4snes/pcx2snes by Alekmaul (PVSnesLib). License: zlib.
