# Mode 7

**Family 7 — the SNES's signature trick.** Mode 7 turns one background into a
single texture the hardware can rotate, scale and skew per scanline — the
floor of F-Zero, the map of Super Mario Kart, the world bending under
Pilotwings. It gets its own family because it is a distinct render model with
its own matrix API and its own "I want to do THAT" pull.

## The ladder

| Rung | Example | Developer question |
|------|---------|--------------------|
| 7.1 | [rotate_scale](rotate_scale/) | How do I rotate and scale a background? |
| 7.2 | [perspective](perspective/) | How do I fake perspective with an HDMA matrix split (F-Zero floor)? |
| 7.3 | [perspective_rotate](perspective_rotate/) | How do I drive the *full* matrix (A/B/C/D) per scanline — rotating perspective? |

Climb from a flat spinning plane (7.1) to a receding horizon (7.2) to a
horizon that also rotates (7.3).

## The idea in one screen

A Mode 7 background is one 128×128-tile plane (1024×1024 px) plus a 2×2 affine
matrix (`M7A`–`M7D`) and a centre of rotation. Set the matrix once and the
whole plane rotates/scales (7.1). Rewrite the matrix **every scanline** with
HDMA and each line samples the texture at a different scale, so the plane
appears to recede into the distance (7.2 does the diagonal terms; 7.3 drives
all four for a horizon that rotates too).

The capstone games live in [`games/`](../games/): **mode7_racing** (F-Zero
style) and **mode7_flying** (Pilotwings style) put this whole family to work.

> luna models the Mode 7 PPU natively, so each rung's render is verified by
> the visual-regression baseline — no emulator side channel needed.
