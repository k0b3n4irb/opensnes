# img2snes — RGB artwork to indexed palettes {#tools_img2snes}

`gfx4snes` needs **indexed** images, but artists work in RGB. `img2snes` is the
bridge: it quantises a full-colour PNG down to a SNES-legal indexed palette, so
your artwork feeds the rest of the pipeline without hand-editing palettes in an
image editor. Run it once on new art, commit the indexed result, and gfx4snes
takes it from there.

## What goes in, what comes out

**In:** a `.png` — RGB, RGBA, or already indexed.

**Out:** a palette-indexed `.png` (2–256 colours, palette embedded). The default
name is `<input>_indexed.png`.

## The flags you will actually use

| Flag | Meaning |
|------|---------|
| `-i FILE` | input image (**required**) |
| `-o FILE` | output path (default `<input>_indexed.png`) |
| `-c N` | number of colours, 2–256 (16 for 4bpp, 4 for 2bpp, 256 for 8bpp) |
| `--round-snes` | snap the palette to SNES BGR555 (multiples of 8) — **recommended** |
| `-p FILE` | match pixels to an existing PNG's palette (shared palette across sprites) |
| `-s FACTOR` | nearest-neighbour scale |
| `-t N` | pad dimensions up to a multiple of N (e.g. 8) |
| `--batch` | treat the input as a glob and convert many files |

## Worked examples

```sh
# Quantise a character sprite to 16 colours, SNES-rounded:
img2snes -i character.png -c 16 --round-snes

# A 4-colour (2bpp) font source, explicit output name:
img2snes -i font.png -c 4 -o font_4col.png

# Force several sprites to share one palette (pass a reference PNG):
img2snes -i enemy.png -p hero_indexed.png
```

Then hand the indexed PNG to @ref tools_gfx4snes with a matching `-u` (16
colours → `-u 16`, etc.).

## Gotchas worth knowing up front

- **Transparency = index 0.** Pixels with alpha below 128 map to colour 0, which
  the SNES treats as transparent — so paint your background in fully-transparent
  pixels, not a "magic" colour.
- **`--round-snes` early.** The SNES only has 15-bit colour; rounding at
  quantisation time means what you see is what the hardware shows, with no
  surprise shifts later.
- **`-p` for shared palettes.** When several sprites must live in one 16-colour
  sub-palette (the @ref craft_planning constraint), quantise the first, then map
  the rest onto its palette with `-p`.
- **Optional and standalone.** img2snes is not wired into the build — it is a
  one-time artist pre-step. Its output is what you commit and reference.

## Where it fits

img2snes is the very first box in the @ref tools pipeline: RGB art → indexed PNG
→ @ref tools_gfx4snes → tiles/palette/map. If your source art is already indexed
(pixel-art tools often export that way), you can skip it entirely.
