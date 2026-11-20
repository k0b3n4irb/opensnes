# opensnes-tilering — maps with more characters than VRAM holds {#tools_opensnes_tilering}

The converter of the `tilering` library module (@ref tutorial_scrolling_tilering),
in the `opensnes-*` family (@ref tools_conventions). A background tilemap
entry names 1,024 characters at most; a large hand-drawn map often has
1,200 to 1,600 distinct ones. The screen shows far fewer at once, and the
module keeps those in a ring of VRAM slots. This tool turns the map picture
into what the module streams, and measures what it will cost.

## Convert

```sh
opensnes-tilering convert res/world.png              # 33x29 window, mirrors merged
opensnes-tilering convert res/world.png --priority   # every cell in front of the sprites of priority 0-2
opensnes-tilering convert res/world.png --save       # writes res/world.png.toml
```

In a project, the settings file beside the picture is enough:

```toml
tool = "opensnes-tilering"

[convert]
window = "33x29"
```

The picture is an indexed PNG of up to 8 palettes of 16 colours: a pixel's
index divided by 16 is its palette, the remainder its colour, and colour 0
of every palette is transparent. Each 8x8 cell uses one palette. Width and
height are multiples of 8.

| Option | Meaning |
|--------|---------|
| `--window WxH` | the window the worst figures are measured for, in characters: 33x29 (a 256x224 screen at any scroll) by default, 64x32 at most |
| `--no-mirrors` | keep a mirrored cell apart instead of merging it with its character |
| `--priority` | set the priority bit of every cell |
| `-o DIR` / `--out DIR`, `--save`, `--json`, `-q` | as every tool of the family |

### What it writes

| File | What |
|------|------|
| `<stem>.chr` | the characters, 32 bytes each (4 bpp): each cell kept once, a cell equal to a known character mirrored horizontally, vertically or both merged with it; numbered by first appearance, row by row from the **bottom** of the map, left to right, so that the characters of a new row follow each other in ROM when the camera goes up |
| `<stem>.trm` | the map, one 16-bit word a cell, row after row, in parts of at most 32 KB holding whole rows: character in bits 0-11 (0-4095), bit 12 0, priority in bit 13, the flips in bits 14 (horizontal) and 15 (vertical) |
| `<stem>.tpl` | one byte a character: its palette, shifted to the tilemap entry's high byte (palette << 2) |
| `<stem>.trh` | 24 bytes the module reads: sizes, rows per part, the window, the worst figures |
| `<stem>.pal` | the PNG's palette, BGR555 |
| `<stem>.inc`, `<stem>_data.as` | the C declarations (`<stem>_CHARS`, `<stem>_WORST_ROW`… and `<stem>_TILERING_MAP`, the initializer of a `TileRingMap`) and the asset sections the build assembles |

The characters go in parts of 1,024 (32 KB, a LoROM bank); a map is
refused past 4,096 distinct characters (the 12 bits of the map word).

## Inspect

```sh
opensnes-tilering inspect res/world.png
# res/world.png: 160x112 characters, 1433 distinct (1443 without merging mirrors), 45856 bytes;
#   window 33x29: worst 389 in view (at 13, 12), 26 new a row, 23 a column
```

Every position of the window is tried:

- **worst in view**: distinct characters in the window at once, and where.
  The ring must hold that many slots, plus room to keep characters for a
  return;
- **new a row / a column**: the characters of a row (or column) entering the
  window that no other row of the window holds. It is what one step of the
  camera can load at most; `tileRingCost()` adds them up for a speed.

`--json` gives the same figures, with `characters_without_mirrors`,
`palettes`, `character_bytes` and `map_bytes`.

## Refused

A picture that is not indexed, a cell that mixes two palettes (its position
is named), a colour past index 127, a size that is not a multiple of 8, a
map smaller than the window (it fits a tilemap: no ring needed), more than
4,096 distinct characters, a window past 64x32.
