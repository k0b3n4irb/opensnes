# Two Players Example

Simultaneous multi-controller input handling.

## Learning Objectives

After this lesson, you will understand:
- Reading from multiple controller ports
- Independent sprite control per player
- Controller disconnection detection
- Separate palettes for player identification

## Prerequisites

- Completed basic input examples
- Understanding of sprite basics

---

## What This Example Does

Two-player simultaneous control:
- Player 1 (blue sprite) controlled by Controller 1
- Player 2 (red sprite) controlled by Controller 2
- Each player moves independently
- Screen boundary checking for both

```
+----------------------------------------+
|                                        |
|    [P1 BLUE]          [P2 RED]         |
|                                        |
|                                        |
|                                        |
|                                        |
|   Controller 1        Controller 2     |
+----------------------------------------+
```

**Controls:**
- Controller 1 D-Pad: Move blue sprite (Player 1)
- Controller 2 D-Pad: Move red sprite (Player 2)

---

## Code Type

**C with Direct Register Access**

| Component | Type |
|-----------|------|
| Console init | Library (`consoleInit`, `setScreenOn`) |
| Sprite display | Library (`oamInit`, `oamSet`, `oamUpdate`) |
| Input reading | Direct register access |
| Boundary check | C code |

---

## Reading Two Controllers

### Controller Registers

| Register | Address | Purpose |
|----------|---------|---------|
| JOY1L | $4218 | Controller 1 low byte |
| JOY1H | $4219 | Controller 1 high byte |
| JOY2L | $421A | Controller 2 low byte |
| JOY2H | $421B | Controller 2 high byte |

### Reading Both Controllers

```c
/* Wait for auto-read completion */
while (REG_HVBJOY & 0x01) {}

/* Read controller 1 */
u16 pad1 = REG_JOY1L | (REG_JOY1H << 8);

/* Read controller 2 */
u16 pad2 = REG_JOY2L | (REG_JOY2H << 8);
```

---

## Player State

### Struct Pattern (Required)

Using a struct with `s16` coordinates is required for reliable movement.
Separate `u16` variables cause horizontal movement issues due to a compiler quirk.

```c
typedef struct {
    s16 x, y;
} Player;

Player p1 = {64, 112};
Player p2 = {192, 112};
```

---

## Player Movement

### Independent Control

```c
/* Player 1 movement */
if (pad1 & KEY_RIGHT) { if (p1.x < 248) p1.x++; }
if (pad1 & KEY_LEFT)  { if (p1.x > 0) p1.x--; }
if (pad1 & KEY_DOWN)  { if (p1.y < 224) p1.y++; }
if (pad1 & KEY_UP)    { if (p1.y > 0) p1.y--; }

/* Player 2 movement */
if (pad2 & KEY_RIGHT) { if (p2.x < 248) p2.x++; }
if (pad2 & KEY_LEFT)  { if (p2.x > 0) p2.x--; }
if (pad2 & KEY_DOWN)  { if (p2.y < 224) p2.y++; }
if (pad2 & KEY_UP)    { if (p2.y > 0) p2.y--; }
```

---

## Player Differentiation

### Separate Palettes

```c
/* Palette 0: Blue (Player 1) */
REG_CGADD = 128;
REG_CGDATA = 0xE0; REG_CGDATA = 0x7C;  /* Blue */

/* Palette 1: Red (Player 2) */
REG_CGADD = 128 + 16;
REG_CGDATA = 0x1F; REG_CGDATA = 0x00;  /* Red */
```

### Sprite Setup

```c
/* Player 1: Palette 0 (blue) */
oamSet(0, p1.x, p1.y, 0, 0, 3, 0);

/* Player 2: Palette 1 (red) */
oamSet(1, p2.x, p2.y, 0, 1, 3, 0);
```

---

## Controller Detection

### Disconnected Controller

```c
/* 0xFFFF indicates no controller connected */
if (pad2 == 0xFFFF) {
    /* Controller 2 not connected */
    /* Could display "CONNECT CONTROLLER 2" */
}
```

### Controller Types

The SNES supports various controllers:
- Standard gamepad (most common)
- Mouse
- Super Scope
- Multitap (4+ players)

---

## Build and Run

```bash
cd examples/input/2_two_players
make clean && make

# Run in emulator (enable 2nd controller)
/path/to/Mesen two_players.sfc
```

Note: Configure emulator for two controllers.

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Two-player input handling |
| `data.asm` | Sprite graphics |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Player Collision

Detect when players touch:
```c
if (abs(p1.x - p2.x) < 16 && abs(p1.y - p2.y) < 16) {
    /* Players colliding! */
}
```

### Exercise 2: Different Speeds

Give players different speeds:
```c
#define P1_SPEED 2
#define P2_SPEED 3  /* Faster */
```

### Exercise 3: Button Actions

Add button-triggered actions:
```c
if (pad1 & KEY_A) {
    /* Player 1 action */
}
if (pad2 & KEY_A) {
    /* Player 2 action */
}
```

### Exercise 4: Split Screen

Implement split-screen view for versus games (advanced):
```c
/* Use windows to divide screen */
/* Each player has their own camera */
```

---

## Technical Notes

### Auto-Joypad Read

The SNES automatically reads controllers during VBlank:
- Enabled by NMITIMEN register
- Takes ~4 scanlines
- Check HVBJOY bit 0 for completion

### Multitap Support

For more than 2 players, use the multitap:
- Requires different register access
- Up to 4-5 players depending on adapter

### Input Latency

Controller reads happen once per frame:
- 60Hz = ~16.7ms input latency
- Additional latency from game logic

### Button Layout

Both controllers have identical mapping:
```
Bit 15: B        Bit 7: A
Bit 14: Y        Bit 6: X
Bit 13: Select   Bit 5: L
Bit 12: Start    Bit 4: R
Bit 11: Up       Bits 3-0: ID (always $F)
Bit 10: Down
Bit 9:  Left
Bit 8:  Right
```

---

## What's Next?

**Collision:** [Collision Demo](../../basics/3_collision_demo/) - Hit detection

**Game:** [Breakout](../../game/1_breakout/) - Complete game

---

## License

Code: MIT
