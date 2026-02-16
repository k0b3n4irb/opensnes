# Graphics Examples

Learn how to display sprites, animate them, and create scrolling backgrounds with visual effects.

## Sub-Categories

| Category | Description | Examples |
|----------|-------------|----------|
| [backgrounds/](backgrounds/) | Background modes and scrolling | Mode 0-7, parallax, continuous scroll |
| [sprites/](sprites/) | Sprite handling | Simple, animated, metasprites, dynamic |
| [effects/](effects/) | Visual effects | Fading, mosaic, windows, HDMA, transparency |

## Quick Reference

### Background Modes

| Mode | BG Layers | Colors | Best For |
|------|-----------|--------|----------|
| 0 | 4 × 2bpp | 4 per layer | Text, simple graphics |
| 1 | 2 × 4bpp + 1 × 2bpp | 16/16/4 | Most games |
| 3 | 1 × 8bpp + 1 × 4bpp | 256/16 | Detailed backgrounds |
| 5 | 1 × 4bpp + 1 × 2bpp (hi-res) | 16/4 | High-resolution text |
| 7 | 1 × 8bpp (affine) | 256 | Rotation, scaling |

### Sprite Palette Locations

Sprite palettes are in CGRAM colors 128-255:

| Palette | CGRAM Index |
|---------|-------------|
| 0 | 128-143 |
| 1 | 144-159 |
| ... | ... |
| 7 | 240-255 |

---

See the sub-category READMEs for detailed documentation on each example.
