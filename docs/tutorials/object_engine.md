# How the Object Engine Works {#tutorial_object_engine}

This page is the reference behind @ref tutorial_object, what the engine
does around your callbacks, field by field and routine by routine. Read the
tutorial first; come here when an object does not move, does not collide,
or reads another object's values.

## The workspace idiom

This is the part a newcomer gets wrong, so it is worth understanding *why* it
is shaped this way.

The 80 records live in bank `$7E`, above `$2000`. Plain C pointers on this
target are bank-`$00` addresses (see the RAM constraint in
`KNOWN_LIMITATIONS.md`), so a plain pointer cannot reach the array. The
engine keeps **one** 64-byte scratch record at a fixed address, which C
touches with ordinary struct syntax:

```c
extern FAR t_objs objWorkspace;
```

It is a `FAR` object (@ref tutorial_far_ram): `objWorkspace.xpos` reads and
writes at the cost of any global, and a pointer to it, or to one of its
fields, is a `FAR` pointer. It sits right behind the pool, which is what
lets the collision routines work on it where it is.

The contract is a copy-in / copy-out sandwich around every callback:

1. Before calling your update callback, `objUpdateAll` copies
   `objbuffers[idx]` into `objWorkspace`.
2. Your callback reads and writes `objWorkspace` fields freely.
3. When the callback returns, `objUpdateAll` copies `objWorkspace` **back**
   into the slot it mirrors — see the reload rules below for why that
   wording matters.

Engine functions you call from inside the callback keep the illusion intact:
`objCollidMap`, `objCollidMapWithSlopes`, `objCollidMap1D` and `objUpdateXY`
work on the workspace itself when it holds the object you name — the case
inside that object's callback — so the values you see afterwards are the
ones the routine computed, and nothing was copied. Called for any other
object, they flush the workspace to its record, work on the record you
named, and leave that one in the workspace.

## Reaching an object you are not currently updating

`objGetPointer(handle)` validates a handle and loads that object into the
workspace. It returns the slot index **plus one**, or 0 when the handle is
stale — that is the validity test:

```c
if (objGetPointer(some_handle) == 0) {
    /* that object is dead; the workspace holds whatever was there before */
    return;
}
u16 hp = objWorkspace.hitpoints;
```

## What reloads the workspace

There is only one workspace, so anything that loads a different object
replaces what you were looking at:

- `objNew()` — leaves the *new* object in the workspace.
- `objGetPointer()` — leaves the object you asked for in the workspace.
- `objKill()` — calls `objGetPointer` internally, so it reloads it too.

So a callback may peek at another object: the workspace tracks its
**owner** — the slot it mirrors. Every reload first flushes the current contents to that
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

## Positions are 24-bit fixed point

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

## Who owns which field

| Field | Written by |
|---|---|
| `prev`, `next`, `nID`, `type` | engine only — do not touch |
| `xpos`, `ypos` | engine (`objNew`, `objUpdateXY`, collision snapping) and you |
| `xvel`, `yvel` | you; collision zeroes them on impact and applies gravity/friction |
| `width`, `height`, `xofs`, `yofs` | you, in the init callback |
| `xmin`, `xmax` | you (conventionally the patrol range from the `.o16`) |
| `tilestand`, `tilesprop`, `tilebprop` | collision routines |
| `onscreen` | `objUpdateAll` |
| `action` | you (plus the collision routines, which set `ACT_BURN` and `ACT_DIE`) |
| `sprnum`, `sprframe`, `sprflip`, `sprpal`, `sprid3216`, `sprblk3216` | you |
| `count`, `dir`, `tempo`, `hitpoints`, `parentID`, `status`, `tileabove`, `sprrefresh` | you — the engine never reads or writes these |

The last row matters: despite their Doxygen comments, `tileabove`, `status`
and `sprrefresh` are never touched by the engine. They are free storage.
## Which objects get updated

`objUpdateAll` does not call every live object's callback. It first tests the
object against a **virtual screen** — roughly one screen plus 64 pixels of
margin on each side, relative to the map camera (`-64` to `320` in X, `-64`
to `288` in Y). Objects outside that box are skipped entirely: they do not
move, do not fall, do not animate. This is the engine's culling strategy, and
it is the reason enemies far off-screen cost nothing.

A second, tighter test (`-32` to `256` X, `-32` to `224` Y) maintains the
`onscreen` flag. That flag is what `objRefreshAll()` filters on.

Two consequences worth internalising:

- The camera used is the `map` module's (`mapGetCameraX()` / `mapGetCameraY()`). Without the map
  module scrolling, they stay at the origin and the window is fixed there.
- Iteration order is type 0 first, then type 1, and so on; within a type, the
  most recently created object is visited first, because `objNew` pushes onto
  the head of that type's list.

## The sprite refresh pass

`objRefreshAll()` is a second pass that calls each **on-screen** object's
*refresh* callback. It exists so a scrolling game can separate "think" from
"draw" and avoid sprite flicker at the screen edge. Run it after
`objUpdateAll()` — it depends on the `onscreen` flags that pass sets.

Neither shipped example uses it; both do their drawing inline in the update
callback and register a null refresh pointer, which the engine skips.

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

## Slopes, and what the tests pin

`objCollidMapWithSlopes` is what Mario uses in `examples/games/mapandobjects`.
A slope tile is not a wall: walking into one must *lift* the object to the
surface height for its horizontal position within the tile, not stop it. The
routine does that by indexing a per-slope-type height table with the object's
centre X relative to the tile.

The six slope properties, as `map.h` names them, are a 1×1 slope ascending
(`T_SLOPEU1`) and descending (`T_SLOPED1`) to the right, plus the lower and
upper halves of a 2×1 slope in each direction (`T_SLOPELU2`, `T_SLOPELD2`,
`T_SLOPEUU2`, `T_SLOPEUD2`). You assign them per tile in the map's attribute
layer.

The functional test `testing/manifests/movement_mapandobjects_slope.toml`
pins exactly the distinction that matters, measured on luna with RIGHT held:

| Frame | Position | What it proves |
|---|---|---|
| 60 | `(32, 96)` | spawned and settled on flat ground |
| 120 | `(99, 96)` | 67 pixels along the flat — Y untouched |
| 144 | `(127, 89)` | on the slope: 28 pixels right, lifted 7 pixels up |
| 216 | `(209, 96)` | back on flat ground past the slope |
| 300 | `(209, 96)` | RIGHT still held, but a solid tile stops him dead |

Frames 144 and 300 are the two halves of the contract: a slope lifts, a wall
stops. The companion manifest `movement_mapandobjects.toml` pins the
flat-ground case the same way — walking right and left along a floor with
Y constant at 96, which is the "he never falls through the floor" negative.

One practical note from the same example: Mario is declared with
`width = 14` and `xofs = 1` rather than a full 16. The comment in `mario.c`
says a width of 16 misbehaves on slopes. Leave a pixel of slack on each side
of a 16-pixel sprite.

## See also

- @ref tutorial_object "Object Engine Tutorial" — registering, spawning, the frame, removing, gotchas.
- @ref tutorial_far_ram "Far RAM" — why the records and the workspace are `FAR`.
- @ref tutorial_map "Scrolling maps" — the camera and the metatile property table.
- @ref object.h "Object API reference" · @ref map.h "Map API reference"
