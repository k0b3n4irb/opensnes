---
name: gfx4snes 2bpp quantization quirk + PIL pre-processing pattern
description: gfx4snes does naive quantization that loses minority colors when 90%+ of pixels share one color. Hard-to-derive workflow — always pre-process via PIL with explicit palette before gfx4snes for 2bpp targets.
type: project
originSessionId: 7cf8aa6f-20b3-4e2f-81b4-25748e91b08f
---
`gfx4snes` (the OpenSNES PNG/BMP → SNES tile converter) uses a naive
4-color quantizer for 2bpp outputs. When the input image is dominated by
one color (>= 90 % of pixels — typical SNES backgrounds with a flat blue
sky and a few white lines), the quantizer picks 4 near-identical
variants of the dominant color and **silently loses the minority
colors** (the white lines disappear or become indistinguishable from the
background).

**Why**: the quantizer is histogram-based; minority pixels don't have
enough mass to claim a palette slot.

**How to apply**: when porting any PVSnesLib 2bpp asset OR converting a
new asset for Mode 0 / 2bpp BG, pre-process the PNG with PIL forcing the
desired 4-color palette **before** gfx4snes runs. Pattern:

```python
from PIL import Image
img = Image.open('input.png').convert('RGB')
palette = [R0,G0,B0, R1,G1,B1, R2,G2,B2, R3,G3,B3]
palette.extend([0] * (768 - len(palette)))
pal_img = Image.new('P', (1, 1))
pal_img.putpalette(palette)
result = img.quantize(colors=4, palette=pal_img, dither=0)
result.save('output.png')
```

Then run `gfx4snes -s 8 -m -p -u 4 -o 4 -i output.png` on the PIL-quantised
image.

**Coupled invariant**: tiles and palette are inseparable. Changing the
palette without regenerating tiles produces ugly banding (the tile data
references palette indices that no longer point to the right colors).
Always re-run the full conversion when the palette changes.

**incbin rebuild gotcha**: changing `.pic` / `.pal` / `.map` does NOT
trigger a Makefile rebuild — `make` doesn't track `.incbin` dependencies.
After regenerating assets, run `make clean && make`.

## When this rule does NOT apply

- 4bpp targets (Mode 1 BG1/BG2). 16 colors give the quantizer enough
  headroom; minority colors survive.
- Sprite assets (always 4bpp on SNES). Same reason.
- Inputs that are already palette-correct (PIL pre-processed elsewhere
  or hand-painted with the exact palette).

## Cross-references

- The PVSnesLib porting workflow (`memory/pvsneslib_porting.md`) — the
  "compare binary assets EARLY" step often surfaces this as the cause
  when ported visuals diverge.
- `tools/gfx4snes/` (the C tool, separate from `devtools/font2snes/`).
