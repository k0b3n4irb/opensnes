# opensnes-sprite — sheets, metasprites and Aseprite clips {#tools_opensnes_sprite}

The sprite artist's tool of the 1.x family (@ref tools_conventions). It
takes what the artist exports — a sprite sheet, an Aseprite animation — and
writes what the OBJ layer and the `anim` module consume. The converters are
`gfx4snes`'s and `aseprite2snes`'s, linked as libraries: a `.pic` or a
`_meta.inc` from here is byte for byte theirs.

## Sheet

```sh
opensnes-sprite sheet res/hero.png --size 16                          # tiles + palette
opensnes-sprite sheet res/hero.png --size 16 --colors 16 --metasprite 32 48 --priority 2
opensnes-sprite sheet res/hero.png --size 16 --metasprite 32 48 --flip --save   # remember it beside the file
```

**In:** an indexed PNG or BMP, cut into `--size` blocks (8, 16, 32 or 64,
the OBJ sizes). A sheet that is not a multiple of the block size gets a
warning: the last row or column is padded with pixels that are not there.

**Out**, beside the input or in `--out`:

| File | What it is |
|---|---|
| `hero.pic` | the 8x8 tiles in OBJ VRAM order (128-px raster rows), `--bpp` deep (4 by default); `--lz` compresses, `--pack` writes packed pixels |
| `hero.pal` | the palette, `--colors` entries (256 by default; 16 for one OBJ palette); `--no-palette` skips it |
| `hero.inc`, `hero_data.as` | the glue, in the lib's `asset.h` naming: `hero.inc` declares `hero_tiles`, `hero_tiles_end`, `hero_pal`, `hero_pal_end` and a ready `GfxAsset hero` (`DECLARE_GFX_ASSET`); `hero_data.as` is the `.incbin` fragment the build gathers into `assets_gen.asm` |
| `hero_meta.inc` | with `--metasprite W H`: one `t_metasprite` table per W x H cell of the sheet, `METASPR_ITEM(x, y, tile, OBJ_PAL(n) \| OBJ_PRIO(p))` entries for `oamDrawMetasprite`; with `--flip` an entry whose block is the mirror of another names that block and carries `OBJ_FLIPX` / `OBJ_FLIPY` |
| `hero_blocks.inc` | with `--compact`: `hero_blocks[]`, one word per block of the sheet in reading order — the block of the `.pic` that holds its pixels, bit 14 if it is drawn mirrored in x, bit 15 in y — and `hero_BLOCKS` / `hero_STORED_BLOCKS` |

**`--flip` alone does not make the `.pic` smaller.** It only says, in the
metasprite table, that a block is another one mirrored; every block of the
sheet is still written, mirrors included. To save ROM and VRAM add
`--compact`: each distinct block is written once (with `--flip`, a block and
its mirrors count as one), and `hero_blocks.inc` tells the game where each
block of the sheet went:

```c
#include "res/players_blocks.inc"

u16 b = players_blocks[frame];            /* frame = the block's number in the sheet */
const u8 *src = players_tiles + my_block_offset(b & 0x01FF);   /* yours: where block k starts in the .pic's 128-px raster */
u16 flips = b & (0x4000 | 0x8000);        /* OBJ_FLIPX / OBJ_FLIPY of an OAM word */
```

This is the table a game that streams its frames needs; a metasprite table
written with `--compact` follows the compacted sheet too. On a sheet of
four 32x32 blocks, A, A mirrored, B, B mirrored, `--flip --compact` keeps
two and the table reads `0, 0|FLIPX, 1, 1|FLIPX`.

`--palette FILE` imposes a raw `.pal`: the indices stay stable when the
sheet changes, and a colour the file does not hold is refused. A tile whose
pixels fall in two palette banks is refused too, with the tile and the
pixel named — the hardware gives one palette per tile.

## Anim

```sh
aseprite -b hero.aseprite --sheet res/hero.png --data res/hero.json --list-tags --format json-array
opensnes-sprite anim res/hero.json --prefix hero     # → res/hero_anim.h
```

One `AnimClip` per Aseprite tag, the frame indices in playback order (the
direction folded in: forward, reverse, pingpong), the per-frame durations
in ticks at `--fps` (60), `ANIM_ONCE` when the tag repeats once. Frame
values index the metasprite table `sheet` wrote, so clip frame *i* draws
metasprite *i*; `--stride N` multiplies them for a single-sprite animation
whose frames are consecutive tile numbers. @ref examples_sprites_aseprite_pipeline
is the worked example.

## Inspect

```sh
opensnes-sprite inspect res/*.png --size 16
# hero.png: 128x96 px, 48 blocks of 16 (192 tiles, 6144 bytes of VRAM at 4 bpp), 14 colours used, highest index 13
```

Nothing is written: the VRAM cost of a sheet before it is drawn.

## Settings beside the asset

`--save` writes `hero.png.toml` (`[sheet]`: size, colors, metasprite,
priority, flip…) and `hero.json.toml` (`[anim]`: prefix, stride, fps);
every later run reads them, the command line overrides for one run.

## From gfx4snes and aseprite2snes

| 0.x | opensnes-sprite |
|---|---|
| `gfx4snes -s 16 -p -i hero.png` | `sheet hero.png --size 16` |
| `-o 16 -u 16` | `--colors 16 --bpp 4` |
| `-T -X 32 -Y 48 -P 2 -F` | `--metasprite 32 48 --priority 2 --flip` |
| `-c fixed.pal`, `-e N` | `--palette fixed.pal`, `--palette-entry N` |
| `-k`, `-z`, `-b`, `-d` | `--pack`, `--lz`, `--blank`, `--round` |
| `aseprite2snes -o hero_anim.h -p hero -t N -f 60 hero.json` | `anim hero.json --prefix hero --stride N --fps 60` |

Backgrounds and tilemaps (`gfx4snes -m`) are `opensnes-tileset`'s, the next
tool of the family.
