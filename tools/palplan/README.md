# palplan — project shared-palette planner

The SNES holds 256 CGRAM colours: **8 background palettes** (indices 0–127) and
**8 sprite palettes** (indices 128–255), 16 colours each. A real game has far
more `.pal` files than slots, and today you hand-pick the `startColor` offset
passed to `dmaCopyCGram()` for every one of them — with nothing to catch a
collision or a wasted slot. palplan is the planner that closes that gap.

Give it a manifest of your project's palettes, and it:

- **merges byte-identical palettes** so they share one slot (always safe),
- **assigns every distinct palette** a collision-free slot + CGRAM index,
- **fails loudly** if you need more than 8 BG or 8 sprite slots,
- **flags near-duplicates** as candidates for a hand-merged shared palette,
- **emits a C header** of named CGRAM offsets, and optionally one combined
  512-byte CGRAM image you can DMA in a single shot.

## Usage

```sh
palplan [options] <manifest>

  -o FILE   write a C header of named CGRAM offsets (PAL_<NAME>_CGRAM)
  -b FILE   write a combined 512-byte CGRAM image (one-shot DMA)
  -t N      near-duplicate hint threshold in colours (default 2, 0 = off)
  -q        quiet: suppress the allocation table (hints/errors stay)
  -h        help
  -v        version
```

## The manifest

One palette per line, `#` comments, three whitespace-separated columns —
`name`, `type` (`bg` or `sprite`), and the `.pal` file. Paths are resolved
relative to the manifest's own directory.

```
# name    type    file
sky       bg      res/sky.pal
town      bg      res/town.pal
hero      sprite  res/hero.pal
enemy     sprite  res/enemy.pal
```

`.pal` files are the raw BGR555 palettes @ref tools_gfx4snes writes with `-p`
(little-endian, 2 bytes/colour, ≤ 16 colours per palette for v1).

## What you get

```
$ palplan -o palplan.h -b palplan.pal project.txt
palplan — 4 palettes, 4 distinct

BG palettes (2 / 8 slots used):
  slot 0  cgram   0  sky              (16 colours)  res/sky.pal
  slot 1  cgram  16  town             (16 colours)  res/town.pal

Sprite palettes (2 / 8 slots used):
  slot 0  cgram 128  hero             (16 colours)  res/hero.pal
  slot 1  cgram 144  enemy            (16 colours)  res/enemy.pal

OK: fits in 8 BG + 8 sprite slots.
```

The generated header names every offset so the game never hard-codes a magic
index:

```c
#include "palplan.h"

dmaCopyCGram((u8 *)hero_pal, PAL_HERO_CGRAM, PALETTE_16_SIZE);  // hero -> 128
```

`PAL_<NAME>_SLOT` is the palette bank number — pass it to `gfx4snes -e` so a
tilemap's entries reference the right slot, or to `OBJ_CGRAM_PAL()`.

The combined image (`-b`) is the whole planned CGRAM in one buffer; load it in a
single DMA and every palette lands where the header says:

```c
extern u8 palplan_pal[];
dmaCopyCGram(palplan_pal, 0, 512);   // all slots at once
```

## Scope (v1) and what it deliberately does not do

palplan v1 is an **allocator over existing `.pal` files**. It never rewrites
tile pixel data, so the only merge it performs is the **byte-identical** case,
which is always safe. That keeps it correct by construction — it can never make
your colours silently wrong.

Two things are intentionally left for v2:

- **Tight-packing sub-16-colour (2bpp) palettes.** A 4-colour Mode 0 palette
  still takes a full 16-colour slot here, because the hardware palette-index
  math for 2bpp couples the base to the BG number. palplan reports the real
  colour count so you know the waste; it does not yet sub-allocate.
- **Re-quantising to merge non-identical palettes** ("repack on demand"). Doing
  that means re-indexing the tiles that reference the palette — data palplan
  does not see. Instead it *flags* near-duplicates as merge candidates and
  leaves the decision (and the re-quantise, upstream in
  [tiledpalettequant](https://github.com/rilden/tiledpalettequant) or
  [SuperFamiconv](https://github.com/Optiroc/SuperFamiconv)) to you.

## Tests

`python3 tools/palplan/tests/run_golden.py` (also run by `make test-tools`)
runs the tool on a committed manifest of BGR555 fixtures and byte-compares the
generated header and combined image against goldens, plus asserts the
over-subscription hard-fail. palplan is deterministic (slots assigned in
manifest order), so any diff is a real change. The fixtures are synthesized
BGR555 palettes generated in-repo, not third-party assets.
