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
    │     │ ↓ │                │ B │        │
    │     └───┘                └───┘        │
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

The NMI handler reads both pads during VBlank, every frame. Three calls give
you that reading; each takes the pad number (0 or 1):

| Call | Bits set for buttons that are |
|---|---|
| `padHeld(pad)` | down now |
| `padPressed(pad)` | down now and up last frame (fires once per press) |
| `padReleased(pad)` | up now and down last frame |

```c
#include <snes.h>

while (1) {
    WaitForVBlank();

    if (padPressed(0) & KEY_A) {
        player_jump();          /* once per press */
    }
    if (padHeld(0) & KEY_RIGHT) {
        player_x++;             /* every frame while held */
    }
}
```

Use `padPressed()` for one-shot actions (jump, menu choice) and `padHeld()`
for continuous ones (movement). An unplugged pad reads as no button down;
`padIsConnected()` tells the two apart (see below).

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

/* in the main loop: */
update_player(padHeld(0));
```

## Two-Player Example

```c
u16 p1_x = 64, p1_y = 112;
u16 p2_x = 192, p2_y = 112;

while (1) {
    u16 pad1 = padHeld(0);
    u16 pad2 = padHeld(1);

    if (pad1 & KEY_LEFT)  { if (p1_x > 0) p1_x--; }
    if (pad1 & KEY_RIGHT) { if (p1_x < 248) p1_x++; }
    if (pad2 & KEY_LEFT)  { if (p2_x > 0) p2_x--; }
    if (pad2 & KEY_RIGHT) { if (p2_x < 248) p2_x++; }

    oamSet(0, p1_x, p1_y, 0, 0, 0, 0);  // Player 1
    oamSet(1, p2_x, p2_y, 0, 1, 0, 0);  // Player 2 (palette 1)
    WaitForVBlank();
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

if (padPressed(0) & KEY_UP) {
    if (menu_selection > 0) {
        menu_selection--;
    }
}
if (padPressed(0) & KEY_DOWN) {
    if (menu_selection < MENU_ITEMS - 1) {
        menu_selection++;
    }
}
if (padPressed(0) & KEY_A) {
    // Confirm selection
    activate_menu_item(menu_selection);
}
if (padPressed(0) & KEY_B) {
    // Cancel / go back
    close_menu();
}
```

## Is a Pad Plugged In?

An empty port and an idle pad both read as "no button pressed". To tell
them apart, `padIsConnected(pad)` returns 1 when a standard pad answered
on that port during the last VBlank:

```c
if (!padIsConnected(1)) {
    textPrintAt(4, 20, "PLUG IN CONTROLLER 2");
}
```

Pads are 0 and 1; a mouse or a Super Scope on the port reads 0 (it is not
a pad). The answer is valid from the first VBlank after `consoleInit()`.

## The SNES Mouse

`mouseInit(port)` looks for a mouse on port 0 or 1 and, if it finds one,
turns on mouse reading in the NMI handler. It reads the result of the
automatic joypad read, so call it after a `WaitForVBlank()`, as
`examples/input/mouse` does:

```c
s16 cursor_x = 128, cursor_y = 112;

WaitForVBlank();
if (!mouseInit(0)) {
    textPrintAt(2, 5, "No mouse detected.");
}

while (1) {
    WaitForVBlank();
    cursor_x += mouseGetX(0);        /* signed motion since last frame */
    cursor_y += mouseGetY(0);        /* positive = down */
    if (mouseButtonsPressed(0) & MOUSE_BUTTON_LEFT) {
        /* click: pressed this frame */
    }
    if (mouseButtonsHeld(0) & MOUSE_BUTTON_RIGHT) {
        /* right button is down */
    }
}
```

The mouse reports motion, not a position: keep your own cursor and clamp
it to the screen. `mouseGetX()` / `mouseGetY()` return -127..+127.
`mouseIsConnected(port)` says whether the NMI handler still sees the
mouse this frame (it can be unplugged mid-game).

Sensitivity has three steps (`MOUSE_SENS_LOW`, `MOUSE_SENS_MEDIUM`,
`MOUSE_SENS_HIGH`). `mouseSetSensitivity(port, level)` queues the change,
which the NMI handler applies; `mouseGetSensitivity(port)` returns the
level in effect, so it may lag one frame behind the request.

## The Super Scope

The Super Scope is a light gun on port 2 only. When it sees the screen's
light it makes the PPU latch its horizontal and vertical counters (a
transition on pin 6 of the second controller connector — fullsnes, PPU
timers and status; anomie's register doc, `$213C`/`$213D`). The NMI
handler reads that latch and the gun's buttons every frame once
`scopeInit()` has found it:

```c
WaitForVBlank();
if (scopeInit()) {
    /* ask the player to shoot the centre of the screen, then: */
    scopeCalibrate();
}

while (1) {
    WaitForVBlank();
    if (scopeButtonsPressed() & SSC_FIRE) {
        u16 x = scopeGetX();         /* 0-255, calibrated */
        u16 y = scopeGetY();         /* 0-223 */
        /* hit test at (x, y) */
    }
}
```

The raw counter values (`scopeGetRawX()`, `scopeGetRawY()`) are not
screen coordinates: the visible picture starts part-way along the
horizontal counter, and every gun aims a little differently.
`scopeCalibrate()` takes the last shot as the screen centre (128, 112)
and offsets every later reading; `scopeGetX()` / `scopeGetY()` return the
corrected position. `examples/input/superscope` shows the whole
detect / calibrate / fire loop.

Buttons come as a mask of `SSC_FIRE`, `SSC_CURSOR`, `SSC_TURBO`,
`SSC_PAUSE`, plus two status flags, `SSC_OFFSCREEN` (aimed off the TV)
and `SSC_NOISE`:

| Call | Returns |
|---|---|
| `scopeButtonsHeld()` | buttons down now (like `padHeld()`) |
| `scopeButtonsPressed()` | buttons that went down this frame |
| `scopeButtonsRepeat()` | a held button re-triggering: first after the hold delay, then every repeat delay |
| `scopeSinceShot()` | frames since the gun last latched a position |

`scopeSetHoldDelay(frames)` (default 60) and `scopeSetRepeatDelay(frames)`
(default 20) tune the auto-repeat. `scopeIsConnected()` returns whether
`scopeInit()` found the gun, and drops to 0 if the gun is unplugged; a gun
plugged in later is not detected on its own, so call
`scopeInit()` again (for example once per frame on a "connect the Super
Scope" screen) to notice a gun plugged in later.

## Hardware Registers

The library reads the pads for you; the registers are listed for reference
and for porting code that reads them directly:

| Register | Address | Description |
|----------|---------|-------------|
| REG_JOY1L | $4218 | Controller 1, low byte |
| REG_JOY1H | $4219 | Controller 1, high byte |
| REG_JOY2L | $421A | Controller 2, low byte |
| REG_JOY2H | $421B | Controller 2, high byte |
| REG_HVBJOY | $4212 | Status (bit 0 = auto-read busy) |

## Tips

1. **`padPressed()` for menus and one-shot actions**: it fires once per press.
2. **`padHeld()` for movement**: true on every frame the button is down.
3. **`padIsConnected()` before asking for a second player**: an empty port
   reads as an idle pad.

## Example

See `examples/input/controller/` for both pads on one screen, with the held, pressed and released buttons.

## Next Steps

- @ref tutorial_audio "Audio & Music"
- @ref input.h "Input API Reference"
