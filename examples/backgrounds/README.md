# Backgrounds

**Family 2 — "fill the screen with your own art."** The background layers are
the SNES's canvas. This family is a ladder through the PPU's background modes,
from a single Mode 1 image up to 256-colour and hi-res, each mode a different
trade of colours against layers against resolution.

## The ladder

| Rung | Example | Developer question |
|------|---------|--------------------|
| 2.1 | [mode1](mode1/) | How do I show a background from a PNG (the canonical Mode 1)? |
| 2.3 | [mode1_lz77](mode1_lz77/) | How do I load a background from *compressed* data? |
| 2.4 | [mode1_bg3_priority](mode1_bg3_priority/) | How do I make BG3 draw in front for a HUD overlay? |
| 2.5 | [mode0](mode0/) | How do I run four 2bpp layers (Mode 0)? |
| 2.6 | [mode3](mode3/) | How do I show a 256-colour (8bpp, Mode 3) background? |
| 2.7 | [mode5](mode5/) · [mode5_hires](mode5_hires/) | How do I use a hi-res (512-wide, Mode 5 + interlace) background? |
| 2.8 | [mode2](mode2/) | How do I scroll each column independently (offset-per-tile, modes 2/4/6)? |

> Under-the-hood rung to come — **2.2 a tilemap built by hand in C**: what the
> asset macro hides, for the `fundamentals/` tier.
>
> Moving the world past the camera lives in its own family: [`scrolling/`](../scrolling/).

## The idea in one screen

A background is *tiles* (8×8 pixel art in VRAM) placed by a *tilemap* (a grid
of tile indices) and coloured by a *palette*. A **mode** picks how many layers
you get and how many colours each: Mode 1 gives two 4bpp layers + one 2bpp
(the workhorse); Mode 0 gives four 2bpp layers; Mode 3 spends everything on
one 8bpp (256-colour) layer; Mode 5 trades colour for 512-pixel width. Same
three ingredients, six trade-offs — pick the mode your scene needs.
