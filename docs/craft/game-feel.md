# Game feel: juice on real hardware {#craft_game_feel}

"Game feel" is the layer of feedback that makes a jump feel springy and a hit
feel like it landed — the same core mechanic reads as limp or delicious
depending on it. The universal principles are covered definitively elsewhere
(see Go-deeper); what nobody covers is *how to produce that juice on the SNES*,
where you have no shader, no tween library, and a 4 KB VBlank budget. Almost
every classic effect turns out to be a cheap trick on a PPU register. This guide
is the SNES-specific toolbox; for the registers behind each, see
@ref snes_graphics_guide.

## Screen shake — nudge the scroll

The highest impact-per-byte effect on the machine: on a big hit, add a small
random offset to every background's scroll for a few frames and decay it to
zero. It is just the camera from @ref craft_camera, perturbed — a couple of
pixels, three or four frames, fading out. Costs nothing, sells everything. Shake
the sprite layer too (offset your OAM Y) for an impact that rattles the whole
screen, or shake only the backgrounds for a softer thud.

## Hit flash — color math, not palette edits

To flash a struck enemy white or tint the screen red on damage, do **not**
rewrite palette entries every frame — that is a DMA you do not need. Use color
math: the PPU adds or subtracts a fixed colour across a layer or the sprites in
one register write. A one-frame white add on hit, a red subtract while the
player is hurt, a blue wash underwater — all are @ref examples_color_shadow_tint
and @ref examples_color_transparency territory, applied for a handful of frames.

## Freeze the frame — hitstop

The cheapest juice of all is *no* hardware: on a heavy hit, stop advancing the
simulation for two to six frames. The image holds, the impact registers, then
motion resumes. It is pure timing — a counter in your update loop — and it is
what makes fighting games and action platformers feel like contact has weight.
Pair it with a one-frame flash and a short shake and a plain collision becomes a
*hit*.

## Punctuate scene changes — fades and dissolves

Cuts feel cheap; transitions feel authored. Two are nearly free on the SNES:

- **Brightness fade** — ramp the screen master brightness (INIDISP) down to
  black and back. The standard scene bookend. See @ref examples_transitions_fading.
- **Mosaic dissolve** — grow the mosaic size to pixelate the screen away, then
  shrink it back on the new scene. A distinctly SNES transition. See
  @ref examples_transitions_mosaic.

Both are a single register animated over a dozen frames, and both read as
production value far above their cost.

## Ambient life — palette cycling

A static scene feels dead; a moving one feels alive. You do not need to animate
tiles to get motion — **rotate a few palette entries** and water shimmers, lava
churns, a portal pulses, with zero tile or DMA cost. It is the cheapest ongoing
motion the hardware offers. See @ref examples_color_palette_cycle. The same
lever gives you a low-health screen pulse or a power-up flash.

## Squash and stretch without scaling

SNES sprites do not scale outside Mode 7, so you fake elasticity the old way:
**pre-draw a few frames** — a squashed sprite for the landing, a stretched one
for the launch, held for a frame or two. It is the animation principle, not a
hardware feature, and it is what separates a character that *thumps* onto the
ground from one that merely stops. See @ref examples_sprites_animated_sprite for
frame-driven animation. For rippling heat, water or a hit-wobble on a whole
layer, per-scanline distortion via HDMA does what sprite scaling cannot — see
@ref examples_hdma_hdma_wave.

## Layer the little things

Juice compounds. A coin pickup that is satisfying is rarely one effect — it is a
flash, a small sound, a number that pops and rises, and maybe a one-frame
freeze, all at once and all gone within a quarter-second. @ref examples_games_shmup_1942
is worth reading for how modest effects stack into a game that feels responsive.

> **Go deeper.** Watch Jonasson & Purho's
> [Juice It or Lose It](https://www.gdcvault.com/play/1016487/Juice-It-or-Lose):
> they take the same barebones game and layer juice onto it live. Every
> technique they show maps to a cheap trick above.

## The one rule

Juice the verbs the player does most — jump, hit, collect, take damage — and
make every effect *brief and decaying*: a few frames, fading to nothing. Feel
comes from the sum of small, short responses to input, not from one big effect.
Add them one at a time and stop when it feels right.

> **Sources:**
> [fullsnes](https://problemkaputt.de/fullsnes.htm) (color math, mosaic, INIDISP
> brightness) and the [SNESdev Wiki](https://snes.nesdev.org/wiki); the SDK's
> color-math and transition examples for the working code.
