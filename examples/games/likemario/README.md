# LikeMario

> A side-scrolling platformer with gravity, tile collision, and a camera that follows
> the player across a world bigger than the screen. This is the real deal.

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

Then open `likemario.sfc` in your emulator (Mesen2 recommended).

## Walkthrough

### 1. The World Is Bigger Than the Screen

The SNES screen is 256 pixels wide — 32 tiles. But the LikeMario map is hundreds of
tiles wide. The PPU's tilemap is only 32x32 entries, so you can't load the entire world
at once. Instead, the game streams new tile columns into VRAM as the camera scrolls.

Think of the tilemap as a circular buffer. As the camera moves right, the leftmost
column scrolls off-screen. That column's VRAM slot gets overwritten with the next
column from the map. The player never sees the swap because it happens off-screen.

### 2. The VRAM Layout

```c
#define VRAM_SPR_LARGE  0x0000   // Sprite graphics
#define VRAM_SPR_SMALL  0x1000   // Sprite graphics (small)
#define VRAM_BG_TILES   0x2000   // Background tileset
#define VRAM_BG_MAP     0x6800   // Background tilemap (32x32)
```

The tileset (the pixel art for every unique tile) lives at `$2000`. The tilemap
(which tile goes where) lives at `$6800`. Sprites get `$0000`-`$1FFF`. This layout
avoids overlaps and leaves room for the 32x32 tilemap to wrap around.

### 3. Streaming: The Column Pipeline

Streaming happens in three stages, split across the frame:

**During active display** (CPU is free, PPU is drawing):
```c
static void map_prepare_column(u16 map_col, u16 vram_col) {
    /* Fill col_buffer[32] from map data — RAM only, no VRAM access */
    for (row = 0; row < map_height; row++) {
        col_buffer[row] = map_row_ptrs[row][map_col];
    }
    col_pending = 1;  /* Flag: "I have data ready" */
}
```

**During VBlank** (PPU is idle, VRAM is writable):
```c
static void map_flush_column(void) {
    if (!col_pending) return;
    REG_VMAIN = 0x81;  /* Auto-increment vertically (down the column) */
    /* Write col_buffer to VRAM */
    col_pending = 0;
}
```

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

### 4. Fixed-Point Physics

The 65816 has no multiply instruction and runs at 3.58 MHz. You can't afford
floating-point anything. Instead, positions and velocities use **8-bit fixed-point**:
the high byte is the pixel position, the low byte is the fractional part (1/256th
of a pixel).

```c
static s16 mario_xvel;     /* Velocity: units of 1/256 pixel per frame */
static u8  mario_xfrac;    /* Fractional pixel accumulator */

/* Apply velocity */
new_frac = (s16)mario_xfrac + mario_xvel;
mario_x += asr8(new_frac);     /* Integer part → pixel position */
mario_xfrac = new_frac & 0xFF; /* Fractional part → carry forward */
```

At 60fps, a velocity of `0x0140` (= 1.25 pixels/frame) moves Mario at 75 pixels per
second. The fractional accumulator means Mario can move at speeds finer than 1 pixel
per frame — essential for smooth deceleration.

> **What's `asr8()`?** Arithmetic shift right by 8 — dividing by 256 while
> preserving the sign, written out with an unsigned shift and two inversions so
> the intent is visible. cc65816's `>>` on a signed value IS arithmetic (the
> compiler checks pin it: `devtools/compiler-tests/cases/test_signed_ops.c`), so
> `val >> 8` would do the same; the helper is readability, not a workaround.
> (Until 2026-10-04 this box claimed the compiler shifted logically.)

### 5. Gravity in Two Lines

```c
mario_yvel += GRAVITY;                /* Accelerate downward every frame */
if (mario_yvel > 0x0400) mario_yvel = 0x0400;  /* Terminal velocity */
```

`GRAVITY = 48` (in 1/256 pixel/frame units). That's roughly 0.19 pixels/frame of
acceleration — feels like a Mario game because these constants were tuned by PVSnesLib
to match the original's feel.

Jumping sets a negative Y velocity:
```c
if (padPressed(0) & KEY_A) {
    if (padHeld(0) & KEY_UP)
        mario_yvel = -MARIO_HIJUMPING;  /* -0x0594: high jump */
    else
        mario_yvel = -MARIO_JUMPING;    /* -0x0394: normal jump */
}
```

### 6. Tile-Based Collision

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

To check if Mario is standing on ground, test two points at his feet:

```c
left  = map_get_tile_prop(mario_x + 2,  mario_y + 16);
right = map_get_tile_prop(mario_x + 13, mario_y + 16);
if (left == T_SOLID || right == T_SOLID) {
    mario_y = ((mario_y + 16) & 0xFFF8) - 16;  /* Snap to tile grid */
    mario_yvel = 0;
    mario_action = ACT_STAND;
}
```

Two points, not one, because Mario is 16 pixels wide. Testing only the center would
let him hang off ledges with half his body in mid-air.

### 7. The Main Loop

One frame, one iteration. Game logic runs during active display, VRAM writes during VBlank:

```c
while (1) {
    /* Active display — game logic (no VRAM access) */
    mario_handle_input();
    mario_apply_physics();
    mario_collide_vertical();
    mario_collide_horizontal();
    mario_clamp_and_transition();
    mario_animate();
    mario_update_camera();
    map_update();                        /* Stage column in RAM */

    /* VBlank — hardware updates */
    WaitForVBlank();
    map_flush_column();                  /* Write staged column to VRAM */
    bgSetScroll(0, camera_x, 0);        /* Update scroll register */
    /* NMI handler auto-flushes the dynamic sprite engine
     * (end-frame + VRAM tile queue) — no manual calls needed. */
}
```

The split is strict: everything above `WaitForVBlank()` touches only RAM.
Everything below it writes to hardware. Break this rule and you get visual corruption.

---

## Tips & Tricks

- **Mario falls through the floor?** Check your collision points. If `mario_y + 16`
  overshoots by even one pixel, the tile lookup returns the row below the floor — empty
  space — and Mario keeps falling.

- **Tiles missing at the right edge?** Your streaming offset is wrong. It must be
  `tile_x + 32`, not `tile_x + 31`. The 33rd tile is partially visible.

- **Mario slides after stopping?** That's intentional — deceleration. The velocity
  decreases by a fixed amount each frame until it reaches zero. Remove the deceleration
  code and Mario stops instantly (feels stiff).

- **Scrolling tears or flickers?** `bgSetScroll()` must happen during VBlank. If it's
  called during active display, the top and bottom halves of the screen scroll by
  different amounts for one frame.

---

## Go Further

- **Add enemies:** Use a second sprite with its own collision box. Check overlap with
  Mario each frame. Start simple — a Goomba that walks left until it hits a wall.

- **Add coins:** Mark certain tiles as collectible in `tile_props[]`. When Mario touches
  one, replace the tilemap entry with an empty tile and increment a counter.

- **Add sound:** The assets include `mariojump.brr` (a jump sound effect) and
  `overworld.it` (music). Wire them up with SNESMOD — see
  [SNESMOD Music](../../audio/snesmod_music/) for the pattern.

- **Study the physics:** Change `GRAVITY`, `MARIO_ACCEL`, and `MARIO_JUMPING`. Small
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

# res/mario_sprite.png.toml — the sprite sheet (opensnes-sprite)
tool = "opensnes-sprite"

[sheet]
size = 16
colors = 16
```

The tools write the `.pic` / `.pal` (and the `.map` for the tileset), a
`res/<name>_data.as` the build links, and a `res/<name>.inc` that declares
`<name>_tiles[]`, `<name>_pal[]` (each with `_end`) — `main.c` includes it.

### Why So Many Modules?

| Module | Why it's here |
|--------|--------------|
| `console` | PPU init, NMI handler, `WaitForVBlank()` |
| `sprite` | OAM buffer for Mario |
| `sprite_dynamic` | VRAM queue for animated sprite frames |
| `sprite_lut` | Tile offset calculations for the dynamic engine |
| `dma` | Bulk transfers: tile data, palettes, OAM buffer |
| `input` | `padHeld()`, `padPressed()` — NMI handler fills the buffers |
| `background` | `bgSetScroll()`, `bgSetGfxPtr()`, `bgInitTileSet()` |

### Data Placement

Every asset sits in an `ASSET_SECTION` (`templates/assets.inc`): the linker
puts it in the asset banks, never in bank $00, which is kept for code. The
converted graphics get theirs from the generated `res/<name>_data.as`; the
map and collision data, which no tool converts, keep a hand-written one in
`data.asm`:

```asm
ASSET_SECTION "rodata2"
mapmario:       .incbin "res/BG1.m16"
tilesetatt:     .incbin "res/map_1_1.b16"
.ends
```

The map and collision data are read by C too, and that works from any bank:
the C side declares them `const`, and every C read of const data is a far
read (since v0.41.0). Until 2026-09-23 they sat in `SEMIFREE BANK 0` "because
the compiler reads bank $00" — true before far pointers, and what this page
said until 2026-09-26.

---

## Modules Used

`console`, `sprite`, `sprite_dynamic`, `sprite_lut`, `dma`, `input`, `background`, `anim` (`LIB_MODULES` in the Makefile).

## Technical Reference

| Register | Address | Role in this example |
|----------|---------|---------------------|
| BGMODE   | $2105   | Mode 1 (BG1 4bpp for the world) |
| BG1SC    | $2107   | BG1 tilemap at $6800 |
| BG12NBA  | $210B   | BG1 tiles at $2000 |
| BG1HOFS  | $210D   | Horizontal scroll (= camera_x) |
| VMAIN    | $2115   | VRAM increment mode ($81 = vertical for column writes) |
| TM       | $212C   | Enable OBJ + BG1 |
| INIDISP  | $2100   | Force blank during init |

## Files

| File | What's in it |
|------|-------------|
| `main.c` | All game logic: physics, collision, streaming, camera (~493 lines) |
| `res/*.png.toml` | the import settings of the tileset and the sprite sheet; the build converts and links them |
| `data.asm` | ROM assets no tool converts: map data, collision table |
| `res/tiles.png` | Background tileset source (8x8 tiles) |
| `res/mario_sprite.png` | Sprite sheet source (16x16 frames) |
| `res/BG1.m16` | World tilemap (tile indices + flip/palette bits) |
| `res/map_1_1.b16` | Per-tile collision properties (solid/empty) |
| `res/overworld.it` | Music file (Impulse Tracker, not yet wired up) |
| `res/mariojump.brr` | Jump sound effect (BRR, not yet wired up) |
| `Makefile` | `LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background` |

## Credits

- Original: alekmaul (PVSnesLib example)
- Tileset and sprites: community assets
