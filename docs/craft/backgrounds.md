# Composing with backgrounds & layers {#craft_backgrounds}

Once @ref craft_planning has you in the right background mode, the craft is
*composition*: deciding what each layer is **for** and how they stack. A SNES
screen is a small stack of transparencies — a few background layers plus the
sprite layer — and the art is assigning each element to the right sheet, at the
right depth. This guide is those decisions; for the priority bits and registers
that implement them, see @ref snes_graphics_guide.

## Give every layer a job

In Mode 1 (your usual choice) you have three background layers and the sprites.
Assign roles before you draw:

| Layer | Typical job |
|-------|-------------|
| **BG1** (16-colour) | the main playfield — the tiles the player interacts with |
| **BG2** (16-colour) | a parallax midground or a second detail layer |
| **BG3** (4-colour) | the HUD/status bar, or a far parallax backdrop |
| **Sprites (OBJ)** | anything that moves independently — player, enemies, pickups, cursor |

The mistake to avoid is treating layers as "more space for tiles." Each layer
is a *depth*, and depth is a design tool: foreground, playfield, midground,
sky. Spend them on separation, not just capacity.

## Depth is priority, not layer order

The powerful, non-obvious part: **you interleave sprites *between* background
layers using priority bits**, so draw order is not fixed by which layer is
which. A single scene can read as:

```
   far backdrop (BG3, low priority)
 < enemies behind a pillar (sprites, low priority)
 < the pillar        (BG2, high priority)
 < the player        (sprites, high priority)
 < the HUD           (BG3 tiles flagged high priority)
```

That is how you get a character walking *behind* a foreground column, or a HUD
that always sits on top, without extra hardware layers — just priority flags on
tiles and sprites. Plan your scene as a set of depths first, then assign each
element a layer + priority that lands it there.

## Three compositions you will reuse

### Parallax — depth from motion
Scroll layers at different speeds and the eye reads distance. A far layer that
creeps, a midground that keeps pace with the camera, a foreground that races
past. Two ways to do it:
- **Per-layer scroll** — simplest: give each BG its own scroll rate. See
  @ref examples_scrolling_mixed_scroll and @ref examples_scrolling_parallax_scroll.
- **Per-scanline scroll (HDMA)** — change a layer's scroll *every line* for
  smooth gradient parallax on a single layer (mountains, water). This is the
  SNES-specific technique the camera literature does not cover.

> **Go deeper.** For camera *movement* itself — deadzones, look-ahead,
> room-locking — read Itay Keren's
> [Scroll Back](https://docs.google.com/document/d/1iNSQIyNpVGHeak6isbP6AHdHD50gs8MNXF1GCf08efg/pub),
> the definitive taxonomy. Compose your parallax layers here; decide how the
> camera *follows the player* there.

### The HUD — always on top, cheaply
Put the status bar on the layer whose priority keeps it above the action —
Mode 1's BG3 is the classic home. Two routes:
- **Tiles with the priority bit set** — a fixed HUD baked into BG3. See
  @ref examples_backgrounds_mode1_bg3_priority.
- **A 9-slice panel** — bordered boxes stamped at runtime (health, dialog).
  See @ref examples_basics_panel_hud.

Keep the HUD out of the scrolling world: it should not move when the camera
does. Give it its own layer (or a fixed region) so gameplay scroll never drags
it.

### Foreground occlusion — walking behind things
Flag a foreground layer (or specific tiles) high-priority so the player sprite
passes *behind* it. Pillars, foliage, doorways, tunnel mouths — cheap depth
that makes a flat tilemap feel three-dimensional. It is the same priority
mechanism as the HUD, aimed the other way.

## When you need more than layers can give

- **Four cheap layers** (menus, board games, busy backdrops): drop to Mode 0 —
  four 4-colour layers. See @ref examples_backgrounds_mode0. You trade colour
  depth for layer count.
- **Per-column motion on one layer** (a rippling flag, heat-haze, water):
  offset-per-tile in Mode 2 gives every column its own scroll from a table.
  See @ref examples_backgrounds_mode2. This does per-*column* what HDMA
  parallax does per-*scanline*.

## The one rule

Design the screen as **depths** — sky, far, playfield, foreground, HUD — then
map each depth to a (layer, priority) pair. Layers are how the hardware
composites; depth is what the player sees. Start from what they see.
