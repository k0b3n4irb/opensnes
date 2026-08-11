# aseprite2snes — Aseprite animations → AnimClip tables {#tools_aseprite2snes}

Aseprite is where sprite animation is actually authored: frames on a timeline,
grouped into named **tags** (`walk`, `idle`, `hurt`), each with a playback
**direction** and a per-frame **duration** in milliseconds. That timeline is
real design work — and until now none of it survived the trip to the SNES.
`gfx4snes` turns a spritesheet into tiles and, with `-P`, into metasprite
geometry, but it has no concept of a tag or a duration. So the animation was
retyped by hand into `DECLARE_ANIM_CLIP()` calls, one frame at a time — the
single most tedious, error-prone step left in the sprite pipeline.

aseprite2snes closes that gap. It reads the JSON Aseprite writes with
`--data --list-tags` and emits a C header of ready-to-use `AnimClip` tables for
the @ref tutorial_animation "anim.h player" — one clip per tag, frame indices and
per-frame tick durations included, playback direction folded into the frame
order, looping vs one-shot derived from the tag.

## Where it fits

aseprite2snes is the animation half of a two-tool sprite step, paired with
`gfx4snes -P`:

```
              ┌─ gfx4snes -P    ──► tiles + metasprite pointer table  (oamDrawMeta)
  hero.ase ──►┤
              └─ aseprite2snes  ──► AnimClip tables, one per tag       (animPlay / animTick)
```

gfx4snes owns the pixels and the metasprite table; aseprite2snes owns the
timeline that indexes it. The two outputs meet at the frame value: a clip's
frame *i* selects metasprite *i*.

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

The full three-step pipeline for one animated sprite:

```sh
aseprite -b hero.aseprite --sheet hero.png \
         --data hero.json --list-tags --format json-array
gfx4snes -s 16 -o 16 -u 16 -p -P 2 -i hero.png
aseprite2snes -o hero_anim.h -p hero hero.json
```

## The generated header

Given tags `walk` (per-frame timing), `idle` (uniform) and `hurt` (ping-pong,
play once), you get an `AnimClip` per tag plus an index enum and a pointer
table over all clips:

```c
static const u16 hero_walk_frames[]    = { 0, 1, 2, 3 };
static const u8  hero_walk_durations[] = { 6, 6, 9, 6 };
static const AnimClip hero_walk = { hero_walk_frames, hero_walk_durations, 4, 0, ANIM_LOOP, 0 };

static const u16 hero_idle_frames[] = { 4, 5 };
static const AnimClip hero_idle = { hero_idle_frames, 0, 2, 8, ANIM_LOOP, 0 };

enum { HERO_ANIM_WALK = 0, HERO_ANIM_IDLE = 1, HERO_ANIM_HURT = 2, HERO_ANIM_COUNT = 3 };
static const AnimClip *const hero_anims[HERO_ANIM_COUNT] = { &hero_walk, &hero_idle, &hero_hurt };
```

A uniform-duration clip uses the `AnimClip.speed` field with a `NULL` duration
array; a clip whose frames differ in length carries a parallel `durations[]`.
Include the header **after** `<snes.h>` — it needs `AnimClip`, `ANIM_LOOP` and
`ANIM_ONCE` from `anim.h`.

Drive it by state, indexing the pointer table:

```c
#include <snes.h>
#include "hero_anim.h"

AnimPlayer p = ANIM_PLAYER_INIT;
animPlay(&p, hero_anims[HERO_ANIM_WALK]);
u16 frame = animTick(&p);                  /* metasprite-table index this tick */
oamDrawMeta(0, x, y, hero_metasprites[frame], BASE_TILE, 0, OBJ_LARGE);
```

## Mapping rules

| Aseprite | Becomes | Notes |
|----------|---------|-------|
| a tag `name` | one `AnimClip` `<prefix>_<name>` | plus an `<PREFIX>_ANIM_<NAME>` enum index |
| `from`…`to` | the clip's `frames[]` | values are metasprite indices; `-t` multiplies |
| per-frame `duration` (ms) | `durations[]` in ticks | converted at `-f` fps, clamped to 1…255 |
| uniform durations | `AnimClip.speed`, `NULL` durations | collapses the array automatically |
| `direction` | frame order | `forward`, `reverse`, `pingpong`, `pingpong_reverse` |
| `repeat` = 1 | `ANIM_ONCE` | `0` / absent / `>1` → `ANIM_LOOP` |

`pingpong` runs out and back without repeating either endpoint. With no
`--list-tags` metadata the tool emits a single clip named `all` over every
frame and warns. Tag and prefix names are sanitised to C identifiers.

## Scope

v1 is the animation layer only; it never touches image data. Decomposing frames
into metasprites, laying out tiles and computing OAM tile numbers are owned by
`gfx4snes -P` — see @ref tools_gfx4snes. An out-of-range tag range, missing
`frames`, or malformed JSON is a hard error with a non-zero exit.
