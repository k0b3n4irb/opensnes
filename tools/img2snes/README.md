# img2snes

PNG RGB/RGBA to indexed PNG converter for the OpenSNES toolchain.

## Why

Artists work in RGB/RGBA. The SNES graphics converter `gfx4snes` expects
palette-indexed PNGs. `img2snes` bridges the gap: it quantizes colors,
handles transparency, and optionally rounds the palette to SNES BGR555
precision — so the artist's PNG goes straight into the asset pipeline
without manual palette editing.

```
Artist PNG (RGB/RGBA)
       |
   img2snes          ← quantize, scale, align, round palette
       |
Indexed PNG (2-256 colors)
       |
   gfx4snes          ← tile conversion, palette extraction
       |
.pic .pal .map       ← SNES-ready assets
```

## Features

- **Median-cut quantization** — reduces colors while preserving visual quality
- **Reference palette** — map pixels to an existing palette (`-p palette.png`)
- **SNES BGR555 rounding** — snaps RGB values to multiples of 8 (`--round-snes`)
- **Nearest-neighbor scaling** — resize with pixel-perfect sharpness (`-s 2.0`)
- **Tile alignment** — round dimensions up to multiples of N (`-t 8`)
- **Transparency** — alpha < 128 maps to index 0 (SNES color 0 = transparent)
- **2 to 256 colors** — works for 2bpp (4 colors), 4bpp (16), or 8bpp (256)

## Build

```bash
make              # builds img2snes in current directory
make install      # copies binary to ../../bin/ (OpenSNES bin/)
make clean        # removes build artifacts
```

Requires `clang` (or set `CC=gcc`). Links against `-lm` only.
All dependencies (lodepng, cmdparser) are embedded in `src/`.

## Usage

```
img2snes -i <input.png> [options]
```

### Options

| Flag | Long | Description | Default |
|------|------|-------------|---------|
| `-i` | `--input` | Input PNG file (RGB, RGBA, or indexed) | required |
| `-o` | `--output` | Output PNG file | `<input>_indexed.png` |
| `-c` | `--colors` | Number of colors (2-256) | 16 |
| `-s` | `--scale` | Scale factor | 1.0 |
| `-p` | `--palette` | Reference PNG to extract palette from | — |
| `-t` | `--tile` | Align dimensions to multiple of N | — |
| | `--round-snes` | Round palette to SNES BGR555 precision | off |
| `-q` | `--quiet` | Suppress output messages | off |
| `-v` | `--verbose` | Show detailed processing info | off |
| `-V` | `--version` | Show version and exit | — |

### Examples

Basic conversion (16 colors, SNES-rounded):

```bash
img2snes -i character.png --round-snes
# → character_indexed.png
```

4-color sprite for BG3 (2bpp):

```bash
img2snes -i font.png -c 4 -o font_4col.png
```

Scale down a hi-res mockup and tile-align to 8px:

```bash
img2snes -i mockup_2x.png -s 0.5 -t 8 --round-snes -o tileset.png
```

Force a shared palette across multiple sprites:

```bash
img2snes -i hero.png -p shared_palette.png -o hero_indexed.png
img2snes -i enemy.png -p shared_palette.png -o enemy_indexed.png
```

## How quantization works

1. Load PNG as RGBA (via lodepng)
2. Scale if requested (nearest-neighbor)
3. Detect transparency (any pixel with alpha < 128 reserves index 0)
4. Run median-cut algorithm on opaque pixels to find optimal palette
5. Map each pixel to nearest palette entry (or index 0 if transparent)
6. Optionally round palette RGB values to SNES BGR555 (multiples of 8)
7. Write indexed PNG with embedded palette

When a reference palette is provided (`-p`), step 4 is skipped — pixels
are mapped directly to the reference palette's colors using nearest-match.

## License

MIT — see source headers for details.
lodepng: zlib license. cmdparser: Apache 2.0.
