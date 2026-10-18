# Mode 7

**the SNES's signature trick.** Mode 7 turns one background into a
single texture the hardware can rotate, scale and skew per scanline — the
floor of F-Zero, the map of Super Mario Kart, the world bending under
Pilotwings. It gets its own family because it is a distinct render model with
its own matrix API and its own "I want to do THAT" pull.

## The ladder

| Example | Developer question |
|---------|--------------------|
| [rotate_scale](rotate_scale/) | How do I rotate and scale a background? |
| [perspective](perspective/) | How do I fake perspective with an HDMA matrix split (F-Zero floor)? |
| [perspective_rotate](perspective_rotate/) | How do I drive the *full* matrix (A/B/C/D) per scanline — rotating perspective? |
| [extbg](extbg/) | How do I split the Mode 7 plane into two layers with a sprite between them (EXTBG)? |
| [dsp1_ground](dsp1_ground/) | How did Super Mario Kart do it? The DSP-1 streams the per-scanline matrices for a real camera. |

Climb from a flat spinning plane (`rotate_scale`) to a receding horizon
(`perspective`) to a horizon that also rotates (`perspective_rotate`), then
let the coprocessor compute the camera for you (`dsp1_ground`); `extbg`
splits the plane in two layers.

## The idea in one screen

A Mode 7 background is one 128×128-tile plane (1024×1024 px) plus a 2×2 affine
matrix (`M7A`–`M7D`) and a centre of rotation. Set the matrix once and the
whole plane rotates/scales (`rotate_scale`). Rewrite the matrix **every scanline** with
HDMA and each line samples the texture at a different scale, so the plane
appears to recede into the distance (`perspective` does the diagonal terms;
`perspective_rotate` drives all four for a horizon that rotates too).

The capstone games live in [`games/`](../games/): **mode7_racing** (F-Zero
style) and **mode7_flying** (Pilotwings style) put this whole family to work.

> luna models the Mode 7 PPU natively, so each example's render is verified by
> the visual-regression baseline — no emulator side channel needed.
