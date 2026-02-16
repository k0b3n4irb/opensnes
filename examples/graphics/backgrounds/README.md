# Background Examples

Learn SNES background modes and scrolling techniques.

## Examples

| Example | Complexity | Description |
|---------|------------|-------------|
| [mode0](mode0/) | 1 | Mode 0: 4 background layers, 2bpp (4 colors each) |
| [mode1](mode1/) | 1 | Mode 1: 2×4bpp + 1×2bpp, most common mode |
| [scrolling](scrolling/) | 2 | Basic background scrolling with D-pad |
| [mode3](mode3/) | 3 | Mode 3: 256-color direct mode |
| [mode5](mode5/) | 3 | Mode 5: Hi-res 512×224 mode |
| [parallax](parallax/) | 3 | Multi-layer parallax scrolling |
| [mode7](mode7/) | 4 | Mode 7: Rotation and scaling |
| [continuous_scroll](continuous_scroll/) | 4 | Dynamic tile loading for large maps |

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
