# opensnes-palette — a project's palettes {#tools_opensnes_palette}

The palette tool of the 1.x family (@ref tools_conventions). Two jobs a
game has around colour, before and after the picture tools: **planning**
how every `.pal` of the project fits the SNES's 8 background and 8 sprite
palettes of 16 colours (CGRAM 0..127 and 128..255), and **quantizing** the
RGB art an artist draws into the indexed PNG that @ref tools_opensnes_sprite
and @ref tools_opensnes_tileset eat. The planner is `palplan`'s
(@ref tools_palplan) and the quantizer `img2snes`'s (@ref tools_img2snes),
linked as libraries: same slots, same bytes.

## Plan

```sh
opensnes-palette plan res/palettes.toml                                  # the plan written in the settings file
opensnes-palette plan res/palettes.toml --sprite "hero.pal npc.pal" --save   # write that file
```

A plan is a **composed asset**: `res/palettes.toml` lists the project's
`.pal` files (as `opensnes-sprite` and `opensnes-tileset` write them beside
each picture), in slot order, relative to the file:

```toml
tool = "opensnes-palette"

[plan]
bg = ["town.pal", "sky.pal", "hud.pal"]
sprite = ["hero.pal", "npc.pal"]
cgram = true        # also write the 512-byte CGRAM image
```

Identical palettes share a slot; every palette gets its CGRAM index and its
slot number; a ninth distinct palette in a region is **refused** (exit 1),
with the near-duplicates named so you know which two to merge. The plan
prints as a table, and `--hints N` sets how close two palettes must be to
be named (2 differing colours by default; 0 never).

**Out**, beside the file or in `--out`:

| File | What it is |
|---|---|
| `palettes.inc` | `PAL_<NAME>_CGRAM` (the `startColor` of `dmaCopyCGram`), `PAL_<NAME>_SLOT` (the OAM palette number, or the palette bank of a tilemap entry) and `PAL_<NAME>_COLORS` for each palette, `<NAME>` the file's stem in capitals |
| `palettes.pal`, `palettes_data.as` | with `cgram = true`: the whole plan as one 512-byte CGRAM image, `palettes_cgram` in the `.inc`, for a single `dmaCopyCGram(palettes_cgram, 0, 512)` |

```c
#include "res/palettes.inc"

dmaCopyCGram(hero_pal, PAL_HERO_CGRAM, 32);
oamSet(0, x, y, hero_tile, PAL_HERO_SLOT, 3, 0);
```

The build runs a plan **after** the pictures' conversions, so the `.pal`
files it reads are fresh (`make/common.mk`). `examples/games/rpg` plans its
sprite palettes this way.

## Quantize

```sh
opensnes-palette quantize art/hero.png --colors 16 --round     # -> art/hero_indexed.png
opensnes-palette quantize art/enemy.png --palette res/hero.pal  # the hero's colours, for a shared slot
```

**In:** an RGB or RGBA PNG. **Out:** `<stem>_indexed.png`, an indexed PNG
of `--colors` colours (16 for 4 bpp, 4 for 2 bpp, 256 for 8 bpp) chosen by
median cut, or mapped onto the colours of `--palette` (a `.pal`, or a PNG's
palette) when several pictures must share a slot. Transparent pixels (alpha
below 128) become index 0, the colour the SNES does not draw. `--round`
snaps the palette to the 15-bit colour of the hardware, so the picture
shows what the console will; `--scale PERCENT` resizes first (nearest
neighbour) and `--align N` pads the size to a multiple of N.

This is the one step of the pipeline meant to be **looked at**: run it on
new art, open the result, commit it, and let the picture tools work from
the indexed file. Pixel-art tools that export indexed PNGs skip it.

## Inspect

```sh
opensnes-palette inspect res/*.pal
# hero.pal: 16 colours (16 distinct, 1 black), 32 bytes — one 16-colour slot
```

`-v` prints the colours in hex; `--json` for a script.

## From palplan and img2snes

| 0.x | opensnes-palette |
|---|---|
| `palplan -o plan.h -b plan.pal project.txt` (a three-column manifest) | `plan palettes.toml` with `bg = [...]`, `sprite = [...]`, `cgram = true` |
| `name type file` lines | the file's stem is the name; two lists are the types |
| `-t N` | `--hints N` |
| `img2snes -i art.png -c 16 --round-snes` | `quantize art.png --colors 16 --round` |
| `-p ref.png` | `--palette ref.pal` or `--palette ref.png` |
| `-s 2.0`, `-t 8` | `--scale 200`, `--align 8` |

The same CGRAM image and the same `PAL_` macros as palplan, the same
indexed PNGs as img2snes (both golden suites are the family's). One change
under both tools: the quantizer's sort now breaks ties by index, so a
picture quantizes to the same bytes on every OS (it depended on the C
library's `qsort` before).

## See also

- @ref craft_planning — why 8 + 8 slots, and how to budget them.
- @ref tools_palplan, @ref tools_img2snes — the 0.x tools; shipped for one more version.
