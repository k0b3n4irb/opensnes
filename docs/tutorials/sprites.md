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
    oamInit();

    // Enable sprites on main screen
    REG_TM = TM_OBJ;

    setScreenOn();

    // ... game loop
}
```

### Setting a Sprite

```c
// oamSet(id, x, y, priority, hflip, vflip, palette)
oamSet(0, 100, 80, 0, 0, 0, 0);  // Sprite 0 at (100, 80)
oamSet(1, 120, 80, 0, 0, 0, 1);  // Sprite 1 with palette 1
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

// Hide all sprites
oamInit();
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

    // Update sprite with horizontal flip based on direction
    oamSet(0, player_x, player_y, 0, !facing_right, 0, 0);
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

See `examples/input/2_two_players/` for a complete example with two independently controlled sprites.

## Performance Tips

1. **Minimize oamUpdate() calls** - Only call once per frame
2. **Use sprite pooling** - Reuse sprite slots instead of creating/destroying
3. **Check sprite limits** - Max 32 sprites per scanline, 128 total
4. **Batch similar sprites** - Group sprites using same tiles/palettes

## Next Steps

- @ref tutorial_input "Controller Input"
- @ref sprite.h "Sprite API Reference"
