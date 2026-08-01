# game_skeleton — the smallest complete game

**Family 5 (Game math & framework) · a capstone.**

## Why it matters for your game

Every example so far teaches one system. This one teaches the *shape that
holds them together*: a title screen, a play loop with a goal and a clock,
and a game-over screen that loops back. It's small enough to read in one
sitting and fork as the starting point for your own game.

![Screenshot](game_skeleton.png)

## What you'll learn

The single idea: **a game is a state machine around one frame loop.** Every
frame you `WaitForVBlank()`, read the pad, then `switch (game_state)` to the
code for the screen you're on. A transition is just an assignment to
`game_state` plus a one-time setup for the new screen:

```
TITLE  --START-->  PLAY  --timer hits 0-->  OVER  --START-->  TITLE
```

It recombines rungs you've climbed: **input** drives the arrow sprite,
a second **sprite** is the coin, a **bounding-box** test scores it, **text**
is the HUD, and `rand()`/`srand()` scatter the coin. Motion and collision are
deliberately trivial — the lesson is the *structure*, not the mechanics.

When the enum + switch starts to sprawl in a bigger game, the opt-in `scene`
framework is the next step up — see [scene_stack](../scene_stack/).

## What to observe / if it breaks

- **Correct run:** title shows `PRESS START`; in play the D-pad moves the
  arrow, touching the coin bumps `SCORE` and respawns it, `TIME` counts down;
  at zero the game-over screen shows the final score; START plays again.
- **Nothing happens on START:** input is read with `padPressed` (edge) — a
  held button fires once; that's intended for menu transitions.
- **Sprites invisible in play:** `setMainScreen(LAYER_BG1 | LAYER_OBJ)` must
  enable OBJ, and colour 1 of an OBJ palette (CGRAM 128+) must be non-black.
- **Number smears (`10` → `9` leaves `90`):** clear the field before
  reprinting — this example prints spaces over the old digits first.

Probe oracles: `game_state` (0 title / 1 play / 2 over), `score`, `time_left`.

## Build & run

```bash
make
../../../tools/luna-test/bin/luna game_skeleton.sfc   # luna-gui: tap START, steer with the D-pad
```

## Modules used

`console`, `dma`, `background`, `sprite`, `text`, `input`

## Where you are

The capstone of the basics family — it ties input, sprites, text and a state
machine into a whole game. For the same structure via the `scene` framework,
see [scene_stack](../scene_stack/); for a HUD box, [panel_hud](../panel_hud/).
