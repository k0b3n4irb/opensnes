# Object Engine Tutorial {#tutorial_object}

This tutorial covers the `object` module — the entity engine that manages a
fixed pool of game objects, dispatches per-type callbacks, and resolves
collision against the `map` module's tile properties. It is what
`examples/demos/mapandobjects` is built on. Add `object` to `LIB_MODULES`.

This page is the tutorial: what the engine is, how to register a type,
spawn, run the frame, pick a collision routine and remove objects. How the
engine works underneath — the workspace and its reload rules, positions,
field ownership, culling, the refresh pass, what each collision routine
reads, slopes — is @ref tutorial_object_engine.

## What the object engine is, and what it is not

It **is** a fixed pool of 80 entity slots. Each slot holds one 64-byte
`t_objs` record. Every object carries a `type` (0-63), and each type registers
up to three callbacks — init, update, refresh. Once a frame you call
`objUpdateAll()` and the engine walks the live objects and calls each one's
update function with its record already copied into a C-visible workspace.

It is **not** an ECS. There are no components, no queries, no systems. The
record is a fixed struct with fixed field names, and a large part of it is
reserved for the engine's own physics and collision bookkeeping. You cannot
add a field; you repurpose `count`, `dir`, `tempo`, `hitpoints` and
`parentID`, which exist precisely so you have somewhere to put game state.

It is also **not** core SDK. The implementation lives in
`lib/contrib/object.asm` and the policy in `lib/contrib/README.md` is
explicit: it is inherited from PVSnesLib, it carries no API-stability
guarantee, and the core team treats it as example-tier code. This tutorial
documents it as it behaves today, rough edges included — see the
[Gotchas](#object_gotchas) section, which is longer than usual for a reason.

## Opting the module in

`<snes.h>` does **not** pull in `<snes/object.h>`. Include it explicitly, and
include `<snes/map.h>` with it — the `ACT_*` action constants and the `T_*`
tile-property constants your callbacks compare against are declared in
`map.h`, not in `object.h`.

```c
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>
```

The module's dependencies are declared in `make/common.mk`:

```
_DEP_object := map sprite sprite_dynamic
```

Those are not optional extras. The engine reads the map module's camera
(`mapGetCameraX()` / `mapGetCameraY()`) to decide which objects are near enough to update, it
reads the map's metatile property table to resolve collision, and it pokes
the dynamic sprite engine's `oambuffer` refresh flags when an object crosses
the screen edge. A Makefile that lists `object` without them will not link.

```makefile
USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background map object
```

That is the line both shipped examples use.

## What the pool costs you

| Where | Bytes | Contents |
|---|---:|---|
| Bank `$00` (the 8 KB C band) | 5 | the engine's current id, slot and kill flag |
| Bank `$7E` (the far band) | 6910 | 80 × 64-byte records, `objWorkspace` (64) right behind them, the per-type active-list heads, the three 256-byte callback tables, the object-loading scratch buffer |

Measured on `examples/demos/mapandobjects`: the `.obj_bank7e` RAMSECTION is
`$1AFE` bytes at `$7E:2466`, `.obj_bank00` is 5 bytes. The
big allocation sits in the far band, so it does not eat your scarce bank-`$00`
RAM budget (see @ref tutorial_far_ram) — but 6.8 KB is 6.8 KB, and the pool
is fixed at compile time. `OB_MAX` is 80 whether you spawn 3 objects or 80.

Per-frame CPU cost is dominated by two block copies per live, in-range
object: 64 bytes into the workspace before the callback, 60 back out after
— about 870 CPU cycles, 7,000 master cycles, per object. Budget for that
before putting 40 objects on screen. The collision routines a callback
calls add none: they work on the workspace where it is.

## Registering a type

```c
typedef void (*ObjInitFn)(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx);
typedef void (*ObjUpdateFn)(u16 idx);

void objInitFunctions(u8 objtype, ObjInitFn initfct, ObjUpdateFn updfct,
                      ObjUpdateFn reffct);
```

Call it once per type, after `objInitEngine()`, passing the functions
directly:

```c
objInitEngine();
objInitFunctions(0, kitinit,    kitupdate,    0);
objInitFunctions(1, insectinit, insectupdate, 0);
```

The parameters are typed, so a callback with the wrong signature is a compile
error rather than a corrupted stack. Any of the three may be 0: the engine
tests every pointer before dispatching and skips a null one.

C callbacks may land outside bank `$00` — the linker puts each function
where it fits, bank `$01` and up once bank `$00` is full. That works because the compiler pushes a
function pointer as a four-byte slot, bank byte included, and
`objInitFunctions` stores all three bytes. Code ported from an older version
that registered its types through a hand-written assembly routine can drop
that routine.

The three callback signatures are:

```c
void init(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx);
void update(u16 idx);
void refresh(u16 idx);
```

`idx` is the raw slot index (0-79), not the handle `objNew()` returned. That
is the value every engine function taking an object wants — pass it straight
back to `objCollidMap`, `objUpdateXY` and friends.

## Bringing objects into the world

`objInitEngine()` clears the pool, rebuilds the free list and sets the default
gravity and friction. Call it once, before registering types or spawning
anything.

```c
objInitEngine();
objInitFunctions(0, kitinit,    kitupdate,    0);   /* the calls shown above */
objInitFunctions(1, insectinit, insectupdate, 0);
objLoadObjects((u8 *)&objlevel);
```

### Spawning one object by hand

`objNew(type, x, y)` claims a free slot and returns a **handle** — not an
index. The handle packs the slot's rolling unique id in the high byte and the
slot index in the low byte, so that a stale handle to a recycled slot can be
detected. It returns 0 when the pool is full, and also stores the handle in
`objGetCurrentId()`.

The canonical init callback looks like this (from
`examples/demos/mapandobjects/kit.c`):

```c
void kitinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;                     /* pool full */

    /* objNew already copied the new record into objWorkspace;
       objGetPointer re-selects it and validates the handle. */
    objGetPointer(objGetCurrentId());
    kitid = objGetCurrentId();

    /* Width 14 + xofs 1 is critical for slope collision (width 16 bugs) */
    objWorkspace.width  = 14;
    objWorkspace.xofs   = 1;
    objWorkspace.height = 16;
    objWorkspace.action = ACT_STAND;
    /* ... set up the sprite ... */
}
```

The collision box is **yours to set**. `objNew` does not guess it: `width`,
`height`, `xofs` and `yofs` are zero until the init callback fills them in,
and a zero-sized box collides with nothing.

### Spawning from map data

`objLoadObjects(const u8 *table)` walks a table of five 16-bit words per
entry, terminated by a single `0xFFFF`:

| Word | Meaning |
|---|---|
| 0 | `x` — spawn position in map pixels |
| 1 | `y` — spawn position in map pixels |
| 2 | `type` — the type index whose init callback is called |
| 3 | `minx` — passed through to the init callback |
| 4 | `maxx` — passed through to the init callback |

It does **not** create the objects itself. For each row it calls that type's
*init* callback with the five values; the callback is what calls `objNew`.
That is why the enemy init functions in `mapandobjects` copy `minx` / `maxx`
into `objWorkspace` — those two words are how a level editor hands a patrol
range to an enemy.

You do not hand-author this table. It is the `.o16` file @ref tools_opensnes_level
(`opensnes-level convert`) writes from a Tiled object layer **named `Entities`** (the name is matched exactly).
Each Tiled object's *type* becomes word 2; custom string properties named
`minx` and `maxx` become words 3 and 4. See @ref tutorial_map for the rest of
the pipeline. The converter caps a map at 64 objects, below the engine's pool
of 80.

```c
#include "res/level.inc"   /* generated: extern const u8 level_objects[], ... */

objLoadObjects(level_objects);
```

## The workspace, in short

The 80 records live in far RAM, so your callbacks never touch them
directly. Before each update callback the engine copies the object's record
into `objWorkspace`, a single `FAR t_objs`; your callback reads and writes
its fields; when the callback returns the engine copies it back. `objNew()`,
`objGetPointer()` and `objKill()` load another object into it. Positions
are 24-bit fixed point (`xpos[1] | (xpos[2] << 8)` is the pixel), and
`objUpdateXY(idx)` is what moves an object by its velocity. The rules in
full, with the fields the engine owns, are in @ref tutorial_object_engine.

## The frame the engine expects

Both examples run the same loop, and the placement is deliberate:

```c
while (1) {
    mapUpdate();        /* stage the map's reveal strip — active display */
    objUpdateAll();     /* every callback, incl. collision — active display */

    WaitForVBlank();
    mapVblank();        /* the map's VRAM DMA — must be in VBlank */
    /* the NMI handler auto-flushes the dynamic sprite engine */
}
```

`objUpdateAll()` belongs **before** `WaitForVBlank()`, in the active frame. It
is pure computation plus two block copies per object; it writes no PPU
register and no VRAM. Putting it after `WaitForVBlank()` would spend your
VBlank budget on gameplay logic and starve `mapVblank()`.

Your callbacks stage sprite output the same way — `oamDynamicDraw(n)` queues
a dynamic sprite, and the NMI handler uploads the queue.

Which objects the engine actually updates (a culling window around the
map camera) and the optional sprite refresh pass are described in
@ref tutorial_object_engine.

## Collision: choosing a routine

All three map-collision routines read the `map` module's metatile
property table, so call none of them before `mapLoad()`. What each reads
and how the `T_*` tile properties act is in @ref tutorial_object_engine.

| Function | Gravity | Friction | Slopes | Use it for |
|---|---|---|---|---|
| `objCollidMap(idx)` | yes | yes | no | side-view platformers on flat and stepped terrain |
| `objCollidMapWithSlopes(idx)` | yes | yes | yes | platformers with diagonal terrain |
| `objCollidMap1D(idx)` | no | no | no | top-down movement, where "down" is not a direction |

All three write back into the workspace:

- **`tilestand`** — non-zero when the object is resting on something. This is
  the ground test. `kit.c` gates its jump on it:
  `if (objWorkspace.tilestand != 0) { ... }`.
- **`tilesprop`** — the property word of the tile under the object's feet.
- **`tilebprop`** — the property word of the tile hit on the left or right
  side, i.e. the wall test.
- **`ypos` and `yvel`** — on a downward hit, Y is snapped to the tile boundary
  and `yvel` zeroed. On an airborne frame, gravity is added to `yvel` (capped
  at 10 pixels per frame).
- **`xvel`** — friction is subtracted each frame, and the velocity is zeroed
  outright when a wall is hit.

They take the slot **index**, the value handed to your update callback — not
the handle from `objNew`.

### Object against object

```c
u16 objCollidObj(u16 idx1, u16 idx2);
```

A plain AABB test between two objects' boxes (`xpos + xofs`, `width`,
`ypos + yofs`, `height`), returning 1 on overlap and 0 otherwise. It takes
a callback's `idx` or a handle, as do `objCollidMap`, `objCollidMapWithSlopes`,
`objCollidMap1D` and `objUpdateXY`: they work on a live slot and mask the
handle's id byte themselves. Only `objKill` and `objGetPointer` need
the whole handle, because the id byte is how they detect a stale one. Like the
map collision routines it flushes the workspace on
entry, so an object that has just moved itself is tested where it now is.

## Removing objects

Three ways, in order of how often you want them:

```c
objKillCurrent();        /* inside an update callback: kill the current object */
objKill(handle);        /* from anywhere: kill by handle */
objKillAll();           /* wipe the pool, e.g. on level change */
```

`objKillCurrent()` is the one to reach for. `objUpdateAll` clears its flag before
each update callback and checks it after; calling it kills the object the engine is
currently updating, after the workspace has been written back, which is the
only safe moment. It is **not** checked around the refresh callback — calling
it there does nothing until the next update pass.

`objKill(handle)` takes a handle, unlike almost everything else in this API.
It unlinks the object from its type's active list and returns the slot to the
free list. Remember that it loads the victim into the workspace on the way,
so the workspace is the victim afterwards (your own edits were flushed to
your slot first).

## Gotchas {#object_gotchas}

This module is inherited from PVSnesLib. Every public function is executed by
a test, but eight of the eighteen (`objCollidMap1D`, `objCollidObj`,
`objInitFriction1D`, `objInitGravity`, `objKill`, `objKillAll`,
`objKillCurrent`, `objRefreshAll`) only by the library fixture (`testing/fixtures/libtests`),
not by any example — they have assertions, not mileage.

### `objCollidMap1D` has no friction unless you ask for it

The routine zeroes velocity on a wall hit but does not decelerate otherwise,
as in PVSnesLib. `objInitFriction1D(amount)` makes every `objCollidMap1D`
call move `xvel` and `yvel` toward zero by `amount`, clamped at zero;
`objInitEngine()` resets it to 0 (no friction).

### A screen-edge crossing forces a full sprite refresh

Whenever any object's `onscreen` flag flips, the engine sets the refresh flag
on **all 128** `oambuffer` entries, once per frame at most. That re-uploads
every dynamic sprite's tiles that frame. It is correct, but it is a cost
spike that lands exactly when a scrolling camera is busiest — worth knowing
when you are chasing a dropped frame (see @ref tutorial_profiling).

### `objNew` leaves `objGetCurrentId()` stale when the pool is full

On failure `objNew` returns 0 but does not clear the current id, which still
holds the previous successful handle. Nothing is written to that object, but
guard with the return value, as the examples do:
`if (objNew(type, xp, yp) == 0) return;`.

## When not to use the object engine

Most of the corpus does not use it, and that is a reasonable default. Reach
for your own array when:

- **Your entities are few and homogeneous.** `examples/games/breakout` tracks
  one ball and one paddle; `collision_demo` tracks four enemies in four
  parallel arrays. A 64-byte record and a callback dispatch buy nothing there.
- **You are not on the `map` module.** The collision routines are
  map-module-only, the culling window is relative to the map camera, and the
  dependency list drags in three modules. Without a streamed map, you are
  paying 6.8 KB of RAM for a linked list.
- **Your entity state does not fit the record.** If you need four custom
  fields per entity, you are already fighting `count` / `dir` / `tempo` /
  `hitpoints`. A struct of your own is clearer and cheaper.
- **You want top-down movement.** `objCollidMap1D` is the intended answer and
  is one of the eight functions no example executes.
- **You want the tile-collision behaviour without the entity model.** The map
  module exposes `mapGetMetaTilesProp()` directly, and
  @ref tutorial_collision covers the hand-rolled patterns
  `examples/demos/likemario` uses to do platformer physics with no object
  engine at all.

Where it earns its keep is the case it was written for: a scrolling
side-view level with a dozen or more heterogeneous entities placed in a map
editor, each with its own behaviour, where off-screen culling and
data-driven spawning matter. That is exactly
`examples/demos/mapandobjects`.

## See also

- [Scrolling maps](map.md) — the `map` module, the Tiled pipeline and the
  `.o16` object layer this engine consumes.
- [Collision detection](collision.md) — the hand-rolled alternative, and the
  sprite-versus-hitbox alignment problem that bites either way.
- [Sprites & animation](sprites.md) — `oambuffer`, `oamDynamicDraw` and the
  dynamic sprite engine your callbacks draw through.
- `examples/demos/mapandobjects` — three entity types, map-driven spawning,
  patrol AI.
- `lib/contrib/README.md` — what contrib status means for API stability.
- @ref tutorial_object_engine "How the object engine works" — the engine reference.
- @ref object.h "Object API reference" · @ref map.h "Map API reference"
