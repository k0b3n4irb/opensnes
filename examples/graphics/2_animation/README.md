# Animation Example

A movable, animated sprite demonstrating character animation on the SNES.

## Learning Objectives

After this lesson, you will understand:
- How sprite animation works on SNES
- Tile swapping for animation frames
- Animation state machines (walk direction)
- Horizontal sprite flipping
- Mixing C and assembly for performance

## Prerequisites

- Understanding of OAM and tile loading
- Completed text examples (for graphics basics)

---

## What This Example Does

A 16x16 monster sprite that:
- Moves with D-pad in any direction
- Animates while moving (3 frames per direction)
- Flips horizontally when moving left
- Uses different animation rows for up/down/left-right

```
+----------------------------------------+
|                                        |
|                                        |
|                                        |
|               [SPRITE]                 |
|                                        |
|                                        |
|                                        |
+----------------------------------------+
```

**Controls:**
- D-pad: Move the monster
- Sprite animates while moving

---

## Code Type

**Mixed C + Assembly**

| Component | Type |
|-----------|------|
| Main loop | C (`main.c`) |
| Input handling | C (direct register reads) |
| Sprite update | Library (`oamSet`, `oamUpdate`) |
| Game state | Assembly variables (`helpers.asm`) |
| Tile calculation | Assembly (avoids multiplication) |
| Graphics loading | C (direct VRAM writes) |

---

## Animation System

### Sprite Sheet Layout

The monster has 9 animation frames in a 3x3 grid:

```
Row 0 (state 0): Walk Down   - frames 0, 2, 4
Row 1 (state 1): Walk Up     - frames 6, 8, 10
Row 2 (state 2): Walk Right  - frames 12, 14, 32
```

### Tile Lookup Table

Instead of runtime multiplication, a lookup table is used:

```asm
sprite_tiles:
    .db 0, 2, 4      ; Walk down frames (state 0)
    .db 6, 8, 10     ; Walk up frames (state 1)
    .db 12, 14, 32   ; Walk right/left frames (state 2)
```

### State Variables

```asm
.RAMSECTION ".game_vars" BANK 0 SLOT 1
    monster_x       dsb 2    ; 16-bit X position
    monster_y       dsb 2    ; 16-bit Y position
    monster_state   dsb 1    ; 0=down, 1=up, 2=left/right
    monster_anim    dsb 1    ; Animation frame (0-2)
    monster_flipx   dsb 1    ; Horizontal flip flag
    monster_tile    dsb 1    ; Current tile number
.ENDS
```

---

## How Animation Works

### 1. Input Updates State

```c
if (pad & KEY_DOWN) {
    monster_state = 0;  /* Walk down */
    monster_flipx = 0;
    monster_y++;
}
if (pad & KEY_LEFT) {
    monster_state = 2;  /* Walk sideways */
    monster_flipx = 1;  /* Flip horizontally */
    monster_x--;
}
```

### 2. Animation Advances on Input

```c
/* Advance animation on any input */
monster_anim = monster_anim + 1;
if (monster_anim >= 3) {
    monster_anim = 0;
}
```

### 3. Tile Calculated from State + Frame

```asm
; tile = sprite_tiles[state * 3 + anim]
calc_tile:
    lda.l monster_state
    asl a               ; state * 2
    clc
    adc.l monster_state ; state * 3
    clc
    adc.l monster_anim  ; + anim
    tax
    lda.l sprite_tiles,x
    sta.l monster_tile
```

### 4. OAM Updated with Flip Flag

```c
u8 flags = monster_flipx ? 0x40 : 0x00;
oamSet(0, monster_x, monster_y, monster_tile, 0, 3, flags);
```

---

## Horizontal Flipping

Instead of separate left-facing graphics, the right-facing frames are flipped:

```c
/* OAM attribute bit 6 = horizontal flip */
u8 flags = monster_flipx ? 0x40 : 0x00;
```

**OAM Attribute Byte:**
```
vhppccct
||||||+- Tile high bit
|||||+-- Unused
||||+--- Palette (0-7)
||++---- Priority (0-3)
|+------ Horizontal flip
+------- Vertical flip
```

---

## Loading Graphics

### Sprite Tiles to VRAM

```c
static void load_sprite_tiles(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < sprites_TILES_SIZE; i++) {
        u8 byte = sprites_tiles[i];
        if ((i & 1) == 0) {
            REG_VMDATAL = byte;
        } else {
            REG_VMDATAH = byte;
        }
    }
}
```

### Sprite Palette to CGRAM

```c
static void load_sprite_palette(void) {
    u8 i;
    const u8 *pal_bytes = (const u8 *)sprites_pal;

    REG_CGADD = 128;  /* Sprite palettes start at 128 */

    for (i = 0; i < sprites_PAL_COUNT * 2; i++) {
        REG_CGDATA = pal_bytes[i];
    }
}
```

---

## Main Loop

```c
while (1) {
    /* Handle input (reads joypad, updates state) */
    handle_input();

    /* Update sprite in OAM buffer */
    update_sprite();

    /* Wait for VBlank and transfer OAM */
    WaitForVBlank();
    oamUpdate();
}
```

---

## Build and Run

```bash
cd examples/graphics/2_animation
make clean && make

# Run in emulator
/path/to/Mesen animation.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | C code for main loop, input, graphics loading |
| `helpers.asm` | Assembly: game state variables, tile lookup |
| `Makefile` | Build configuration |
| `assets/sprites.bmp` | Source sprite sheet |
| `sprites.h` | Generated tile and palette data |

---

## Why Mixed C + Assembly?

**Assembly is used for:**
1. **RAM variables** - Direct control over memory layout
2. **Tile calculation** - Avoids compiler-generated multiplication which can have issues

**C is used for:**
1. **Main loop logic** - Easier to read and modify
2. **Input handling** - Simple conditionals
3. **Graphics loading** - Straightforward loops

---

## Exercises

### Exercise 1: Speed Control
Make the character move faster when holding the B button.

### Exercise 2: Animation Speed
Currently animation advances every frame of input. Add a timer to slow it down:
```c
static u8 anim_timer = 0;
anim_timer++;
if (anim_timer >= 4) {
    anim_timer = 0;
    monster_anim = (monster_anim + 1) % 3;
}
```

### Exercise 3: Add a Jump
Make the character jump when pressing A:
1. Add `monster_vy` for vertical velocity
2. On A press, set `monster_vy = -4`
3. Each frame, add gravity and update position
4. Stop when hitting the ground

### Exercise 4: Create Your Own Animation
1. Edit `assets/sprites.bmp` with new graphics
2. Keep the same layout (16x16 tiles in a grid)
3. Rebuild with `make clean && make`

---

## Technical Notes

### Sprite Sizing

Initialized with `oamInitEx(OBJ_SIZE16_L32, 0)`:
- Small sprites: 16x16 pixels
- Large sprites: 32x32 pixels
- OAM table at address 0

### Screen Setup

```c
REG_TM = 0x10;       /* Enable sprites on main screen */
REG_INIDISP = 0x0F;  /* Full brightness */
```

---

## Sprite Credit

Sprite artwork by Stephen "Redshrike" Challener from [OpenGameArt.org](http://opengameart.org).

---

## What's Next?

**Audio:** [Tone Example](../../audio/1_tone/) - SPC700 sound

**Text:** [Calculator Example](../../basics/1_calculator/) - Interactive UI

---

## License

Art: CC-BY 3.0 (see OpenGameArt)
Code: MIT
