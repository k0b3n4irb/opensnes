# Colour

**Part of Family 6 (Colour & effects) — squeezing more colour out of the
PPU.** The SNES shows 256 colours at once from a 15-bit (32768-colour) palette
— unless you cheat. This family is the colour-math blend, the palette-bypass
mode, and the tricks that push past 256 by reloading the palette mid-frame.

## The ladder

| Rung | Example | Developer question |
|------|---------|--------------------|
| 6b   | [palette_cycle](palette_cycle/) | How do I animate (waterfall/fire/lights) by rotating CGRAM, no pixels moved? |
| 6c.1 | [transparency](transparency/) | How do I blend two layers with colour math? |
| 6c.2 | [shadow_tint](shadow_tint/) | How do I darken (night) or colour-cast (underwater/sunset) a whole scene? |
| 6c.3 | [direct_color](direct_color/) | How do I use direct-colour mode (the pixel byte *is* the RGB)? |
| 6d.1 | [gradient_9bit](gradient_9bit/) | How do I fake a "9-bit" gradient with brightness dithering? |
| 6d.2 | [hicolor_1792](hicolor_1792/) | How do I show 1792 colours via H-IRQ CGRAM streaming? |
| 6d.3 | [hicolor_blend](hicolor_blend/) | How do I show 3840 colours via an RGB channel-split blend? |

## The idea in one screen

The PPU reads pixel colours from CGRAM (256 slots). **Colour math** ($2130–
$2132) adds or subtracts one layer's colour over another — transparency,
shadows, tints. **Direct colour** spends the 8bpp pixel byte *as* a BGR value,
skipping CGRAM entirely. And to beat the 256-at-once limit, you **reload CGRAM
mid-frame** under an H-IRQ so different scanlines see different palettes
(`hicolor_1792`), or split the RGB channels across two blended layers
(`hicolor_blend`, 3840 colours).
