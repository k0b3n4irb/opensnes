# OpenSNES Example Walkthroughs

This guide provides detailed explanations of each example project, breaking down the code line by line to help you understand SNES programming concepts.

---

## Table of Contents

1. [Hello World](#1-hello-world) - Text display using background tiles
2. [Sprite Display](#2-sprite-display) - Show a 16x16 sprite
3. [Two-Player Input](#3-two-player-input) - Move sprites with controllers
4. [SNESMOD Music](#4-snesmod-music) - Tracker music playback

---

## 1. Hello World

**Location:** `examples/text/hello_world/`

This example displays "HELLO WORLD!" on screen using background tiles. It demonstrates the fundamental concepts of SNES graphics.

### Key Concepts

- Mode 0 backgrounds (2-bit per pixel, 4 colors)
- Loading tiles to VRAM
- Setting up a tilemap
- Palette configuration

### Code Walkthrough

#### Hardware Register Setup

```c
/* Set up Mode 0 (4 BG layers, all 2bpp) */
REG_BGMODE = 0x00;

/* BG1 tilemap at word $0400 */
REG_BG1SC = 0x04;

/* BG1 tiles at word $0000 */
REG_BG12NBA = 0x00;
```

**What's happening:**
- `REG_BGMODE = 0x00` selects Mode 0, which gives us 4 background layers, each with 4 colors
- `REG_BG1SC = 0x04` places the tilemap at VRAM address $0400 (word address)
- `REG_BG12NBA = 0x00` places BG1's tile graphics at VRAM address $0000

#### Loading Tile Graphics

```c
REG_VMAIN = 0x80;  /* Auto-increment after writing high byte */
REG_VMADDL = 0x00;
REG_VMADDH = 0x00;

for (i = 0; i < 144; i += 2) {
    REG_VMDATAL = font_tiles[i];
    REG_VMDATAH = font_tiles[i + 1];
}
```

**What's happening:**
- `REG_VMAIN = 0x80` sets VRAM to increment after writing the high byte
- We write 16 bytes per tile (8x8 pixels, 2bpp format)
- Each tile row is 2 bytes: bitplane 0 and bitplane 1

#### Setting the Palette

```c
REG_CGADD = 0;
REG_CGDATA = 0x00; REG_CGDATA = 0x28;  /* Color 0: Dark blue */
REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* Color 1: White */
```

**What's happening:**
- Colors are 15-bit: `0BBBBBGGGGGRRRRR`
- Color 0 ($2800) = RGB(0, 0, 10) = dark blue
- Color 1 ($7FFF) = RGB(31, 31, 31) = white

#### Writing the Tilemap

```c
/* Write message at row 14, column 10 */
addr = 0x05CA;  /* $0400 + 14*32 + 10 */
REG_VMADDL = addr & 0xFF;
REG_VMADDH = addr >> 8;

while (message[i] != 0xFF) {
    REG_VMDATAL = message[i];  /* Tile index */
    REG_VMDATAH = 0;           /* Attributes (palette 0) */
    i++;
}
```

**What's happening:**
- Tilemap entries are 2 bytes: tile index (low) + attributes (high)
- Screen is 32 tiles wide, so row 14, column 10 = offset 14*32 + 10

#### Enabling Display

```c
REG_TM = 0x01;       /* Enable BG1 on main screen */
REG_INIDISP = 0x0F;  /* Full brightness, screen on */
```

### Try It Yourself

1. Modify the font tiles to add new characters
2. Change the background color (color 0)
3. Move the text to a different position
4. Add a second line of text

---

## 2. Sprite Display

**Location:** `examples/graphics/sprites/simple_sprite/`

This example shows a 16x16 pixel sprite on screen. It introduces OAM (Object Attribute Memory) and sprite graphics.

### Key Concepts

- Sprite graphics in VRAM
- OAM structure (low table + high table)
- Sprite palettes (CGRAM $80-$FF)
- Sprite size configuration

### OAM Structure

The SNES has 128 sprites, each described by 4 bytes in the "low table":

| Byte | Description |
|------|-------------|
| 0 | X position (bits 0-7) |
| 1 | Y position |
| 2 | Tile number |
| 3 | Attributes (vhoopppc) |

Plus a 32-byte "high table" with 2 bits per sprite:
- Bit 0: X position bit 8 (for off-screen sprites)
- Bit 1: Size select (0=small, 1=large)

### Code Walkthrough

#### Sprite Size Configuration

```c
REG_OBJSEL = 0x00;  /* Small=8x8, Large=16x16, sprites at VRAM $0000 */
```

The OBJSEL register sets sprite sizes and VRAM location:
- Bits 0-2: Name base address (x $4000)
- Bits 3-4: Name select
- Bits 5-7: Size select (0 = 8x8/16x16)

#### Loading Sprite Graphics

```c
REG_VMAIN = 0x80;
REG_VMADDL = 0x00;
REG_VMADDH = 0x00;

for (i = 0; i < sprite_TILES_SIZE; i += 2) {
    REG_VMDATAL = sprite_tiles[i];
    REG_VMDATAH = sprite_tiles[i + 1];
}
```

Sprites use 4bpp (16 colors) format: 32 bytes per 8x8 tile.

#### Loading Sprite Palette

```c
REG_CGADD = 128;  /* Sprite palettes start at color 128 */

for (i = 0; i < 16; i++) {
    REG_CGDATA = color & 0xFF;
    REG_CGDATA = (color >> 8) & 0xFF;
}
```

Sprite palettes occupy CGRAM colors 128-255 (8 palettes of 16 colors).

#### Setting Up OAM

```c
/* Sprite 0: X=120, Y=100, Tile=0, Attr=0x30 */
REG_OAMDATA = 120;   /* X position */
REG_OAMDATA = 100;   /* Y position */
REG_OAMDATA = 0;     /* Tile number */
REG_OAMDATA = 0x30;  /* vhoopppc: priority 3, palette 0 */

/* Hide unused sprites at Y=240 */
for (i = 1; i < 128; i++) {
    REG_OAMDATA = 0;
    REG_OAMDATA = 240;  /* Off-screen */
    REG_OAMDATA = 0;
    REG_OAMDATA = 0;
}

/* High table: sprite 0 = large (16x16) */
REG_OAMDATA = 0x02;  /* Bits: --ss --ss (sprite 3,2,1,0) */
```

**Attribute byte breakdown (0x30):**
- Bit 7 (v): Vertical flip = 0
- Bit 6 (h): Horizontal flip = 0
- Bits 5-4 (oo): Priority = 3 (in front of all BGs)
- Bits 3-1 (ppp): Palette = 0
- Bit 0 (c): Tile name bit 8 = 0

### Try It Yourself

1. Change the sprite position
2. Flip the sprite horizontally or vertically
3. Change the sprite's palette
4. Add multiple sprites

---

## 3. Two-Player Input

**Location:** `examples/input/two_players/`

Move sprites around the screen using the D-pad. Supports two players simultaneously.

### Key Concepts

- Auto-joypad reading
- Button state polling (both controllers)
- Boundary checking
- Real-time sprite movement for multiple players

### Controller Button Layout

```c
#define KEY_B       0x8000
#define KEY_Y       0x4000
#define KEY_SELECT  0x2000
#define KEY_START   0x1000
#define KEY_UP      0x0800
#define KEY_DOWN    0x0400
#define KEY_LEFT    0x0200
#define KEY_RIGHT   0x0100
#define KEY_A       0x0080
#define KEY_X       0x0040
#define KEY_L       0x0020
#define KEY_R       0x0010
```

### Code Walkthrough

#### Player State

Each player has their own position stored in a struct:

```c
typedef struct {
    s16 x, y;
} Player;

Player p1 = {64, 112};   /* Player 1 starts left */
Player p2 = {192, 112};  /* Player 2 starts right */
```

#### Sprite Setup (Two Palettes)

Each player gets a different colored sprite via separate palettes:

```c
/* Load sprite tile + Player 1 palette (blue) */
oamInitGfxSet((u8 *)sprite_tile, 32,
              (u8 *)pal_blue, 8,
              0, 0x0000, OBJ_SIZE8_L16);

/* Load Player 2 palette (red) into palette slot 1 */
dmaCopyCGram((u8 *)pal_red, 128 + 16, 8);

/* Set up both sprites */
oamSet(0, p1.x, p1.y, 0, 0, 0, 0);  /* Palette 0 (blue) */
oamSet(1, p2.x, p2.y, 0, 0, 0, 1);  /* Palette 1 (red)  */
```

#### Reading Both Controllers

The NMI handler reads both joypads automatically. Use `padHeld()` with the
controller index (0 or 1) to get the current button state:

```c
pad1 = padHeld(0);  /* Controller 1 */
pad2 = padHeld(1);  /* Controller 2 */
```

#### Independent Movement with Boundary Checking

Each player's D-pad input moves only their sprite:

```c
/* Player 1 movement */
if (pad1 & KEY_UP)    { if (p1.y > 0) p1.y--; }
if (pad1 & KEY_DOWN)  { if (p1.y < 224) p1.y++; }
if (pad1 & KEY_LEFT)  { if (p1.x > 0) p1.x--; }
if (pad1 & KEY_RIGHT) { if (p1.x < 248) p1.x++; }

/* Player 2 movement */
if (pad2 & KEY_UP)    { if (p2.y > 0) p2.y--; }
if (pad2 & KEY_DOWN)  { if (p2.y < 224) p2.y++; }
if (pad2 & KEY_LEFT)  { if (p2.x > 0) p2.x--; }
if (pad2 & KEY_RIGHT) { if (p2.x < 248) p2.x++; }

/* Update both sprites */
oamSet(0, p1.x, p1.y, 0, 0, 0, 0);
oamSet(1, p2.x, p2.y, 0, 0, 0, 1);
```

The boundary checks prevent sprites from wrapping around screen edges.

### Try It Yourself

1. Add a speed boost when holding A or B
2. Implement collision detection between the two sprites
3. Add a score display for a competitive mini-game
4. Make each player's sprite a different size (8x8 vs 16x16)

---

## 4. SNESMOD Music

**Location:** `examples/audio/snesmod_music/`

Play tracker music with interactive controls: play, stop, pause/resume, volume, and fade out.

### Key Concepts

- SNESMOD audio system setup
- Soundbank conversion from Impulse Tracker (.it) files
- Module loading and playback
- Per-frame audio processing
- Volume and transport controls

### Makefile Configuration

SNESMOD requires specific Makefile settings:

```makefile
# Enable SNESMOD audio system
USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input

# Soundbank source files (.it format)
SOUNDBANK_SRC  := music/pollen8.it
```

The build system converts `.it` files into a soundbank binary and generates `soundbank.h`
with constants like `SOUNDBANK_BANK` and `MOD_POLLEN8`.

### Code Walkthrough

#### Initialization

```c
#include <snes/snesmod.h>
#include "soundbank.h"

/* Initialize SNESMOD (uploads driver to SPC700) */
snesmodInit();

/* Tell SNESMOD where the soundbank lives in ROM */
snesmodSetSoundbank(SOUNDBANK_BANK);

/* Load a module from the soundbank */
snesmodLoadModule(MOD_POLLEN8);

/* Start playback (0 = from beginning) */
snesmodPlay(0);
```

**What's happening:**
- `snesmodInit()` uploads the SNESMOD driver to the SPC700's 64KB audio RAM
- `snesmodSetSoundbank()` tells the driver which ROM bank holds the converted soundbank
- `snesmodLoadModule()` transfers the module (patterns + samples) to audio RAM
- `snesmodPlay(0)` starts playback from pattern 0

#### Per-Frame Processing (Critical!)

```c
while (1) {
    WaitForVBlank();
    snesmodProcess();  /* MUST be called every frame! */
    /* ... game logic ... */
}
```

**`snesmodProcess()` is mandatory every frame.** It handles CPU↔SPC communication:
streaming pattern data, processing commands, and keeping playback in sync.
Skipping it causes audio glitches or silence.

#### Transport Controls

```c
/* Play / Stop */
snesmodPlay(0);     /* Start from beginning */
snesmodStop();      /* Stop playback */

/* Pause / Resume */
snesmodPause();
snesmodResume();
```

#### Volume Control

```c
/* Set module volume (0-127) */
snesmodSetModuleVolume(volume);

/* Fade volume over time (target, speed) */
snesmodFadeVolume(0, 4);  /* Fade to silence, speed 4 */
```

### Creating Music

1. Use [OpenMPT](https://openmpt.org/) (free) to compose in Impulse Tracker format
2. SNES supports 8 channels, 32kHz max sample rate
3. Keep total sample data under ~58KB (shares 64KB audio RAM with driver + echo buffer)
4. Export as `.it` and place in your project's `music/` directory

### Try It Yourself

1. Replace `pollen8.it` with your own tracker module
2. Add sound effects alongside music (see `snesmod_sfx` example)
3. Implement music selection with multiple modules
4. Trigger music changes based on game events

### Deep Dive: What SNESMOD Does Under the Hood

For the curious — here's what happens beneath the library calls.

The SNES has a completely separate **SPC700** audio processor:
- 8-bit CPU running at 1.024 MHz
- Custom DSP with 8 voices, hardware ADSR, echo/reverb
- 64KB dedicated audio RAM (not accessible by the main CPU)

Communication happens via 4 shared I/O ports ($2140-$2143). When you call
`snesmodInit()`, it uses the SPC700's built-in IPL boot ROM to upload the
SNESMOD driver code into audio RAM:

```
CPU writes to $2140-$2143  →  SPC700 reads from $F4-$F7
SPC700 writes to $F4-$F7   →  CPU reads from $2140-$2143
```

The driver then configures DSP registers for each voice:

```
$F2 = register address    $F3 = register value
Voice registers: VOL_L, VOL_R, PITCH_L, PITCH_H, ADSR1, ADSR2, ...
```

Audio samples use **BRR** (Bit Rate Reduction) compression:
- 9 bytes per block (1 header + 8 data bytes = 16 decoded samples)
- ~4:1 compression ratio vs raw PCM
- The DSP decompresses in real-time during playback

`snesmodProcess()` streams pattern data from ROM to the SPC700 each frame,
telling the driver which notes to play, at what pitch and volume.

---

## Common Patterns

### VBlank Synchronization

Always update graphics during VBlank to avoid visual glitches:

```c
static void wait_vblank(void) {
    while (REG_HVBJOY & 0x80) {}   /* Wait until not in VBlank */
    while (!(REG_HVBJOY & 0x80)) {}  /* Wait for VBlank to start */
}
```

### Main Loop Structure

```c
int main(void) {
    /* 1. Initialize hardware */
    setup_graphics();
    setup_audio();

    /* 2. Enable interrupts */
    REG_NMITIMEN = 0x81;

    /* 3. Turn on screen */
    REG_INIDISP = 0x0F;

    /* 4. Main loop */
    while (1) {
        wait_vblank();

        /* Read input */
        joy = read_joypad();

        /* Update game state */
        update_game();

        /* Update graphics */
        update_sprites();
    }
}
```

### Hiding Unused Sprites

Always hide unused sprites at Y=240 (off-screen):

```c
for (i = used_sprites; i < 128; i++) {
    oam[i].y = 240;
}
```

---

## Building Examples

Each example has a Makefile:

```bash
cd examples/text/hello_world
make
# Creates hello_world.sfc

# Run in emulator
open hello_world.sfc  # macOS
# or: mesen hello_world.sfc
```

## Next Steps

After understanding these examples:

1. **Combine concepts**: Add sprites to your text demo
2. **Add scrolling**: Move background layers
3. **Add animation**: Cycle through sprite tiles
4. **Add collision**: Detect sprite-sprite overlaps
5. **Add music**: Use a tracker format converter

See `docs/GETTING_STARTED.md` for more advanced topics.
