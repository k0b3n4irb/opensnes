# Background Examples

Learn SNES background modes and scrolling techniques.

## Examples

| Example | Complexity | Description |
|---------|------------|-------------|
| [1_mode0](1_mode0/) | 1 | Mode 0: 4 background layers, 2bpp (4 colors each) |
| [1_mode1](1_mode1/) | 1 | Mode 1: 2×4bpp + 1×2bpp, most common mode |
| [2_scrolling](2_scrolling/) | 2 | Basic background scrolling with D-pad |
| [3_mode3](3_mode3/) | 3 | Mode 3: 256-color direct mode |
| [3_mode5](3_mode5/) | 3 | Mode 5: Hi-res 512×224 mode |
| [3_parallax](3_parallax/) | 3 | Multi-layer parallax scrolling |
| [4_mode7](4_mode7/) | 4 | Mode 7: Rotation and scaling |
| [4_continuous_scroll](4_continuous_scroll/) | 4 | Dynamic tile loading for large maps |

## Key Concepts

### BG Scroll Registers
- `$210D-$210E`: BG1 H/V scroll
- `$210F-$2110`: BG2 H/V scroll
- `$2111-$2112`: BG3 H/V scroll
- `$2113-$2114`: BG4 H/V scroll

### Tilemap Structure
Each tilemap entry is 2 bytes:
```
Bit 15-10: Palette
Bit 9: Priority
Bit 8: Y flip
Bit 7: X flip
Bit 0-9: Tile number
```
