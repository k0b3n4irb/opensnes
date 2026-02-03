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

### [3. Mode 7](3_mode7/)
**Difficulty:** ⭐⭐⭐⭐ Advanced

Mode 7 rotation and scaling effects.

**Concepts:**
- Mode 7 matrix transformation
- Rotation and scaling math
- HDMA for per-scanline effects

---

### [4. Scrolling](4_scrolling/)
**Difficulty:** ⭐⭐ Beginner+

Basic background scrolling with D-pad control.

**Concepts:**
- Scroll registers (BGxHOFS, BGxVOFS)
- Large tilemaps for scrolling
- Camera movement

---

### [5. Mode 1](5_mode1/)
**Difficulty:** ⭐⭐ Beginner+

Mode 1 with multiple background layers.

**Concepts:**
- Mode 1 setup (2 4bpp layers + 1 2bpp layer)
- Layer priorities
- Tilemap setup

---

### [6. Simple Sprite](6_simple_sprite/)
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

### [7. Fading](7_fading/)
**Difficulty:** ⭐⭐ Beginner+

Screen brightness fading effects.

**Concepts:**
- INIDISP register ($2100)
- Brightness levels (0-15)
- Smooth fade transitions

---

### [8. Mode 0](8_mode0/)
**Difficulty:** ⭐⭐ Beginner+

Mode 0 with 4 background layers.

**Concepts:**
- Mode 0 (4 × 2bpp backgrounds)
- 4 palettes per layer
- Layer setup

---

### [9. Parallax](9_parallax/)
**Difficulty:** ⭐⭐⭐ Intermediate

Multi-layer parallax scrolling.

**Concepts:**
- Different scroll speeds per layer
- Depth illusion
- HDMA-based per-scanline scroll

---

### [10. Mode 3](10_mode3/)
**Difficulty:** ⭐⭐⭐ Intermediate

Mode 3 with 256-color direct color.

**Concepts:**
- Mode 3 (1 × 8bpp + 1 × 4bpp)
- Direct color mode
- Large tile graphics

---

### [11. Mode 5](11_mode5/)
**Difficulty:** ⭐⭐⭐ Intermediate

Mode 5 hi-res mode (512×224).

**Concepts:**
- Interlaced/hi-res display
- Mode 5 setup
- 16×8 tiles

---

### [12. Continuous Scroll](12_continuous_scroll/)
**Difficulty:** ⭐⭐⭐ Intermediate

Seamless infinite scrolling with dynamic tile loading.

**Concepts:**
- Tilemap streaming
- Edge detection for loading
- Wrap-around handling

**Important:** This example documents the struct coordinate pattern.

---

### [13. Animation System](13_animation_system/)
**Difficulty:** ⭐⭐⭐ Intermediate

Flexible sprite animation framework.

**Concepts:**
- Animation state machine
- Frame timing
- Direction handling

---

### [14. Metasprite](14_metasprite/)
**Difficulty:** ⭐⭐⭐ Intermediate

Multi-tile sprites (metasprites).

**Concepts:**
- Combining multiple hardware sprites
- Metasprite data format
- Flip support for metasprites

---

### [15. Dynamic Sprite](15_dynamic_sprite/)
**Difficulty:** ⭐⭐⭐⭐ Advanced

Dynamic sprite engine with VRAM management.

**Concepts:**
- On-demand VRAM loading
- Sprite slot management
- oambuffer system

---

### [16. HDMA Gradient](16_hdma_gradient/)
**Difficulty:** ⭐⭐⭐ Intermediate

HDMA-based color gradient effects.

**Concepts:**
- HDMA setup and tables
- Fixed color register ($2132)
- Sky/water gradients

---

### [17. Transparency](17_transparency/)
**Difficulty:** ⭐⭐⭐ Intermediate

Color math and transparency effects.

**Concepts:**
- Color addition/subtraction
- Half-transparent sprites
- Window-based masking

---

### [18. Window](18_window/)
**Difficulty:** ⭐⭐⭐ Intermediate

Hardware window effects.

**Concepts:**
- Window 1 and Window 2
- Window logic (AND, OR, XOR)
- Per-layer window enable

---

### [19. Mosaic](19_mosaic/)
**Difficulty:** ⭐⭐ Beginner+

Mosaic pixelation effect.

**Concepts:**
- Mosaic register ($2106)
- Per-BG mosaic enable
- Mosaic size (1-16)

---

### [20. HDMA Wave](20_hdma_wave/)
**Difficulty:** ⭐⭐⭐⭐ Advanced

Wavy distortion effects using HDMA.

**Concepts:**
- Per-scanline scroll modification
- Sine wave tables
- Water/heat shimmer effects

---

### [21. Animated Sprite](21_animated_sprite/)
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

**Next Topic:** [Input](../input/) → Handle controller input
