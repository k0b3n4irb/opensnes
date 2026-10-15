# Transitions

**Colour & effects — how a scene *leaves*.** Cutting hard
between screens looks cheap; a transition sells the change. The SNES gives you
two cheap, hardware-cheap ways to do it — dim the whole screen, or dissolve it
into blocks — and this family is those two.

## The ladder

| Example | Developer question |
|---------|--------------------|
| [fading](fading/) | How do I fade the screen in and out? |
| [mosaic](mosaic/) | How do I pixelate / mosaic-dissolve the screen? |

> An iris wipe — an HDMA-shaped window closing to a shrinking circle — is
> `hdmaIrisWipe()`, shown in [`hdma/hdma_helpers`](../hdma/hdma_helpers/).

## The idea in one screen

Both are single-register effects the PPU applies for free every frame. **Fade**
steps the master brightness (`INIDISP`, 0–15) across a few frames — `fadeIn` /
`fadeOut` do the stepping for you. **Mosaic** (`$2106`) grows the pixel-block
size 1→15, so the image dissolves into ever-coarser squares; `mosaicFadeIn`
runs it as a transition. Neither redraws anything — you change one register and
the hardware does the rest, which is why they cost nothing and belong in every
scene change.
