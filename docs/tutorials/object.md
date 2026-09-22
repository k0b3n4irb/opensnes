# Object Engine Tutorial {#tutorial_object}

This tutorial covers the `object` module — the entity engine that manages a
fixed pool of game objects, dispatches per-type callbacks, and resolves
collision against the `map` module's tile properties. It is what
`examples/games/mapandobjects` and `examples/maps/slope_collision` are built
on. Add `object` to `LIB_MODULES`.

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
(`x_pos` / `y_pos`) to decide which objects are near enough to update, it
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
| Bank `$00` (the 8 KB C band) | 69 | `objWorkspace` (64) + `objgetid`, `objptr`, `objtokill` |
| Bank `$7E` (the far band) | 6842 | 80 × 64-byte records, the per-type active-list heads, the three 256-byte callback tables, the object-loading scratch buffer |

Measured on `examples/maps/slope_collision`: the `.obj_bank7e` RAMSECTION is
`$1ABA` bytes at `$7E:2466`, `.obj_bank00` is `$45` bytes at `$00:0109`. The
big allocation sits in the far band, so it does not eat your scarce bank-`$00`
RAM budget (see @ref tutorial_far_ram) — but 6.8 KB is 6.8 KB, and the pool
is fixed at compile time. `OB_MAX` is 80 whether you spawn 3 objects or 80.

Per-frame CPU cost is dominated by two 64-byte block copies per live,
in-range object: one into the workspace before the callback, one back out
after. Budget for that before putting 40 objects on screen.

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
objInitFunctions(0, marioinit,  marioupdate,  0);
objInitFunctions(1, goombainit, goombaupdate, 0);
```

The parameters are typed, so a callback with the wrong signature is a compile
error rather than a corrupted stack. Any of the three may be 0: the engine
tests every pointer before dispatching and skips a null one.

C callbacks routinely land outside bank `$00` — in `slope_collision.sym`,
`marioinit` sits in bank `$01`. That works because the compiler pushes a
function pointer as a four-byte slot, bank byte included, and
`objInitFunctions` stores all three bytes. Until 2026-09-18 it did not (see
the Gotchas), and both examples registered their types through a hand-written
assembly routine; if you are porting code that copied that shim, delete it.

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
objRegisterTypes();
objLoadObjects((u8 *)&objmario);
```

### Spawning one object by hand

`objNew(type, x, y)` claims a free slot and returns a **handle** — not an
index. The handle packs the slot's rolling unique id in the high byte and the
slot index in the low byte, so that a stale handle to a recycled slot can be
detected. It returns 0 when the pool is full, and also stores the handle in
the global `objgetid`.

The canonical init callback looks like this (from
`examples/maps/slope_collision/mario.c`):

```c
void marioinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;                     /* pool full */

    /* objNew already copied the new record into objWorkspace;
       objGetPointer re-selects it and validates the handle. */
    objGetPointer(objgetid);
    marioid = objgetid;

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

You do not hand-author this table. It is the `.o16` file `tmx2snes` emits
from a Tiled object layer **named `Entities`** (the name is matched exactly).
Each Tiled object's *type* becomes word 2; custom string properties named
`minx` and `maxx` become words 3 and 4. See @ref tutorial_map for the rest of
the pipeline. The converter caps a map at 64 objects, below the engine's pool
of 80.

```c
extern u8 objmario;                     /* .incbin of the .o16 */
objLoadObjects((u8 *)&objmario);
```

## The workspace idiom

This is the part a newcomer gets wrong, so it is worth understanding *why* it
is shaped this way.

The 80 records live in bank `$7E`, above `$2000`. Plain C pointers on this
target are bank-`$00` addresses (see the RAM constraint in
`KNOWN_LIMITATIONS.md`), so C simply cannot reach the array. Rather than
making every field access a far read, the engine keeps **one** 64-byte
scratch record in bank `$00`, where C can touch it with ordinary struct
syntax:

```c
extern t_objs objWorkspace;
```

The contract is a copy-in / copy-out sandwich around every callback:

1. Before calling your update callback, `objUpdateAll` copies
   `objbuffers[idx]` into `objWorkspace`.
2. Your callback reads and writes `objWorkspace` fields freely.
3. When the callback returns, `objUpdateAll` copies `objWorkspace` **back**
   into the slot it mirrors — see the next section for why that wording
   matters.

Engine functions you call from inside the callback keep the illusion intact:
`objCollidMap`, `objCollidMapWithSlopes`, `objCollidMap1D` and `objUpdateXY`
each flush the workspace to the record on entry and reload it on exit, so the
values you see afterwards are the ones the routine computed.

### Reaching an object you are not currently updating

`objGetPointer(handle)` validates a handle and loads that object into the
workspace. It sets the global `objptr` to the slot index **plus one**, or to
0 when the handle is stale — that is the validity test:

```c
objGetPointer(some_handle);
if (objptr == 0) {
    /* that object is dead; the workspace holds whatever was there before */
    return;
}
u16 hp = objWorkspace.hitpoints;
```

### What reloads the workspace

There is only one workspace, so anything that loads a different object
replaces what you were looking at:

- `objNew()` — leaves the *new* object in the workspace.
- `objGetPointer()` — leaves the object you asked for in the workspace.
- `objKill()` — calls `objGetPointer` internally, so it reloads it too.

Until 2026-09-19 this was a trap: the engine wrote the workspace back into
the slot **it was iterating**, whatever the workspace held by then, so an
update callback that peeked at the player returned with the player's 64 bytes
written over the enemy's slot. The workspace now tracks its **owner** — the
slot it mirrors. Every reload first flushes the current contents to that
owner, and every write-back goes to the owner, not to the caller's index:

```c
void enemy_update(u16 idx) {
    objWorkspace.xvel = -64;             /* edit the enemy... */
    objGetPointer(player_handle);        /* ...flushed to the enemy's slot;
                                            workspace now = the player */
    u16 px = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    /* Returning here is safe: the write-back goes to the player's slot,
       which it already matches. The enemy keeps its own record. */
}
```

What is still yours to manage: after the peek, `objWorkspace` **is** the
player. Writing to it edits the player, and the collision routines you call
next with the enemy's `idx` reload the enemy first. If you want to keep
editing your own object, call `objGetPointer(my_own_handle)` to come back.

### Positions are 24-bit fixed point

`xpos` and `ypos` are three-byte fields, not integers. Byte 0 is the
sub-pixel fraction; bytes 1-2 are the whole-pixel world coordinate. Velocity
(`xvel` / `yvel`) is a signed 8.8 value added into that, so 256 is one pixel
per frame and `0x0140` is 1.25 pixels per frame.

Read a pixel position the way the examples do:

```c
mariox = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
marioy = objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8);
```

Write one by setting both bytes (and zeroing the fraction if you want an
exact landing):

```c
objWorkspace.xpos[1] = 0;
objWorkspace.xpos[2] = 0;
```

Note that `objUpdateXY(idx)` is what actually integrates velocity into
position. The engine never does it for you; a callback that never calls it
has objects that accelerate and never move.

### Who owns which field

| Field | Written by |
|---|---|
| `prev`, `next`, `nID`, `type` | engine only — do not touch |
| `xpos`, `ypos` | engine (`objNew`, `objUpdateXY`, collision snapping) and you |
| `xvel`, `yvel` | you; collision zeroes them on impact and applies gravity/friction |
| `width`, `height`, `xofs`, `yofs` | you, in the init callback |
| `xmin`, `xmax` | you (conventionally the patrol range from the `.o16`) |
| `tilestand`, `tilesprop`, `tilebprop` | collision routines |
| `onscreen` | `objUpdateAll` |
| `action` | you (plus one engine write — see the gotcha below) |
| `sprnum`, `sprframe`, `sprflip`, `sprpal`, `sprid3216`, `sprblk3216` | you |
| `count`, `dir`, `tempo`, `hitpoints`, `parentID`, `status`, `tileabove`, `sprrefresh` | you — the engine never reads or writes these |

The last row matters: despite their Doxygen comments, `tileabove`, `status`
and `sprrefresh` are never touched by the engine. They are free storage.

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

### Which objects actually get updated

`objUpdateAll` does not call every live object's callback. It first tests the
object against a **virtual screen** — roughly one screen plus 64 pixels of
margin on each side, relative to the map camera (`-64` to `320` in X, `-64`
to `288` in Y). Objects outside that box are skipped entirely: they do not
move, do not fall, do not animate. This is the engine's culling strategy, and
it is the reason enemies far off-screen cost nothing.

A second, tighter test (`-32` to `256` X, `-32` to `224` Y) maintains the
`onscreen` flag. That flag is what `objRefreshAll()` filters on.

Two consequences worth internalising:

- The camera used is the `map` module's `x_pos` / `y_pos`. Without the map
  module scrolling, they stay at the origin and the window is fixed there.
- Iteration order is type 0 first, then type 1, and so on; within a type, the
  most recently created object is visited first, because `objNew` pushes onto
  the head of that type's list.

### The sprite refresh pass

`objRefreshAll()` is a second pass that calls each **on-screen** object's
*refresh* callback. It exists so a scrolling game can separate "think" from
"draw" and avoid sprite flicker at the screen edge. Run it after
`objUpdateAll()` — it depends on the `onscreen` flags that pass sets.

Neither shipped example uses it; both do their drawing inline in the update
callback and register a null refresh pointer. If you register a refresh
callback for a type, register a real one — see the gotcha below.

## Collision: which routine reads what

All three map-collision routines read the **map module's** metatile property
table, the one `mapLoad()` built from the `.b16` file. They resolve the
object's collision box to map cells, look up each cell's property word, and
act on it. That is the whole relationship between the two modules: the
`object` engine has no collision data of its own, and calling any of these
before `mapLoad()` reads an uninitialised table.

The property words are the `T_*` constants from `map.h`:

| Property | Value | Effect in the collision routines |
|---|---|---|
| `T_SOLID` | `0xFF00` | blocks — any property with a non-zero high byte is treated as solid |
| `T_LADDER` | `0x0001` | treated as standable — the object rests on it |
| `T_FIRES` | `0x0002` | sets `ACT_BURN` |
| `T_SPIKE` | `0x0004` | sets `ACT_DIE` |
| `T_PLATE` | `0x0008` | treated as standable (one-way platform) |
| `T_SLOPEU1` … `T_SLOPEUD2` | `0x0020`-`0x0025` | slope surfaces — only `objCollidMapWithSlopes` understands these |

Gravity is additionally suppressed on a frame where nothing solid was found
underneath but the last tile property read was non-zero *and* the object's
`action` carries the `ACT_CLIMB` bit — that is the hook for ladder climbing.

### Choosing a routine

| Function | Gravity | Friction | Slopes | Use it for |
|---|---|---|---|---|
| `objCollidMap(idx)` | yes | yes | no | side-view platformers on flat and stepped terrain |
| `objCollidMapWithSlopes(idx)` | yes | yes | yes | platformers with diagonal terrain |
| `objCollidMap1D(idx)` | no | no | no | top-down movement, where "down" is not a direction |

All three write back into the workspace:

- **`tilestand`** — non-zero when the object is resting on something. This is
  the ground test. `mario.c` gates its jump on it:
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
handle's id byte themselves (since 2026-09-21 — before that a handle sent them
to memory past the pool, silently). Only `objKill` and `objGetPointer` need
the whole handle, because the id byte is how they detect a stale one. Like the
map collision routines it flushes the workspace on
entry, so an object that has just moved itself is tested where it now is.

## Slopes, and what the tests pin

`objCollidMapWithSlopes` is what `examples/maps/slope_collision` is named for.
A slope tile is not a wall: walking into one must *lift* the object to the
surface height for its horizontal position within the tile, not stop it. The
routine does that by indexing a per-slope-type height table with the object's
centre X relative to the tile.

The six slope properties, as `map.h` names them, are a 1×1 slope ascending
(`T_SLOPEU1`) and descending (`T_SLOPED1`) to the right, plus the lower and
upper halves of a 2×1 slope in each direction (`T_SLOPELU2`, `T_SLOPELD2`,
`T_SLOPEUU2`, `T_SLOPEUD2`). You assign them per tile in the map's attribute
layer.

The functional test `tools/luna-test/manifests/movement_slope_collision.toml`
pins exactly the distinction that matters, measured on luna with RIGHT held:

| Frame | Position | What it proves |
|---|---|---|
| 60 | `(32, 96)` | spawned and settled on flat ground |
| 120 | `(99, 96)` | 67 pixels along the flat — Y untouched |
| 144 | `(127, 89)` | on the slope: 28 pixels right, lifted 7 pixels up |
| 216 | `(209, 96)` | back on flat ground past the slope |
| 300 | `(209, 96)` | RIGHT still held, but a solid tile stops him dead |

Frames 144 and 300 are the two halves of the contract: a slope lifts, a wall
stops. The companion manifest for `mapandobjects` pins the flat-ground case
for `objCollidMap` the same way — walking right and left along a floor with
Y constant at 192, which is the "he never falls through the floor" negative.

One practical note from the same example: Mario is declared with
`width = 14` and `xofs = 1` rather than a full 16. The comment in `mario.c`
says a width of 16 misbehaves on slopes. Leave a pixel of slack on each side
of a 16-pixel sprite.

## Removing objects

Three ways, in order of how often you want them:

```c
objtokill = 1;          /* inside an update callback: kill the current object */
objKill(handle);        /* from anywhere: kill by handle */
objKillAll();           /* wipe the pool, e.g. on level change */
```

`objtokill` is the one to reach for. `objUpdateAll` clears it before each
update callback and checks it after; setting it kills the object the engine is
currently updating, after the workspace has been written back, which is the
only safe moment. It is **not** checked around the refresh callback — setting
it there does nothing until the next update pass.

`objKill(handle)` takes a handle, unlike almost everything else in this API.
It unlinks the object from its type's active list and returns the slot to the
free list. Remember that it loads the victim into the workspace on the way,
so the workspace is the victim afterwards (your own edits were flushed to
your slot first).

## Gotchas {#object_gotchas}

This module is inherited from PVSnesLib and has not had the audit the core
modules have. The following are verified against `lib/contrib/object.asm` and
the shipped ROMs. Every public function is now executed by a test, but seven
of the sixteen (`objCollidMap1D`, `objCollidObj`, `objInitFriction1D`,
`objInitGravity`, `objKill`, `objKillAll`, `objRefreshAll`) only by the
library fixture (`devtools/libtests`), not by any example — they have
assertions, not mileage.

### 🟢 `objInitFunctions` stored garbage — fixed 2026-09-18

It read its arguments with the **pre-A6 stack map**, from before pointers
became four-byte slots, so every offset was wrong: what it took for the type
was the update callback's bank byte. Its comment then claimed C could not
pass a bank byte at all, and it wrote `$00` into all three. Both examples
worked around it with a hand-written assembly routine using `:label`.

The compiler pushes `pea.w :fn` alongside `pea.w fn` for every pointer, so
the bank is on the stack — the routine now reads it. Both examples dropped
their shim and register from C, and their manifests pass unchanged, which is
what proves the dispatch reaches bank `$01` correctly.

### 🟢 A type with no registered callbacks jumped into nowhere — fixed 2026-09-19

Neither the update pass, the refresh pass nor the `objLoadObjects` init
dispatch checked for a null pointer, and the tables start zeroed, so an object
whose type was never registered called address `$00:0000`. All three sites now
test the 24-bit pointer and skip the call. An unregistered type is inert: it
keeps its slot, is never updated, and `objLoadObjects` skips its table entry.
A null refresh callback is likewise safe, so `objRefreshAll()` no longer
depends on every type having one.

### 🟢 `objLoadObjects` ignored the bank byte of your pointer — fixed 2026-09-18

The routine block-copied the table into its scratch buffer with the source
bank forced to `$00`. The examples got away with it because their `.o16` data
sits in a `SUPERFREE` section the linker happens to place in bank `$00` — an
`ASSET_SECTION` table (banks 7-1 by design, see
`.claude/rules/bank0_budget.md`) would have loaded garbage instead. The DMA
now takes the bank the caller pushed, so the table can live anywhere.

### 🟢 `objInitGravity`'s gravity argument did nothing — fixed 2026-09-18

`objInitGravity(gravity, friction)` stored both values, but the three
collision routines added the assemble-time constant `GRAVITY` instead of
reading the one you passed; only friction was honoured. They now read the
variable, which `objInitEngine` still seeds with `GRAVITY` (41, i.e. 41/256
pixel per frame squared), so the default is unchanged and the argument is
finally real.

### 🟢 `ACT_BURN` and `ACT_DIE` went to slot 0 — fixed 2026-09-18

When a collision routine found a `T_FIRES` or `T_SPIKE` tile it wrote the
action word **without indexing by the current object**, so the hazard always
landed in object slot 0 instead of the one that touched it. All three
routines did it, at six sites. Writing this page is what found it.

The index register already held the object there — the neighbouring
`tilesprop` stores use it — so the fix was the missing `,x`. No map in the
corpus carries `T_FIRES` or `T_SPIKE`, which is both why it survived and why
the fix could not change any existing behaviour.

### 🟢 A callback that peeked at another object became a copy of it — fixed 2026-09-19

The write-back after a callback went to the slot being iterated, whatever the
workspace held. The workspace is now owner-tracked (see *What reloads the
workspace* above): edits are flushed to their owner before any reload, and
write-backs go to the owner. The cost is about 140 CPU cycles per callback —
enough to move a late-frame write across a frame boundary, which is how the
slope example's manifest noticed.

### 🟢 `objCollidObj` returned garbage for "no contact" — fixed 2026-09-20

The routine stored its result in a scratch variable and returned without
loading it, so the caller got whatever the accumulator held: 1 on contact by
accident, and on every "no contact" exit an intermediate such as `x + width`
— non-zero, so truthy. Nothing in the corpus called it, which is how it
survived. It also read the slots without flushing the workspace first, so a
callback that had just moved its object tested the old position. Both fixed;
the first call from a test found them.

### 🟢 Three ways a pending workspace edit could be lost or do damage — fixed 2026-09-20

The owner-tracked workspace of 2026-09-19 had three holes, all found by the
first test that edited an object *outside* a callback and then let the engine
run:

- Reloading the slot the workspace already mirrored skipped the flush and
  reloaded, discarding the edit (`objNew`, `objGetPointer` on the same
  object, field writes, `objUpdateAll`). Every reload now flushes first.
- The write-back carried all 64 bytes, including the active-list links the
  engine rewrites in the slot while the workspace holds the copy taken at
  load time. The write-back now covers bytes 4 to 63; the links never travel.
- `objUpdateAll` and `objRefreshAll` wrote `onscreen` into the slot and only
  then flushed a pending edit, putting the workspace's stale `onscreen` back
  over it. Both passes now flush on entry.

### 🟢 `objCollidMap1D` had no friction, and no way to get any — opt-in since 2026-09-19

The routine zeroes velocity on a wall hit but never decelerates otherwise;
PVSnesLib's `FRICTION1D` constant was defined and never used. The default is
unchanged — no friction, so ported code behaves the same — but
`objInitFriction1D(amount)` now makes every `objCollidMap1D` call move `xvel`
and `yvel` toward zero by `amount`, clamped at zero. `objInitEngine()` resets
it to 0.

### 🟡 A screen-edge crossing forces a full sprite refresh

Whenever any object's `onscreen` flag flips, the engine sets the refresh flag
on **all 128** `oambuffer` entries, once per frame at most. That re-uploads
every dynamic sprite's tiles that frame. It is correct, but it is a cost
spike that lands exactly when a scrolling camera is busiest — worth knowing
when you are chasing a dropped frame (see @ref tutorial_profiling).

### 🟡 `objNew` leaves `objgetid` stale when the pool is full

On failure `objNew` returns 0 but does not clear `objgetid`, which still holds
the previous successful handle. This used to let `objLoadObjects` write the
workspace into the previously created object; since the owner-tracked
workspace (2026-09-19) the workspace has no owner at that point and nothing is
written. `objgetid` itself is still stale, so keep guarding with the return
value, as the examples do: `if (objNew(type, xp, yp) == 0) return;`.

### 🟢 `objKillAll` leaked slots — fixed 2026-09-19

After killing every live object it forced the free-list head back to slot 0.
`objKill` had already pushed each freed slot onto the list, so every slot
chained ahead of slot 0 — anything killed after it — became unreachable. Two
objects of different types were enough: the pool came back with 79 slots, and
lost more on each level change. The reset is gone; the library fixture
asserts that 80 allocations succeed after `objKillAll()`.

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
  is one of the seven never-executed functions.
- **You want the tile-collision behaviour without the entity model.** The map
  module exposes `mapGetMetaTilesProp()` directly, and
  @ref tutorial_collision covers the hand-rolled patterns
  `examples/games/likemario` uses to do platformer physics with no object
  engine at all.

Where it earns its keep is the case it was written for: a scrolling
side-view level with a dozen or more heterogeneous entities placed in a map
editor, each with its own behaviour, where off-screen culling and
data-driven spawning matter. That is exactly
`examples/games/mapandobjects`.

## See also

- [Scrolling maps](map.md) — the `map` module, the Tiled pipeline and the
  `.o16` object layer this engine consumes.
- [Collision detection](collision.md) — the hand-rolled alternative, and the
  sprite-versus-hitbox alignment problem that bites either way.
- [Sprites & animation](sprites.md) — `oambuffer`, `oamDynamicDraw` and the
  dynamic sprite engine your callbacks draw through.
- `examples/games/mapandobjects` — three entity types, map-driven spawning,
  patrol AI.
- `examples/maps/slope_collision` — one entity type, slope-aware collision.
- `lib/contrib/README.md` — what contrib status means for API stability.
- @ref object.h "Object API reference" · @ref map.h "Map API reference"
