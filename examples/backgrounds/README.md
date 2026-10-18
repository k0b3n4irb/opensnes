# Backgrounds

**"fill the screen with your own art."** The background layers are
the SNES's canvas. This family is a ladder through the PPU's background modes,
from a single Mode 1 image up to 256-colour and hi-res, each mode a different
trade of colours against layers against resolution.

## The ladder

| Example | Developer question |
|---------|--------------------|
| [mode1](mode1/) | How do I show a background from a PNG (the canonical Mode 1)? |
| [mode1_lz77](mode1_lz77/) | How do I load a background from *compressed* data? |
| [mode1_bg3_priority](mode1_bg3_priority/) | How do I make BG3 draw in front for a HUD overlay? |
| [mode0](mode0/) | How do I run four 2bpp layers (Mode 0)? |
| [mode3](mode3/) | How do I show a 256-colour (8bpp, Mode 3) background? |
| [mode5_hires](mode5_hires/) | How do I use a hi-res (512-wide, Mode 5 + interlace) background? |
| [mode2](mode2/) | How do I scroll each column independently (offset-per-tile, modes 2/4/6)? |
| [mode4](mode4/) | How does offset-per-tile work with a 256-colour layer (Mode 4's single row)? |
| [mode6](mode6/) | How does offset-per-tile work in hi-res (Mode 6)? |

> Moving the world past the camera lives in its own family: [`scrolling/`](../scrolling/).

## The idea in one screen

A background is *tiles* (8×8 pixel art in VRAM) placed by a *tilemap* (a grid
of tile indices) and coloured by a *palette*. A **mode** picks how many layers
you get and how many colours each: Mode 1 gives two 4bpp layers + one 2bpp
(the workhorse); Mode 0 gives four 2bpp layers; Mode 3 spends everything on
one 8bpp (256-colour) layer; Mode 5 trades colour for 512-pixel width. Same
three ingredients, six trade-offs — pick the mode your scene needs.
