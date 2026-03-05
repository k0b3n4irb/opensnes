# Visual Effects Examples

Learn SNES special effects: brightness fading, mosaic pixelation, hardware windows,
color math transparency, and HDMA-driven per-scanline effects. These techniques
give SNES games their distinctive visual style.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [fading](fading/) | Beginner | Screen brightness fading for transitions |
| [mosaic](mosaic/) | Beginner | Pixelation mosaic effect on backgrounds |
| [window](window/) | Intermediate | Hardware window masking (rectangular regions) |
| [transparency](transparency/) | Intermediate | Color math add/subtract blending between layers |
| [transparent_window](transparent_window/) | Intermediate | Combining color math with windowed regions |
| [gradient_colors](gradient_colors/) | Intermediate | HDMA-driven CGRAM color gradients per scanline |
| [hdma_gradient](hdma_gradient/) | Intermediate | HDMA brightness/color gradient effect |
| [parallax_scrolling](parallax_scrolling/) | Advanced | HDMA-driven multi-rate parallax scrolling |
| [hdma_wave](hdma_wave/) | Advanced | HDMA wave distortion (wavy scanline offsets) |

## Key Concepts

### Screen Brightness (INIDISP $2100)

- Bits 0-3: Brightness level (0 = black, 15 = full)
- Bit 7: Force blank (allows VRAM access outside VBlank)

### Mosaic ($2106)

- Bits 0-3: Enable per BG layer
- Bits 4-7: Mosaic block size (0 = 1x1, 15 = 16x16)

### Hardware Windows ($2123-$212B)

- Window 1 left/right: `$2126` / `$2127`
- Window 2 left/right: `$2128` / `$2129`
- Logic operations: AND, OR, XOR, XNOR per layer

### HDMA (Horizontal Blank DMA)

HDMA transfers data to PPU registers once per scanline during HBlank:
- Use channels 6-7 to avoid conflicts with general DMA
- Table format: count byte + data bytes per entry
- Repeat mode (bit 7 of count = 1) writes every scanline in the group

### Color Math ($2130-$2132)

- Add or subtract colors between main screen and sub screen (or fixed color)
- Half-color mode divides the result by 2 for smoother blending

---

Start with **fading** and **mosaic** for simple register writes, then explore
**hdma_wave** and **parallax_scrolling** for per-scanline HDMA techniques.
