# Visual Effects Examples

Learn SNES special effects: fading, windows, transparency, and HDMA.

## Examples

| Example | Complexity | Description |
|---------|------------|-------------|
| [fading](fading/) | 1 | Screen brightness fading |
| [mosaic](mosaic/) | 2 | Pixelation mosaic effect |
| [window](window/) | 2 | Hardware window masking |
| [transparency](transparency/) | 3 | Color math (add/subtract) |
| [hdma_gradient](hdma_gradient/) | 3 | HDMA color gradients |
| [hdma_wave](hdma_wave/) | 4 | HDMA wave distortion |

## Key Concepts

### Screen Brightness (INIDISP $2100)
- Bits 0-3: Brightness (0=black, 15=full)
- Bit 7: Force blank

### Mosaic ($2106)
- Bits 0-3: Enable per BG
- Bits 4-7: Mosaic size (0=1×1, 15=16×16)

### Windows ($2123-$212B)
- Window 1 left/right: $2126/$2127
- Window 2 left/right: $2128/$2129
- Logic: AND, OR, XOR, XNOR

### HDMA
Horizontal DMA runs once per scanline:
- Channels 6-7 recommended (avoid DMA conflicts)
- Table format: count + data bytes
- Repeat mode (bit 7=1) for scroll registers
