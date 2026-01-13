# OpenSNES Example Walkthroughs

This guide provides detailed explanations of each example project, breaking down the code line by line to help you understand SNES programming concepts.

---

## Table of Contents

1. [Hello World](#1-hello-world) - Text display using background tiles
2. [Sprite Display](#2-sprite-display) - Show a 16x16 sprite
3. [Joypad Input](#3-joypad-input) - Move a sprite with the controller
4. [Audio Playback](#4-audio-playback) - Play sounds with button press

---

## 1. Hello World

**Location:** `examples/text/1_hello_world/`

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

**Location:** `examples/graphics/1_sprite/`

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

## 3. Joypad Input

**Location:** `examples/input/1_joypad/`

Move a sprite around the screen using the D-pad. Hold A or B for faster movement.

### Key Concepts

- Auto-joypad reading
- Button state polling
- Boundary checking
- Real-time sprite movement

### Controller Button Layout

```c
#define JOY_B       0x8000
#define JOY_Y       0x4000
#define JOY_SELECT  0x2000
#define JOY_START   0x1000
#define JOY_UP      0x0800
#define JOY_DOWN    0x0400
#define JOY_LEFT    0x0200
#define JOY_RIGHT   0x0100
#define JOY_A       0x0080
#define JOY_X       0x0040
#define JOY_L       0x0020
#define JOY_R       0x0010
```

### Code Walkthrough

#### Enabling Auto-Joypad

```c
REG_NMITIMEN = 0x81;  /* Enable NMI + auto-joypad */
```

The SNES automatically reads all controllers during VBlank when bit 0 is set.

#### Reading Controller State

```c
static u16 read_joypad(void) {
    /* Wait for auto-read to complete */
    while (REG_HVBJOY & 0x01) {}
    return REG_JOY1L | (REG_JOY1H << 8);
}
```

**Important:** Always wait for auto-read to complete before reading JOY1L/H!

#### Movement with Boundary Checking

```c
if (joy & JOY_UP) {
    if (sprite_y > speed) {
        sprite_y = sprite_y - speed;
    } else {
        sprite_y = 0;
    }
}
```

This prevents the sprite from wrapping around when it reaches the screen edge.

#### Variable Speed

```c
speed = 1;
if (joy & (JOY_A | JOY_B)) {
    speed = 3;
}
```

Holding A or B triples the movement speed.

### Try It Yourself

1. Add acceleration (gradual speed increase)
2. Implement edge wrapping instead of boundary clamping
3. Add a second sprite controlled by buttons
4. Implement 8-way diagonal movement

---

## 4. Audio Playback

**Location:** `examples/audio/1_tone/`

Press A to play a "tada" sound effect. Demonstrates SPC700 audio programming.

### Key Concepts

- SPC700 IPL boot protocol
- Uploading code/data to audio RAM
- BRR sample format
- DSP voice configuration
- CPU-SPC communication

### SPC700 Architecture

The SNES has a separate audio processor:
- **SPC700**: 8-bit CPU running at 1.024 MHz
- **DSP**: Digital Signal Processor with 8 voices
- **64KB Audio RAM**: Holds driver code, samples, and echo buffer

### Communication Protocol

The CPU and SPC communicate via 4 I/O ports ($2140-$2143):

```c
#define REG_APUIO0 (*(volatile u8*)0x2140)
```

This example uses an "echo" protocol:
1. SPC continuously echoes port0 values
2. Write 0x55 to trigger sound
3. Wait for echo confirmation
4. Clear to 0x00

### Code Walkthrough

#### SPC Initialization

```c
spc_wait_ready();  /* Wait for IPL to be ready */

/* Upload driver, directory, and sample */
spc_upload(0x0200, spc_driver, sizeof(spc_driver));
spc_upload(0x0300, sample_dir, sizeof(sample_dir));
spc_upload(0x0400, &tada_brr_start, 8739);

spc_execute(0x0200);  /* Start driver */
```

#### DSP Voice Setup (in driver)

The SPC driver configures DSP registers:

```asm
; Voice 0 volume
mov $F2, #$00     ; V0VOLL register
mov $F3, #$7F     ; Max volume

; Pitch (sample playback rate)
mov $F2, #$02     ; V0PITCHL
mov $F3, #$40
mov $F2, #$03     ; V0PITCHH
mov $F3, #$04

; ADSR envelope
mov $F2, #$05     ; V0ADSR1
mov $F3, #$FF     ; Attack/Decay
mov $F2, #$06     ; V0ADSR2
mov $F3, #$E0     ; Sustain/Release
```

#### Playing Sound

```c
/* Detect A button press */
if ((joy & JOY_A) && !a_was_pressed) {
    REG_APUIO0 = 0x55;              /* Send play command */
    while (REG_APUIO0 != 0x55) {}   /* Wait for echo */
    REG_APUIO0 = 0x00;              /* Clear command */
    while (REG_APUIO0 != 0x00) {}
}
```

### BRR Sample Format

BRR (Bit Rate Reduction) is the SNES's compressed audio format:
- 9 bytes per block (1 header + 8 data bytes)
- 16 samples per block
- 4:1 compression ratio

### Try It Yourself

1. Change the playback pitch
2. Add a second sample
3. Implement volume control
4. Create a simple melody

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
cd examples/text/1_hello_world
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
