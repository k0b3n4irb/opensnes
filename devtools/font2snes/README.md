# font2snes.py — 1bpp Font to SNES Tile Converter

Converts a 1bpp font (defined as a C header with pixel data) to SNES 2bpp or
4bpp tile format. This is the Python reference implementation; the compiled
C version lives in `tools/font2snes/`.

## When to use

- **Generating font tiles** for the text rendering system
- **Debugging font conversion** — the Python version is easier to inspect than the C tool

## Dependencies

None (Python stdlib only).

## Usage

```bash
# Convert to 2bpp binary (default)
python3 devtools/font2snes/font2snes.py opensnes_font.h font.bin

# Convert to 4bpp binary
python3 devtools/font2snes/font2snes.py -b 4 opensnes_font.h font_4bpp.bin

# Output as C header instead of binary
python3 devtools/font2snes/font2snes.py -c opensnes_font.h font_2bpp.h
```

## Options

| Flag | Description |
|------|-------------|
| `-b`, `--bpp {2,4}` | Bits per pixel (default: 2) |
| `-c`, `--c-header` | Output as C header instead of binary |
| `-v`, `--verbose` | Verbose output |
