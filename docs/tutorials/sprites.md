# Sprites & Animation Tutorial {#tutorial_sprites}

This tutorial covers SNES sprites (OBJ layer) including OAM management and animation.

## SNES Sprite Basics

- Up to **128 sprites** on screen
- Sizes: 8x8, 16x16, 32x32, 64x64 (two sizes per mode)
- 4bpp (16 colors per palette)
- 8 palettes available (palettes 8-15 in CGRAM)
- Stored in **OAM** (Object Attribute Memory) - 544 bytes

## OAM Structure

Each sprite uses 4 bytes in main OAM + 2 bits in high table:

**Main OAM (4 bytes per sprite):**
| Byte | Content |
|------|---------|
| 0 | X position (low 8 bits) |
| 1 | Y position |
| 2 | Tile number (low 8 bits) |
| 3 | Attributes: vhoopppN |

**Attributes:**
- v = Vertical flip
- h = Horizontal flip
- oo = Priority (0-3)
- ppp = Palette (0-7, maps to CGRAM 128-255)
- N = Tile number bit 8

## Using OpenSNES Sprite Functions

### Initialize OAM

```c
#include <snes.h>

int main(void) {
    consoleInit();

    // Initialize OAM (hides all sprites)
    oamInit(OAM_DEFAULT_SIZE, OAM_DEFAULT_TILE_BASE);

    // Enable sprites on main screen
    REG_TM = TM_OBJ;

    setScreenOn();

    // ... game loop
}
```

### Setting a Sprite

```c
// oamSet(id, x, y, tile, palette, priority, flags)
oamSet(0, 100, 80, 0, 0, 0, 0);  // Sprite 0 at (100, 80), tile 0
oamSet(1, 120, 80, 0, 1, 0, 0);  // Sprite 1 with palette 1
```

### Updating OAM

```c
while (1) {
    WaitForVBlank();

    // Update sprite positions
    oamSet(0, player_x, player_y, 0, 0, 0, 0);

    // Transfer OAM buffer to hardware
    oamUpdate();
}
```

### Hiding Sprites

```c
// Hide a specific sprite (moves Y off-screen)
oamHide(5);

// Hide every sprite at once
oamClear();
```

Re-initialising is *not* how you hide sprites — `oamInit()` reconfigures
OBJSEL. Call `oamHide(id)` per sprite, or `oamClear()` to hide them all.

### The `name_base` argument is a page number, not a VRAM address

`oamInit(size, name_base)` takes `name_base` as a **page number 0-7**,
not a VRAM address. Each page is $2000 word addresses (16 KB), so tiles
DMA'd to word $4000 need base 2. The value is masked to 3 bits: passing
a VRAM address — the natural mistake, since every other VRAM parameter in
the SDK takes one — silently yields a wrong base (`0x6000 & 7` is 0) and
the sprites render whatever tiles sit at word 0, with no diagnostic. Use
the `OBJ_NAME_BASE(addr)` macro to convert, so the intent survives the
call:

```c
oamInit(OBJ_SIZE8_L16, OBJ_NAME_BASE(0x6000));   // base 3
```

## Loading Sprite Tiles

Sprite tiles go in VRAM (location set by REG_OBJSEL):

```c
const u8 sprite_tile[32] = {
    // 8x8 tile, 4bpp (32 bytes)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void load_sprite_tiles(void) {
    u16 i;

    // Configure sprite settings
    REG_OBJSEL = 0x00;  // 8x8/16x16 sprites, tiles at $0000

    // Set VRAM address
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    // Upload tiles
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = sprite_tile[i];
        REG_VMDATAH = sprite_tile[i + 1];
    }
}
```

### Converting a sheet: `-s` must match the sprite size

A 16x16 OBJ does not read four consecutive tiles. It reads `n`, `n+1`,
`n+16`, `n+17` — the second row comes from **16 tiles later**, because
the hardware treats the sheet as 16 tiles (128 px) wide. Same idea for
32x32 (`n`, `n+1`, `n+2`, `n+3`, `n+16` …) and 64x64.

`gfx4snes` produces exactly that layout **when you tell it the sprite
size**:

```bash
gfx4snes -s 8  -p -i cursor.png    # a sheet of 8x8 sprites
gfx4snes -s 16 -p -i hero.png      # a sheet of 16x16 sprites
gfx4snes -s 32 -p -i boss.png      # a sheet of 32x32 sprites
```

With `-s 16` the converter cuts the image into 16x16 blocks and emits
each one as the four tiles the hardware expects, whatever the sheet's
width or height.

**The trap**: converting a sheet of 16x16 frames with `-s 8`. It
succeeds, produces the right number of tiles, and the sprite renders —
wrong. The top half is one frame and the bottom half is whatever tile
happened to land 16 slots later, which depends on the PNG's width. A
sheet 16 tiles wide happens to work; make it 18 tiles wide by adding a
frame and every sprite in the game breaks at once, with no diagnostic.

The rule is one line: **`-s` is the sprite's size, not the tile size.**

`gfx4snes` cannot catch the mistake — with `-s 8` it has no way to know
you meant 16x16 sprites. It *does* warn when the image is not an exact
multiple of the block size, since the last row or column of blocks would
then be built from pixels that are not in the image.

## Sprite Palettes

Sprite palettes use CGRAM addresses 128-255:

```c
void load_sprite_palette(void) {
    // Palette 0 for sprites (CGRAM 128)
    REG_CGADD = 128;

    // Color 0: Transparent
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    // Color 1: Black
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    // Color 2: White
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;
    // Color 3: Red
    REG_CGDATA = 0x1F; REG_CGDATA = 0x00;
    // ... colors 4-15
}
```

## Animation

### Frame-based Animation

```c
u8 anim_frame = 0;
u8 anim_timer = 0;
const u8 ANIM_SPEED = 8;  // Frames per animation step

void update_animation(void) {
    anim_timer++;
    if (anim_timer >= ANIM_SPEED) {
        anim_timer = 0;
        anim_frame++;
        if (anim_frame >= 4) {  // 4 frames of animation
            anim_frame = 0;
        }
    }
}

// In main loop:
update_animation();
// Set tile based on frame (assuming tiles 0-3 are animation frames)
oamSetTile(0, anim_frame);
```

### Movement with Animation

```c
u16 player_x = 128;
u16 player_y = 112;
u8 facing_right = 1;

void update_player(u16 pad) {
    if (pad & KEY_LEFT) {
        if (player_x > 0) player_x--;
        facing_right = 0;
    }
    if (pad & KEY_RIGHT) {
        if (player_x < 248) player_x++;
        facing_right = 1;
    }

    // Update sprite with horizontal flip based on direction (flip = flags arg)
    oamSet(0, player_x, player_y, 0, 0, 0, facing_right ? 0 : OBJ_FLIPX);
}
```

## Sprite Sizes

Configure sprite sizes with REG_OBJSEL:

```c
// REG_OBJSEL: sssnnbbb
//   sss = size mode (see table)
//   nn = name select (gap between tables)
//   bbb = base address (/8192)

// Size modes:
// 0: 8x8 and 16x16
// 1: 8x8 and 32x32
// 2: 8x8 and 64x64
// 3: 16x16 and 32x32
// 4: 16x16 and 64x64
// 5: 32x32 and 64x64

REG_OBJSEL = 0x00;  // 8x8/16x16, tiles at $0000
REG_OBJSEL = 0x20;  // 8x8/32x32, tiles at $0000
REG_OBJSEL = 0x60;  // 16x16/32x32, tiles at $0000
```

## Example: Two Players

See `examples/input/two_players/` for a complete example with two independently controlled sprites.

## Performance Tips

1. **Minimize oamUpdate() calls** - Only call once per frame
2. **Use sprite pooling** - Reuse sprite slots instead of creating/destroying
3. **Check sprite limits** - Max 32 sprites per scanline, 128 total
4. **Batch similar sprites** - Group sprites using same tiles/palettes

## Next Steps

- @ref tutorial_input "Controller Input"
- @ref sprite.h "Sprite API Reference"
