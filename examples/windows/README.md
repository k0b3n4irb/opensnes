# Windows

**Part of Family 6 (Colour & effects) — masking part of the screen.** The PPU
has two hardware *windows*: rectangular (or, driven per scanline, any-shaped)
regions that gate what each layer and colour-math shows. Spotlights, portholes,
iris wipes, "only darken inside this shape" — all windows.

> Not the same as `panel` (tile boxes) — these are the PPU's masking registers.

## The ladder

| Rung | Example | Developer question |
|------|---------|--------------------|
| 6f.1 | [window](window/) | How do I mask part of the screen with one window? |
| 6f.2 | [window_multi_hdma](window_multi_hdma/) | How do I shape *both* windows per scanline? |
| 6f.3 | [transparent_window](transparent_window/) | How do I combine a window with colour math (a darkened region)? |

Climb from one static rectangle (6f.1) to two HDMA-shaped windows (6f.2) to a
window gating colour math (6f.3).

## The idea in one screen

Each window is a left/right edge pair (`WH0`/`WH1`, `$2126`–`$2129`); a layer
or colour math reads a per-layer bit that says "apply me inside / outside the
window." Set the edges once and you get a static rectangle. Rewrite the edges
**every scanline** with HDMA and the two straight edges trace any curve — a
circle, a wave, a shrinking iris. Point colour math at the window instead of a
layer and you darken/tint only the shape.
