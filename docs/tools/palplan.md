# palplan — project shared-palette planner {#tools_palplan}

The SNES gives you exactly **8 background palettes and 8 sprite palettes** of 16
colours each — 256 CGRAM entries, no more. Every `.pal` your game loads has to
land in one of those slots, at a CGRAM colour index you pass to `dmaCopyCGram`.
Pick two overlapping indices and one asset silently recolours the other; leave a
slot half-used and you run out sooner than you had to. Fitting many palettes
into that space is the single most-cited SNES asset headache, and nothing in the
pipeline helped — until palplan.

palplan reads a manifest of your project's palettes and plans the whole CGRAM
layout for you: it merges identical palettes, assigns collision-free slots,
tells you when you have asked for more than the hardware has, and writes a C
header so your code never hard-codes a magic index again.

## What goes in, what comes out

**In:** a small text **manifest** listing your `.pal` files (the BGR555 palettes
@ref tools_gfx4snes writes with `-p`), each tagged `bg` or `sprite`.

**Out:**

| File | What it is | Loaded with |
|------|-----------|-------------|
| *(stdout)* | the allocation table + collision / near-duplicate report | — (read it) |
| `.h` (`-o`) | named CGRAM offsets: `PAL_<NAME>_CGRAM`, `_SLOT`, `_COLORS` | `#include` |
| `.pal` (`-b`) | one combined 512-byte CGRAM image | `dmaCopyCGram(img, 0, 512)` |

## The manifest

One palette per line, `#` comments, three whitespace columns — `name`, `type`
(`bg` or `sprite`), `file`. Paths resolve relative to the manifest's directory:

```
# name    type    file
sky       bg      res/sky.pal
town      bg      res/town.pal
hero      sprite  res/hero.pal
enemy     sprite  res/enemy.pal
```

## The flags you will actually use

| Flag | Meaning |
|------|---------|
| `-o FILE` | write the C header of named CGRAM offsets |
| `-b FILE` | write the combined 512-byte CGRAM image (one-shot DMA) |
| `-t N` | near-duplicate hint threshold, in differing colours (default 2, `0` = off) |
| `-q` | quiet — suppress the table, keep hints and errors |

## Worked examples

```sh
# Plan the layout and read the report:
palplan project.txt

# Generate the header your game includes:
palplan -o src/palplan.h project.txt

# Header + a single combined CGRAM image to DMA in one shot:
palplan -o src/palplan.h -b res/palplan.pal project.txt

# Only flag palettes that differ by a single colour (stricter merge hints):
palplan -t 1 project.txt
```

The header turns a hand-counted index into a name:

```c
#include "palplan.h"

// hero was assigned sprite slot 0 -> CGRAM 128
dmaCopyCGram((u8 *)hero_pal, PAL_HERO_CGRAM, PALETTE_16_SIZE);
```

and `PAL_<NAME>_SLOT` is the palette bank you hand to `gfx4snes -e` so a
tilemap's entries reference the matching slot.

## Gotchas worth knowing up front

- **v1 merges only identical palettes.** A merge palplan performs is always the
  byte-for-byte case, so it can never make your colours silently wrong. Palettes
  that are *nearly* the same are reported as merge candidates, not merged —
  fusing them means re-indexing tile pixels, which palplan does not touch.
- **Every palette takes a full 16-colour slot.** A 4-colour (2bpp) palette still
  reserves a whole slot in v1; the report shows the real colour count so you see
  the waste. Tight-packing 2bpp palettes is v2 (the hardware ties the 2bpp base
  to the BG number).
- **BG and sprite palettes never share.** They live in different CGRAM regions
  (0–127 vs 128–255) and are referenced differently, so identity-merge only
  applies within a region.
- **Over-subscription is a hard error, not a warning.** More than 8 distinct BG
  or sprite palettes exits non-zero — the report points you at the near-
  duplicates to merge first.

## See it in practice

- @ref examples_basics_collision_demo — loads three palettes into three distinct
  slots by hand (`OBJ_CGRAM_BASE`, `OBJ_CGRAM_PAL(1)`, BG at 0) — exactly the
  offsets palplan would name for you.
- @ref examples_games_rpg — a fixed-palette project with backgrounds *and*
  sprites, the kind of asset count where a planner earns its keep.

The **why** behind the 8+8 slot budget and the BG-0–127 / sprite-128–255
convention is @ref craft_planning. Read it alongside this page.
