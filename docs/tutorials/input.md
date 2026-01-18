# Controller Input Tutorial {#tutorial_input}

This tutorial covers reading SNES controllers including button detection and multi-player support.

## SNES Controller Layout

```
        L                              R
    ┌───────────────────────────────────────┐
    │     ┌───┐                   ┌───┐     │
    │     │ ↑ │              (X)  │ X │     │
    │ ┌───┼───┼───┐      (Y)   (A)├───┤     │
    │ │ ← │   │ → │ [SEL][STA] │ Y │ A │    │
    │ └───┼───┼───┘            ├───┼───┤    │
    │     │ ↓ │                │ B │       │
    │     └───┘                └───┘       │
    └───────────────────────────────────────┘
```

## Button Constants

OpenSNES defines these button masks in `input.h`:

```c
// D-Pad
#define KEY_UP      0x0800
#define KEY_DOWN    0x0400
#define KEY_LEFT    0x0200
#define KEY_RIGHT   0x0100

// Face Buttons
#define KEY_A       0x0080
#define KEY_B       0x8000
#define KEY_X       0x0040
#define KEY_Y       0x4000

// Shoulder Buttons
#define KEY_L       0x0020
#define KEY_R       0x0010

// Start/Select
#define KEY_START   0x1000
#define KEY_SELECT  0x2000
```

## Reading Controllers

### Direct Register Access (Recommended)

```c
#include <snes.h>

u16 pad;

while (1) {
    WaitForVBlank();

    // Wait for auto-joypad read to complete
    while (REG_HVBJOY & 0x01) {}

    // Read controller 1
    pad = REG_JOY1L | (REG_JOY1H << 8);

    // Check for disconnected controller
    if (pad == 0xFFFF) {
        // No controller connected
        continue;
    }

    // Check buttons
    if (pad & KEY_A) {
        // A button pressed
    }
    if (pad & KEY_UP) {
        // D-pad up pressed
    }
}
```

### Reading Controller 2

```c
u16 pad1, pad2;

// Wait for auto-joypad
while (REG_HVBJOY & 0x01) {}

// Read both controllers
pad1 = REG_JOY1L | (REG_JOY1H << 8);
pad2 = REG_JOY2L | (REG_JOY2H << 8);
```

## Edge Detection

Detect button presses (not just held state):

```c
u16 pad_current;
u16 pad_previous = 0;
u16 pad_pressed;

while (1) {
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}

    // Read current state
    pad_current = REG_JOY1L | (REG_JOY1H << 8);

    // Detect new presses (down this frame, not last frame)
    pad_pressed = pad_current & ~pad_previous;

    // Save for next frame
    pad_previous = pad_current;

    // Skip if disconnected
    if (pad_current == 0xFFFF) {
        pad_pressed = 0;
    }

    // Now use pad_pressed for one-shot actions
    if (pad_pressed & KEY_A) {
        // A was just pressed (fires once)
        player_jump();
    }

    // Use pad_current for continuous actions
    if (pad_current & KEY_RIGHT) {
        // Right is held (fires every frame)
        player_x++;
    }
}
```

## Movement Example

```c
u16 player_x = 128;
u16 player_y = 112;

void update_player(u16 pad) {
    // Movement with boundary checking
    if (pad & KEY_UP) {
        if (player_y > 0) player_y--;
    }
    if (pad & KEY_DOWN) {
        if (player_y < 224) player_y++;
    }
    if (pad & KEY_LEFT) {
        if (player_x > 0) player_x--;
    }
    if (pad & KEY_RIGHT) {
        if (player_x < 248) player_x++;
    }
}
```

## Two-Player Example

```c
// Player positions
u16 p1_x = 64, p1_y = 112;
u16 p2_x = 192, p2_y = 112;

while (1) {
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}

    // Read both controllers
    u16 pad1 = REG_JOY1L | (REG_JOY1H << 8);
    u16 pad2 = REG_JOY2L | (REG_JOY2H << 8);

    // Player 1 movement
    if (pad1 != 0xFFFF) {
        if (pad1 & KEY_UP)    { if (p1_y > 0) p1_y--; }
        if (pad1 & KEY_DOWN)  { if (p1_y < 224) p1_y++; }
        if (pad1 & KEY_LEFT)  { if (p1_x > 0) p1_x--; }
        if (pad1 & KEY_RIGHT) { if (p1_x < 248) p1_x++; }
    }

    // Player 2 movement
    if (pad2 != 0xFFFF) {
        if (pad2 & KEY_UP)    { if (p2_y > 0) p2_y--; }
        if (pad2 & KEY_DOWN)  { if (p2_y < 224) p2_y++; }
        if (pad2 & KEY_LEFT)  { if (p2_x > 0) p2_x--; }
        if (pad2 & KEY_RIGHT) { if (p2_x < 248) p2_x++; }
    }

    // Update sprites
    oamSet(0, p1_x, p1_y, 0, 0, 0, 0);  // Player 1
    oamSet(1, p2_x, p2_y, 0, 0, 0, 1);  // Player 2
    oamUpdate();
}
```

## Button Combinations

```c
// Check for multiple buttons
if ((pad & KEY_A) && (pad & KEY_B)) {
    // A + B pressed together
}

// Check for button + direction
if ((pad & KEY_A) && (pad & KEY_DOWN)) {
    // Down + A (e.g., slide attack)
}

// Create combo masks
#define COMBO_HADOUKEN (KEY_DOWN | KEY_RIGHT | KEY_A)
if ((pad & COMBO_HADOUKEN) == COMBO_HADOUKEN) {
    // All combo buttons pressed
}
```

## Menu Navigation

```c
u8 menu_selection = 0;
const u8 MENU_ITEMS = 4;

if (pad_pressed & KEY_UP) {
    if (menu_selection > 0) {
        menu_selection--;
    }
}
if (pad_pressed & KEY_DOWN) {
    if (menu_selection < MENU_ITEMS - 1) {
        menu_selection++;
    }
}
if (pad_pressed & KEY_A) {
    // Confirm selection
    activate_menu_item(menu_selection);
}
if (pad_pressed & KEY_B) {
    // Cancel / go back
    close_menu();
}
```

## Hardware Registers

For reference, the joypad registers:

| Register | Address | Description |
|----------|---------|-------------|
| REG_JOY1L | $4218 | Controller 1, low byte |
| REG_JOY1H | $4219 | Controller 1, high byte |
| REG_JOY2L | $421A | Controller 2, low byte |
| REG_JOY2H | $421B | Controller 2, high byte |
| REG_JOY3L | $421C | Controller 3 (multitap) |
| REG_JOY3H | $421D | Controller 3 (multitap) |
| REG_JOY4L | $421E | Controller 4 (multitap) |
| REG_JOY4H | $421F | Controller 4 (multitap) |
| REG_HVBJOY | $4212 | Status (bit 0 = auto-read busy) |

## Tips

1. **Always wait for auto-joypad** - Check `REG_HVBJOY & 0x01` before reading
2. **Check for disconnected controllers** - `0xFFFF` means no controller
3. **Use edge detection for menus** - Prevents rapid-fire selection
4. **Use held state for movement** - Smoother continuous motion

## Example

See `examples/input/2_two_players/` for a complete two-player demo.

## Next Steps

- @ref tutorial_audio "Audio & Music"
- @ref input.h "Input API Reference"
