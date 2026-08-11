# aseprite2snes — Aseprite animation export → OpenSNES AnimClip tables

Aseprite is where sprite artists actually author animation: frames on a
timeline, grouped into named **tags** (`walk`, `idle`, `hurt`), each with a
playback **direction** and a per-frame **duration** in milliseconds. None of
that metadata survived the trip to the SNES — `gfx4snes` turns a spritesheet
into tiles and (with `-P`) into metasprite geometry, but it has no concept of a
tag or a duration. The timeline was retyped by hand into `DECLARE_ANIM_CLIP()`
calls, frame by frame.

aseprite2snes closes that gap. It reads the JSON Aseprite writes with
`--data --list-tags` and emits a C header of ready-to-use `AnimClip` tables for
the OpenSNES `anim.h` player — **one clip per tag**, frame indices and per-frame
tick durations included, playback direction folded into the frame order,
looping vs one-shot derived from the tag.

It **pairs with `gfx4snes -P`**: gfx4snes owns the pixels and the metasprite
pointer table; aseprite2snes owns the timeline that indexes it. v1 is the
animation layer only — it never touches image data.

## Usage

```sh
aseprite2snes [options] <export.json>

  -o FILE    write the header to FILE (default: stdout)
  -p PREFIX  symbol/macro prefix (default: input basename)
  -t STRIDE  frame value = frame index × STRIDE (default: 1)
  -f FPS     frame rate for ms→tick conversion (default: 60)
  -h         help
  -V         version
```

The typical three-step sprite pipeline:

```sh
# 1. Aseprite exports the sheet + the animation metadata
aseprite -b hero.aseprite --sheet hero.png \
         --data hero.json --list-tags --format json-array

# 2. gfx4snes turns the sheet into tiles + a metasprite pointer table
gfx4snes -s 16 -o 16 -u 16 -p -P 2 -i hero.png

# 3. aseprite2snes turns the timeline into AnimClip tables
aseprite2snes -o hero_anim.h -p hero hero.json
```

## What it emits

For a `hero.json` with tags `walk` (frames 0–3, per-frame timing), `idle`
(4–5, uniform) and `hurt` (6–7, ping-pong, play once):

```c
static const u16 hero_walk_frames[]    = { 0, 1, 2, 3 };
static const u8  hero_walk_durations[] = { 6, 6, 9, 6 };
static const AnimClip hero_walk = { hero_walk_frames, hero_walk_durations, 4, 0, ANIM_LOOP, 0 };

static const u16 hero_idle_frames[] = { 4, 5 };
static const AnimClip hero_idle = { hero_idle_frames, 0, 2, 8, ANIM_LOOP, 0 };
/* … */

enum { HERO_ANIM_WALK = 0, HERO_ANIM_IDLE = 1, HERO_ANIM_HURT = 2, HERO_ANIM_COUNT = 3 };
static const AnimClip *const hero_anims[HERO_ANIM_COUNT] = { &hero_walk, &hero_idle, &hero_hurt };
```

Drive it with the `anim.h` player, indexing by state:

```c
#include <snes.h>
#include "hero_anim.h"

AnimPlayer p = ANIM_PLAYER_INIT;
animPlay(&p, hero_anims[HERO_ANIM_WALK]);
u16 frame = animTick(&p);   /* metasprite-table index this tick */
oamDrawMeta(0, x, y, hero_metasprites[frame], BASE_TILE, 0, OBJ_LARGE);
```

## Mapping rules

- **Frame values** default to the frame's index into the metasprite pointer
  table (gfx4snes emits one entry per sheet cell, in the same order Aseprite
  lists frames), so clip frame *i* selects metasprite *i*. For a single hardware
  sprite whose consecutive frames are consecutive OAM tile numbers, `-t STRIDE`
  multiplies the index into a tile number instead.
- **Durations** are Aseprite's per-frame milliseconds converted to ticks at
  `-f` fps (default 60), clamped to `[1, 255]`. If every frame in a clip shares
  one duration, the clip uses the `AnimClip.speed` field and a `NULL` duration
  array; otherwise a parallel `durations[]` is emitted.
- **Direction** sets the frame order: `forward` as-is, `reverse` back-to-front,
  `pingpong` out-and-back (endpoints not repeated), `pingpong_reverse` the
  mirror.
- **Loop vs once**: a tag with Aseprite `repeat` of exactly 1 becomes
  `ANIM_ONCE`; infinite (`0`), absent, or `>1` repeat loops (`ANIM_LOOP`, with a
  warning for the unrepresentable finite `>1` case).
- **No tags?** With no `--list-tags` data the tool emits a single clip named
  `all` over every frame, and warns.

Tag and prefix names are sanitised to C identifiers.

## Tests

`tests/run_golden.py` byte-compares the tool's output on `tests/fixtures/hero.json`
against `tests/golden/hero_anim.h`, and asserts an out-of-range tag hard-fails.
Run via `make test-tools` (needs `make tools` first), or directly:

```sh
python3 tools/aseprite2snes/tests/run_golden.py
```

## Scope (v1)

Animation metadata only. Out of scope by design (owned by `gfx4snes -P`):
decomposing frames into metasprites, laying out tiles, computing OAM tile
numbers, or handling the spritesheet PNG. See the tool header comment in
`src/aseprite2snes.c` for the rationale.
