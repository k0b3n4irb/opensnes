# The PPU {#hardware_ppu}

The picture processing unit: what it draws, from which memories, in which
formats. This page is the one home of the PPU's tables; the registers that
set them are in @ref registers, and the sprite attribute memory byte by byte
is in @ref oam.

## Two chips

| Chip | Name | Function |
|------|------|----------|
| PPU1 | 5C77 | Addresses VRAM, decides what is drawn |
| PPU2 | 5C78 | Composes the picture, reads the palettes, outputs the video signal |

## Memories

| Memory | Size | Holds |
|--------|------|-------|
| **VRAM** | 64 KB (32K words) | Tiles and tilemaps, for backgrounds and sprites |
| **OAM** | 544 bytes | Attributes of the 128 sprites (@ref oam) |
| **CGRAM** | 512 bytes | 256 colours of 15 bits |

None of them is mapped in the CPU's address space: they are read and written
through ports ($2104, $2118/$2119, $2122), and only during VBlank or force
blank — the PPU ignores a write during active display.

## Resolution

| Mode | Resolution |
|------|------------|
| NTSC | 256x224 or 256x239 |
| PAL | 256x239 or 256x224 (same picture sizes; more blank lines) |
| Hi-res | 512x224 (Modes 5/6), 448/478 lines in interlace |

Hi-res and interlace in practice: @ref tutorial_graphics.

## Background modes {#hardware_ppu_modes}

| Mode | BG1 | BG2 | BG3 | BG4 | Notes |
|------|-----|-----|-----|-----|-------|
| 0 | 2bpp | 2bpp | 2bpp | 2bpp | Four layers, 4 colours each |
| **1** | **4bpp** | **4bpp** | **2bpp** | - | **The common one**: two 16-colour layers and a 4-colour one; BGMODE bit 3 raises BG3's high-priority tiles above everything |
| 2 | 4bpp | 4bpp | - | - | Offset-per-tile (BG3 holds the offset table) |
| 3 | 8bpp | 4bpp | - | - | 256 colours on BG1; direct colour available |
| 4 | 8bpp | 2bpp | - | - | Offset-per-tile; direct colour available |
| 5 | 4bpp | 2bpp | - | - | Hi-res, 512 pixels wide |
| 6 | 4bpp | - | - | - | Hi-res and offset-per-tile |
| 7 | 8bpp | - | - | - | Rotation and scaling; direct colour available |

**Direct colour** (CGWSEL $2130 bit 0) is for the 256-colour layers of
Modes 3, 4 and 7: the tile data is read as a colour (`BBGGGRRR`, completed
by the tile's three palette bits) instead of a CGRAM index. Colour 0 stays
transparent, so there is no black pixel in that mode (anomie's register
doc, *Direct Color Mode*).

The mode is chosen with `setMode()`; @ref craft_planning weighs each mode
against its VRAM cost.

## Tiles

Every graphic is made of 8x8 tiles of indexed colour, stored as bit planes.
A tile's size depends only on its colour depth:

| Depth | Colours | Bytes per 8x8 tile | Typical use |
|-------|---------|--------------------|-------------|
| 2bpp | 4 | 16 | HUD, text, simple backgrounds |
| 4bpp | 16 | 32 | Main backgrounds, all sprites |
| 8bpp | 256 | 64 | One detailed layer (Modes 3, 4, 7) |

Colour 0 of every palette is transparent. A layer addresses 1024 tiles (the
tile number of a tilemap entry has 10 bits). Sprites are always 4bpp.
Conversion from PNG: @ref tools_opensnes_tileset (backgrounds) and
@ref tools_opensnes_sprite (sprite sheets).

## Tilemaps {#hardware_ppu_tilemaps}

Each tilemap entry is **16 bits**:

```
VHOP PPCC CCCC CCCC
│││  ││└─────────── Tile number (0-1023)
│││  └└───────────── Palette (0-7)
││└─────────────────  Priority (0-1)
│└──────────────────  Horizontal flip
└───────────────────  Vertical flip
```

A map is 32x32 entries per screen, and a layer can join two or four screens
(BGnSC bits 0-1):

| Setting | Tiles | Pixels | Memory |
|---------|-------|--------|--------|
| 32x32 | 1024 | 256x256 | 2 KB |
| 64x32 | 2048 | 512x256 | 4 KB |
| 32x64 | 2048 | 256x512 | 4 KB |
| 64x64 | 4096 | 512x512 | 8 KB |

The map wraps at its edges; scrolling over it: @ref tutorial_scrolling.

## VRAM layout {#hardware_ppu_vram}

VRAM is addressed by **16-bit word**: its 64 KB are words $0000-$7FFF. The
layout is free; three registers place each part, in word units:

| Register | Places | Unit |
|----------|--------|------|
| $2107-$210A BGnSC | Each BG's tilemap | $400 words (bits 7-2) |
| $210B-$210C BG12NBA / BG34NBA | Each BG's tiles | $1000 words (one nibble per BG) |
| $2101 OBJSEL | The sprite tiles | $2000 words (bits 2-0), and the gap to the second sprite table (bits 4-3) |

One layout that fits a Mode 1 game:

| Word address | Size | Content |
|--------------|------|---------|
| $0000 | 24 KB | BG tiles (4bpp, 768 tiles) |
| $3000 | 2 KB | BG1 tilemap |
| $3400 | 2 KB | BG2 tilemap |
| $3800 | 2 KB | BG3 tilemap |
| $4000 | 16 KB | Sprite tiles (4bpp, 512 tiles) |

Tiles, maps and sprite tiles share the one pool: plan it before drawing
(@ref craft_planning).

## CGRAM and colours {#hardware_ppu_cgram}

CGRAM holds 256 colours of 2 bytes:

| Colours | Use |
|---------|-----|
| 0-127 | Background palettes: 8 palettes of 16 colours in the 4bpp modes; 2bpp layers read them as 4-colour groups; an 8bpp layer uses all 256 |
| 128-255 | Sprite palettes: 8 palettes of 16 colours; only palettes 4-7 take part in colour math |

A tilemap entry picks a background palette (bits 10-12); a sprite's
attribute byte picks one of the eight sprite palettes, in the upper half.

Colours are **BGR555**, 15 bits:

```
0BBB BBGG GGGR RRRR
 │││││││││││└────── Red (0-31)
 │││││└└└└└──────── Green (0-31)
 └└└└└───────────── Blue (0-31)
```

White is $7FFF, red $001F, green $03E0, blue $7C00. The 32768 colours give
256 on screen at once from CGRAM.

## Sprites {#hardware_ppu_sprites}

The OBJ layer shows up to **128 sprites**, 4bpp, each one of two sizes that
OBJSEL ($2101, bits 7-5) chooses for all sprites at once:

| OBJSEL size | Small | Large |
|-------------|-------|-------|
| 0 | 8x8 | 16x16 |
| 1 | 8x8 | 32x32 |
| 2 | 8x8 | 64x64 |
| 3 | 16x16 | 32x32 |
| 4 | 16x16 | 64x64 |
| 5 | 32x32 | 64x64 |
| 6 | 16x32 | 32x64 |
| 7 | 16x32 | 32x32 |

Sizes 6 and 7 are undocumented: Nintendo's manual reserves them; fullsnes
and the SNESdev wiki describe them. The SDK names sizes 0-5
(`OBJ_SIZE8_L16` … `OBJ_SIZE32_L64`). Each sprite's size bit, position,
tile, palette, priority and flips are in OAM: @ref oam.

A sprite larger than 8x8 reads its tiles from a sheet 16 tiles wide: a
16x16 sprite at tile `n` uses `n`, `n+1`, `n+16`, `n+17` (@ref tutorial_sprites
shows how the converter lays a sheet out for it).

### Per-line limits

| Limit | Value | Past it |
|-------|-------|---------|
| Sprites on one line | 32 | The sprites of higher OAM index are dropped |
| 8-pixel slivers on one line | 34 | Parts of sprites are clipped |

A game with many sprites on a line rotates their OAM order from frame to
frame, so the loss turns into flicker spread over all of them.

### Metasprites

A character made of several hardware sprites is a **metasprite**: a list of
sprites with offsets from one origin. The SDK's form is `MetaspriteItem`
(`<snes/sprite.h>`), one entry per sprite and a terminator:

```c
static const MetaspriteItem hero_frame0[] = {
    METASPR_ITEM(0,  0,  0, OBJ_PRIO(2)),   /* top-left */
    METASPR_ITEM(16, 0,  1, OBJ_PRIO(2)),   /* top-right */
    METASPR_ITEM(0,  16, 2, OBJ_PRIO(2)),   /* bottom-left */
    METASPR_ITEM(16, 16, 3, OBJ_PRIO(2)),   /* bottom-right */
    METASPR_TERM
};
```

The tile field is in 8x8 name units from the character's base tile.
`oamDrawMetasprite()` draws one; @ref tools_opensnes_sprite writes them
from a sheet, and @ref tutorial_animation plays them as clips.

## Priorities

- **Between sprites:** the lower OAM index is drawn in front, whatever the
  priority bits (with priority rotation off, the library's setting).
- **Sprites against backgrounds:** a sprite's two priority bits (0-3) place
  it among the background layers, whose tiles carry one priority bit each.
- **Mode 1:** BGMODE bit 3 puts BG3's high-priority tiles in front of
  everything — the usual HUD.

## Further reading

| Resource | Description |
|----------|-------------|
| [SNESdev Wiki - Sprites](https://snes.nesdev.org/wiki/Sprites) | Sprite reference |
| [Fullsnes](https://problemkaputt.de/fullsnes.htm) | The complete technical reference |
| [Super Famicom Wiki](https://wiki.superfamicom.org) | Hardware documentation |
| [nesdoug tutorials](https://nesdoug.com) | C/ASM tutorials with examples |
| [Fabien Sanglard, SNES PPU](https://fabiensanglard.net/snes_ppus_how/) | PPU internals |
