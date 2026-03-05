# Graphics Examples

Learn how the SNES PPU renders backgrounds, displays sprites, and produces visual
effects. These 21 examples cover the full range of the PPU's capabilities.

## Sub-Categories

| Category | Examples | What It Covers |
|----------|----------|----------------|
| [backgrounds/](backgrounds/) | 7 | Background modes (1, 7), scrolling, compression |
| [effects/](effects/) | 9 | Fading, mosaic, windows, HDMA, transparency, color math |
| [sprites/](sprites/) | 5 | Sprite display, animation, metasprites, dynamic VRAM |

## Quick Reference

### Background Modes

| Mode | BG Layers | Colors | Best For |
|------|-----------|--------|----------|
| 0 | 4 x 2bpp | 4 per layer | Text, simple graphics |
| 1 | 2 x 4bpp + 1 x 2bpp | 16/16/4 | Most games |
| 3 | 1 x 8bpp + 1 x 4bpp | 256/16 | Detailed backgrounds |
| 5 | 1 x 4bpp + 1 x 2bpp (hi-res) | 16/4 | High-resolution text |
| 7 | 1 x 8bpp (affine) | 256 | Rotation, scaling |

### Sprite Palette Locations

Sprite palettes occupy CGRAM colors 128-255:

| Palette | CGRAM Index |
|---------|-------------|
| 0 | 128-143 |
| 1 | 144-159 |
| ... | ... |
| 7 | 240-255 |

---

See the sub-category READMEs for detailed documentation on each example.
