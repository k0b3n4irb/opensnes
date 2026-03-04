# Background Examples

Learn SNES background modes and scrolling techniques. The PPU supports multiple
background layers with different color depths, and hardware scrolling registers
allow smooth movement without redrawing the screen.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [mode1](mode1/) | Beginner | Mode 1 basics: 2x4bpp + 1x2bpp, the most common SNES mode |
| [mode1_bg3_priority](mode1_bg3_priority/) | Beginner | Using the BG3 priority bit in Mode 1 |
| [mode1_lz77](mode1_lz77/) | Intermediate | Loading LZ77-compressed background tile and map data |
| [mixed_scroll](mixed_scroll/) | Intermediate | Multiple BG layers scrolling at different speeds |
| [continuous_scroll](continuous_scroll/) | Advanced | Dynamic tile loading for large scrolling maps |
| [mode7](mode7/) | Advanced | Mode 7 rotation and scaling (affine transform) |
| [mode7_perspective](mode7_perspective/) | Advanced | Pseudo-3D perspective effect (F-Zero style) |

## Key Concepts

### BG Scroll Registers

| Register | Purpose |
|----------|---------|
| `$210D-$210E` | BG1 H/V scroll |
| `$210F-$2110` | BG2 H/V scroll |
| `$2111-$2112` | BG3 H/V scroll |
| `$2113-$2114` | BG4 H/V scroll |

### Tilemap Entry (2 bytes)

```
Bit 15-10: Palette
Bit 9:     Priority
Bit 8:     Y flip
Bit 7:     X flip
Bit 0-9:   Tile number
```

### Continuous Scroll Strategy

For maps larger than one screen, stream new tile columns/rows into VRAM as the
camera moves. Only one column or row needs updating per frame, staying well within
the VBlank DMA budget.

---

Start with **mode1** to understand background layers, then progress to
**continuous_scroll** for real-world scrolling techniques.
