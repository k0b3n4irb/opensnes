# likemario

> A side-scrolling platformer with gravity, tile collision, and a camera that follows
> the player across a world bigger than the screen: Kit, the fox-kit courier, runs
> over the rooftops of the city at night. This is the real deal.

![Screenshot](likemario.png)

## Controls

| Button | Action |
|--------|--------|
| D-Pad Left/Right | Walk |
| A | Jump (hold Up + A for higher jump) |

## What You'll Learn

- How tile streaming works — loading new map columns into VRAM as the camera scrolls
- Fixed-point physics: gravity, acceleration, and sub-pixel movement on a CPU with no multiply
- Tile-based collision detection with O(1) lookups (no sprite-to-sprite checks)
- The double-buffer pattern: stage data in RAM during active display, flush to VRAM during VBlank
- Why the camera position and the scroll register are two different things

---

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/games/likemario
```

Then open `likemario.sfc` in luna (`luna-gui likemario.sfc`) or any emulator.

## Walkthrough

### 1. The World Is Bigger Than the Screen

The SNES screen is 256 pixels wide — 32 tiles. But the rooftop level is 2352 pixels,
294 tiles wide. The PPU's tilemap is only 32x32 entries, so you can't load the entire world
at once. Instead, the game streams new tile columns into VRAM as the camera scrolls.

Think of the tilemap as a circular buffer. As the camera moves right, the leftmost
column scrolls off-screen. That column's VRAM slot gets overwritten with the next
column from the map. The player never sees the swap because it happens off-screen.

### 2. The VRAM Layout

```c
#define VRAM_SPR_LARGE  0x0000   // Kit's 32x32 frames (the dynamic engine's pool)
#define VRAM_BG2_TILES  0x1000   // The skyline's tiles (the engine's small pool, unused here)
#define VRAM_BG_TILES   0x2000   // The level's 802 tiles, up to $5220
#define VRAM_BG_MAP     0x6800   // BG1 tilemap (64x32)
#define VRAM_BG2_MAP    0x7000   // BG2 tilemap (64x32)
```

The level's tileset (the pixel art for every unique tile) lives at `$2000`, its
tilemap (which tile goes where) at `$6800`. Kit's frames are streamed to `$0000`.
The dynamic sprite engine keeps a second pool at `$1000` for small sprites; this game
draws none, so the far skyline of BG2 takes the room (155 tiles: its two halves are
mirrors, and the converter's `flip` stores the second as flip bits).

### 3. Streaming: The Column Pipeline

Streaming happens in three stages, split across the frame:

**During active display** (CPU is free, PPU is drawing):
```c
static void map_prepare_column(u16 map_col, u16 vram_col) {
    /* Fill col_buffer[32] from map data — RAM only, no VRAM access */
    for (row = 0; row < map_height; row++) {
        col_buffer[row] = map_row_ptrs[row][map_col];
    }
    /* Note the transfer: 64 bytes, the address stepping by 32 words
     * (down the column) */
    vramQueuePush((const u8 *)col_buffer, vram_addr, 64, VRAM_QUEUE_COLUMN);
}
```

**During VBlank** (PPU is idle, VRAM is writable), `vramQueueFlush()` sends
what was noted, one DMA per entry (the `vramqueue` module).

**The decision logic** (every frame):
```c
static void map_update(void) {
    tile_x = camera_x >> 3;
    if (tile_x != last_tile_x) {
        /* Camera moved — stream the column 32 tiles ahead */
        new_col = tile_x + 32;
        map_prepare_column(new_col, new_col & 31);
        last_tile_x = tile_x;
    }
}
```

> **Why +32, not +31?** The screen shows tiles 0-31, but tile 32 is partially visible
> at the right edge (the camera rarely sits on an exact tile boundary). Without that
> extra column, you'd see a blank strip flickering at the right side during scrolling.
> This was a bug in the original port that took a while to track down.

### 4. The Level and the Skyline

The level is a row of buildings, each made of one to three roof pieces of 128x128
pixels at one height, with gaps between them. `devtools/art_assets/build.py` lays it
out from the generated pieces in `assets/neo-lumen/tiles/` and writes the four files
this example reads: `res/tiles.png` (the 802 distinct 8x8 tiles, in the order the map
numbers them), `res/BG1.m16` (the map), `res/map_1_1.b16` (solid or empty, per tile)
and `res/skyline.png`. A roof's walkable surface is the top of its neon strip, so a
tile is solid when it sits at the surface's row or below; the script refuses a tile
that would be solid in one place and empty in another. Each step up is at most 16
pixels and each gap at most 24, so the normal jump crosses them all
(`testing/manifests/playthrough_likemario.toml` plays the level to its end).

BG2 is the band of far towers, scrolled at a quarter of the camera's speed:

```c
bgSetScroll(1, camera_x >> 2, 0);   /* the far towers: a quarter of the speed */
```

### 5. Fixed-Point Physics

The 65816 has no multiply instruction and runs at 3.58 MHz. You can't afford
floating-point anything. Instead, positions and velocities use **8-bit fixed-point**:
the high byte is the pixel position, the low byte is the fractional part (1/256th
of a pixel).

```c
static s16 kit_xvel;     /* Velocity: units of 1/256 pixel per frame */
static u8  kit_xfrac;    /* Fractional pixel accumulator */

/* Apply velocity */
new_frac = (s16)kit_xfrac + kit_xvel;
kit_x += asr8(new_frac);     /* Integer part → pixel position */
kit_xfrac = new_frac & 0xFF; /* Fractional part → carry forward */
```

At 60fps, a velocity of `0x0140` (= 1.25 pixels/frame) moves Kit at 75 pixels per
second. The fractional accumulator means Kit can move at speeds finer than 1 pixel
per frame — essential for smooth deceleration.

> **What's `asr8()`?** Arithmetic shift right by 8 — dividing by 256 while
> preserving the sign, written out with an unsigned shift and two inversions so
> the intent is visible. cc65816's `>>` on a signed value IS arithmetic (the
> compiler checks pin it: `devtools/compiler-tests/cases/test_signed_ops.c`), so
> `val >> 8` would do the same; the helper is readability, not a workaround.

### 6. Gravity in Two Lines

```c
kit_yvel += GRAVITY;                /* Accelerate downward every frame */
if (kit_yvel > 0x0400) kit_yvel = 0x0400;  /* Terminal velocity */
```

`GRAVITY = 48` (in 1/256 pixel/frame units). That's roughly 0.19 pixels/frame of
acceleration — these constants come from the PVSnesLib example this one began as,
and they feel right for a platformer.

Jumping sets a negative Y velocity:
```c
if (padPressed(0) & KEY_A) {
    if (padHeld(0) & KEY_UP)
        kit_yvel = -KIT_HIJUMPING;  /* -0x0594: high jump */
    else
        kit_yvel = -KIT_JUMPING;    /* -0x0394: normal jump */
}
```

### 7. Tile-Based Collision

No bounding boxes, no sprite lists. The world is made of tiles, and collision is a
lookup:

```c
static u8 map_get_tile_prop(s16 px, s16 py) {
    tx = px >> 3;
    ty = py >> 3;
    if (tx < 0 || ty < 0 || tx >= map_width || ty >= map_height)
        return T_SOLID;  /* Out of bounds = wall */
    tile_id = map_row_ptrs[ty][tx] & 0x03FF;  /* Mask off flip/palette bits */
    return tile_props[tile_id];
}
```

`map_row_ptrs[]` is precomputed at load time — one pointer per row, so looking up a
tile is just an array index. No multiplication, no searching. O(1).

To check if Kit is standing on ground, test two points at his feet:

```c
left  = map_get_tile_prop(kit_x + 2,  kit_y + 16);
right = map_get_tile_prop(kit_x + 13, kit_y + 16);
if (left == T_SOLID || right == T_SOLID) {
    kit_y = ((kit_y + 16) & 0xFFF8) - 16;  /* Snap to tile grid */
    kit_yvel = 0;
    kit_action = ACT_STAND;
}
```

Two points, not one, because the collision box is 16 pixels wide. Testing only the
center would let Kit hang off ledges with half his body in mid-air. The box is
smaller than his 32x32 frame: the sprite is drawn 8 pixels left and 14 up of it
(`KIT_DRAW_DX`, `KIT_DRAW_DY`), so his feet, on row 30 of the frame, are the box's
bottom.

### 8. The Main Loop

One frame, one iteration. Game logic runs during active display, VRAM writes during VBlank:

```c
while (1) {
    /* Active display — game logic (no VRAM access) */
    kit_handle_input();
    kit_apply_physics();
    kit_collide_vertical();
    kit_collide_horizontal();
    kit_clamp_and_transition();
    kit_animate();
    kit_update_camera();
    map_update();                        /* Stage column in RAM */

    /* VBlank — hardware updates */
    WaitForVBlank();
    vramQueueFlush();                    /* Send the staged column to VRAM */
    bgSetScroll(0, camera_x, 0);        /* Update scroll register */
    /* NMI handler auto-flushes the dynamic sprite engine
     * (end-frame + VRAM tile queue) — no manual calls needed. */
}
```

The split is strict: everything above `WaitForVBlank()` touches only RAM.
Everything below it writes to hardware. Break this rule and you get visual corruption.

---

## Tips & Tricks

- **Kit falls through the floor?** Check your collision points. If `kit_y + 16`
  overshoots by even one pixel, the tile lookup returns the row below the floor — empty
  space — and Kit keeps falling.

- **Tiles missing at the right edge?** Your streaming offset is wrong. It must be
  `tile_x + 32`, not `tile_x + 31`. The 33rd tile is partially visible.

- **Kit slides after stopping?** That's intentional — deceleration. The velocity
  decreases by a fixed amount each frame until it reaches zero. Remove the deceleration
  code and Kit stops instantly (feels stiff).

- **Scrolling tears or flickers?** `bgSetScroll()` must happen during VBlank. If it's
  called during active display, the top and bottom halves of the screen scroll by
  different amounts for one frame.

---

## Go Further

- **Add enemies:** Use a second sprite with its own collision box. Check overlap with
  Kit each frame. Start simple — a robot insect (`assets/neo-lumen/characters/insect.png`)
  that crawls left until it hits a wall.

- **Add data shards:** Mark certain tiles as collectible in `tile_props[]`. When Kit touches
  one, replace the tilemap entry with an empty tile and increment a counter.

- **More sound:** the music and the jump go through SNESMOD (`res/rooftops.it`,
  `res/jump_sfx.it`). Add a landing or a coin effect the same way — see
  [SNESMOD Music](../../audio/snesmod_music/) for the pattern, and for the
  room the effects need beside the music.

- **Study the physics:** Change `GRAVITY`, `KIT_ACCEL`, and `KIT_JUMPING`. Small
  tweaks completely change how the game feels. This is how real platformers are tuned.

---

## Under the Hood: The Build

### The Makefile

```makefile
TARGET      := likemario.sfc
CSRC        := main.c
ASMSRC      := data.asm
USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background
```

### Asset Conversion

The Makefile has no conversion rule: each picture carries its import settings
in a file beside it, and the build runs the right tool before compiling.

```toml
# res/tiles.png.toml — the background tileset (opensnes-tileset)
tool = "opensnes-tileset"

[convert]
colors = 16

# res/skyline.png.toml — BG2, palette 1; flip folds the mirrored half, pages lays
# the 64-wide map out as two 32x32 screens
tool = "opensnes-tileset"

[convert]
colors = 16
palette-entry = 1
flip = true
pages = true

# res/kit.png.toml — Kit's sheet (opensnes-sprite), 32x32 frames
tool = "opensnes-sprite"

[sheet]
size = 32
colors = 16
```

The tools write the `.pic` / `.pal` (and the `.map` for the tileset), a
`res/<name>_data.as` the build links, and a `res/<name>.inc` that declares
`<name>_tiles[]`, `<name>_pal[]` (each with `_end`) — `main.c` includes it.

### Why So Many Modules?

| Module | Why it's here |
|--------|--------------|
| `console` | PPU init, NMI handler, `WaitForVBlank()` |
| `sprite` | OAM buffer for Kit |
| `sprite_dynamic` | VRAM queue for animated sprite frames |
| `sprite_lut` | Tile offset calculations for the dynamic engine |
| `dma` | Bulk transfers: tile data, palettes, OAM buffer |
| `input` | `padHeld()`, `padPressed()` — NMI handler fills the buffers |
| `background` | `bgSetScroll()`, `bgSetGfxPtr()`, `bgInitTileSet()` |
| `anim` | Kit's idle, walk, jump and fall clips |
| `vramqueue` | The map column, noted during the frame and sent in VBlank |

### Data Placement

Every asset sits in an `ASSET_SECTION` (`templates/assets.inc`): the linker
puts it in the asset banks, never in bank $00, which is kept for code. The
converted graphics get theirs from the generated `res/<name>_data.as`; the
map and collision data, which no tool converts, keep a hand-written one in
`data.asm`:

```asm
ASSET_SECTION "rodata2"
maplevel:       .incbin "res/BG1.m16"
tilesetatt:     .incbin "res/map_1_1.b16"
.ends
```

The map and collision data are read by C too, and that works from any bank:
the C side declares them `const`, and every C read of const data is a far
read.

---

## Modules Used

`console`, `sprite`, `sprite_dynamic`, `sprite_lut`, `dma`, `input`, `background`, `anim`, `vramqueue` (`LIB_MODULES` in the Makefile).

## Technical Reference

| Register | Address | Role in this example |
|----------|---------|---------------------|
| BGMODE   | $2105   | Mode 1 (BG1 the level, BG2 the skyline, both 4bpp) |
| BG1SC    | $2107   | BG1 tilemap at $6800 |
| BG12NBA  | $210B   | BG1 tiles at $2000 |
| BG1HOFS  | $210D   | Horizontal scroll (= camera_x) |
| VMAIN    | $2115   | VRAM increment mode ($81 = vertical for column writes) |
| BG2SC    | $2108   | BG2 tilemap at $7000 |
| TM       | $212C   | Enable OBJ + BG1 + BG2 |
| INIDISP  | $2100   | Force blank during init |

## Files

| File | What's in it |
|------|-------------|
| `main.c` | All game logic: physics, collision, streaming, camera (~493 lines) |
| `res/*.png.toml` | the import settings of the tileset, the skyline and the sprite sheet; the build converts and links them |
| `data.asm` | ROM assets no tool converts: map data, collision table |
| `res/tiles.png` | the level's 802 distinct 8x8 tiles, in map order (written by `devtools/art_assets/build.py`) |
| `res/skyline.png` | the far towers of BG2, 512x256 (written by the same script) |
| `res/kit.png` | Kit seen from the side, 32x32 frames: idle, walk, run, jump |
| `res/BG1.m16` | the level map: width, height, size, then one entry per 8x8 cell |
| `res/map_1_1.b16` | per-tile collision properties (solid/empty) |
| `res/rooftops.it` | The music, "Neo-Lumen Rooftops" (Impulse Tracker), composed for OpenSNES (`devtools/audio_assets`, CC0) |
| `res/jump_sfx.it` | The jump effect: Kenney's `phaseJump1` (kenney.nl, CC0) as a one-instrument module |
| `Makefile` | `LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background` |

## Credits

- The engine began as alekmaul's PVSnesLib example.
- The roofs, the skyline and Kit: generated for OpenSNES with PixelLab, CC0
  (`assets/neo-lumen/README.md` gives the seeds).
