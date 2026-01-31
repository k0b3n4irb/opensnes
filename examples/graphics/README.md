# Topic: Graphics

Learn how to display sprites, animate them, and create scrolling backgrounds.

## What You'll Learn

- OAM (Object Attribute Memory) for sprites
- 4bpp tile format (16 colors)
- Sprite sizes and priorities
- Background scrolling
- Using the gfx4snes tool

## SNES Graphics Architecture

The SNES PPU (Picture Processing Unit) combines multiple layers:

```
        ┌─────────────────────────────────────┐
        │           Final Screen              │
        ├─────────────────────────────────────┤
        │  ┌─────────┐   Sprite Layer         │
        │  │ Sprites │◀── OAM (128 sprites)   │
        │  └────┬────┘                        │
        │       │ priority sorting            │
        │  ┌────▼────┐                        │
        │  │   BG1   │◀── Tilemap + Tiles     │
        │  ├─────────┤                        │
        │  │   BG2   │◀── Tilemap + Tiles     │
        │  ├─────────┤                        │
        │  │  BG3/4  │◀── (Mode dependent)    │
        │  └─────────┘                        │
        │       ▲                             │
        │  Backdrop color                     │
        └─────────────────────────────────────┘
```

## Sprites (Objects)

Sprites are independent graphics that can move freely over backgrounds.

### OAM Structure

OAM holds data for 128 sprites:

```
Low Table (512 bytes) - 4 bytes per sprite:
┌──────────┬──────────┬──────────┬──────────┐
│  Byte 0  │  Byte 1  │  Byte 2  │  Byte 3  │
│ X pos    │ Y pos    │ Tile #   │ Attrib   │
│ (low 8)  │ (0-255)  │ (0-255)  │ vhppccct │
└──────────┴──────────┴──────────┴──────────┘

Attributes byte:
  v = Vertical flip
  h = Horizontal flip
  pp = Priority (0-3)
  ccc = Palette (0-7)
  t = Tile high bit (for tiles 256-511)

High Table (32 bytes) - 2 bits per sprite:
  Bit 0 = X position bit 8 (for X > 255)
  Bit 1 = Size select (0=small, 1=large)
```

### Sprite Sizes

Set via register `$2101` (OBJSEL):

| Value | Small | Large |
|-------|-------|-------|
| 0 | 8×8 | 16×16 |
| 1 | 8×8 | 32×32 |
| 2 | 8×8 | 64×64 |
| 3 | 16×16 | 32×32 |
| 4 | 16×16 | 64×64 |
| 5 | 32×32 | 64×64 |

### 16×16 Sprite Tile Layout

A 16×16 sprite uses 4 tiles arranged in VRAM:

```
In VRAM (128 pixels wide):     On Screen:
┌────┬────┬────┬...           ┌────┬────┐
│ T0 │ T1 │    │              │ T0 │ T1 │
├────┼────┼────┤      ──▶     ├────┼────┤
│T16 │T17 │    │              │T16 │T17 │
└────┴────┴────┘              └────┴────┘
(16 tiles per row)
```

## 4BPP Tile Format

"4bpp" means **4 bits per pixel** = 16 colors.

Each 8×8 tile = 32 bytes:
- Bytes 0-15: Bitplanes 0 and 1 (interleaved)
- Bytes 16-31: Bitplanes 2 and 3 (interleaved)

```
Byte layout:
Row 0: bp0, bp1    Row 0: bp2, bp3
Row 1: bp0, bp1    Row 1: bp2, bp3
...                ...
Row 7: bp0, bp1    Row 7: bp2, bp3
└──────────────┘   └──────────────┘
   16 bytes           16 bytes
```

---

## Lessons

### [1. Sprite](1_sprite/)
**Difficulty:** ⭐⭐ Beginner+

Display a sprite and move it with the D-pad.

**Concepts:**
- OAM buffer management
- Loading tiles to VRAM
- Loading palettes to CGRAM
- Reading controller input
- The gfx4snes tool

**Key Registers:**
- `$2101` OBJSEL - Sprite size/base
- `$2102-2104` OAM access
- `$4218-4219` Joypad data

---

### [2. Animation](2_animation/)
**Difficulty:** ⭐⭐⭐ Intermediate

Animate a sprite with multiple frames.

**Concepts:**
- Frame timing (VBlank-based)
- Tile swapping vs data uploading
- Animation state machine
- Horizontal sprite flipping

**Key Techniques:**
- Timer countdown for frame changes
- Sprite sheets (multiple frames in VRAM)
- Attribute bit 6 for H-flip

---

### 3. Background *(coming soon)*
**Difficulty:** ⭐⭐⭐ Intermediate

Create a scrolling background.

**Concepts:**
- Mode 1 setup
- Large tilemaps
- Scroll registers

---

## Quick Reference

### Sprite Palette Locations

Sprite palettes are in CGRAM colors 128-255:

| Palette | CGRAM Index |
|---------|-------------|
| 0 | 128-143 |
| 1 | 144-159 |
| 2 | 160-175 |
| ... | ... |
| 7 | 240-255 |

### Priority Stacking

Higher priority = drawn on top:

```
Priority 3: ████████  (frontmost)
Priority 2: ████████
Priority 1: ████████
Priority 0: ████████  (behind BGs)
```

### Hide a Sprite

Set Y position to 240 (below visible screen):
```c
oam_lo[sprite_id * 4 + 1] = 240;
```

---

**Previous Topic:** [Text](../text/) ← Display text on screen

**Next Topic:** Input *(coming soon)* → Handle controller input
