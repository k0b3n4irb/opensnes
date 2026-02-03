# Calculator Example

A working 4-function calculator demonstrating SNES programming with user input.

## Learning Objectives

After this lesson, you will understand:
- Handling joypad input with edge detection
- Managing application state in C
- Implementing cursor-based UI
- 16-bit integer arithmetic on the SNES
- Working around compiler limitations

## Prerequisites

- Completed [1. Hello World](../../text/1_hello_world/)
- Completed [2. Custom Font](../../text/2_custom_font/)
- Understanding of tile-based graphics

---

## What This Example Does

An interactive calculator with:
- 16 on-screen buttons (0-9, +, -, *, /, C, =)
- Cursor navigation with D-pad
- 5-digit display (0-65535)
- Full 4-function arithmetic

```
+----------------------------------------+
|                                        |
|          CALCULATOR                    |
|                                        |
|             00008                      |
|                                        |
|        [7]  8   9   /                  |
|         4   5   6   *                  |
|         1   2   3   -                  |
|         0   C   =   +                  |
|                                        |
+----------------------------------------+
```

---

## Code Type

**Mixed C + Direct Register Access**

| Component | Type |
|-----------|------|
| Hardware init | Library (`consoleInit()`) |
| Video mode | Library (`setMode()`) |
| Screen enable | Library (`setScreenOn()`) |
| VBlank sync | Library (`WaitForVBlank()`) |
| Input reading | Direct register access |
| Font tiles | Embedded C array (25 tiles) |
| Arithmetic | Software implementation (inlined) |

---

## Controls

| Input | Action |
|-------|--------|
| D-pad Up/Down | Move cursor between rows |
| D-pad Left/Right | Move cursor between columns |
| A Button | Press the selected button |

---

## Key Implementation Details

### 1. Edge Detection for Input

The calculator tracks both current and previous button states to detect new presses:

```c
u16 pad_prev;
u16 pad;
u16 pad_pressed;

/* In main loop */
pad = REG_JOY1L | (REG_JOY1H << 8);
pad_pressed = pad & ~pad_prev;  /* Bits set only on new press */
pad_prev = pad;

if (pad_pressed & KEY_A) {
    press_button();  /* Only fires once per press */
}
```

### 2. Button Grid Layout

The 16 buttons are arranged in a 4x4 grid:

```c
static const u8 button_tiles[16] = {
    TILE_7, TILE_8, TILE_9, TILE_DIV,  /* Row 0 */
    TILE_4, TILE_5, TILE_6, TILE_MUL,  /* Row 1 */
    TILE_1, TILE_2, TILE_3, TILE_MINUS,/* Row 2 */
    TILE_0, TILE_C, TILE_EQ, TILE_PLUS /* Row 3 */
};

/* Convert cursor position to array index */
pos = cursor_y * 4 + cursor_x;
```

### 3. Software Arithmetic

Due to compiler issues with the `__mul16` and `__div16` runtime functions, arithmetic is implemented with basic operations only:

**Multiplication** (repeated addition):
```c
while (b > 0) {
    result = result + a;
    b = b - 1;
}
```

**Division** (repeated subtraction):
```c
while (a >= b) {
    a = a - b;
    result = result + 1;
}
```

**Multiply by 10** (for digit entry, using shifts):
```c
/* x*10 = x*8 + x*2 = (x<<3) + (x<<1) */
display_value = (val << 3) + (val << 1) + digit;
```

### 4. Digit Extraction Without Division

To display numbers, digits are extracted using repeated subtraction:

```c
d4 = 0;
while (val >= 10000) { val = val - 10000; d4++; }
d3 = 0;
while (val >= 1000) { val = val - 1000; d3++; }
d2 = 0;
while (val >= 100) { val = val - 100; d2++; }
d1 = 0;
while (val >= 10) { val = val - 10; d1++; }
d0 = val;  /* Ones digit is the remainder */
```

### 5. Cursor Rendering

The cursor is shown using bracket tiles around the selected button:

```c
static void draw_cursor(u8 show) {
    u8 bx, by;
    bx = BTN_START_X + cursor_x * BTN_SPACE;
    by = BTN_START_Y + cursor_y * 2;
    if (show) {
        write_tile(bx - 1, by, TILE_LBRACK);  /* [ */
        write_tile(bx + 1, by, TILE_RBRACK);  /* ] */
    } else {
        write_tile(bx - 1, by, TILE_SPACE);
        write_tile(bx + 1, by, TILE_SPACE);
    }
}
```

---

## Calculator State Machine

```c
static u16 display_value;  /* Current visible number */
static u16 accumulator;    /* Stored value for pending operation */
static u8 pending_op;      /* 0=none, 1=+, 2=-, 3=*, 4=/ */
static u8 new_number;      /* Flag: next digit starts fresh */
```

**Flow:**
1. Enter digits -> updates `display_value`
2. Press operator -> stores `display_value` in `accumulator`, sets `pending_op`
3. Enter more digits -> updates `display_value`
4. Press = -> performs `accumulator OP display_value`, shows result

---

## Build and Run

```bash
cd examples/basics/1_calculator
make clean && make

# Run in emulator
/path/to/Mesen calculator.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Complete calculator implementation (513 lines) |
| `Makefile` | Build configuration |
| `calc_loop.asm` | Legacy file (not used) |
| `debug.lua` | Mesen debugging script |
| `test_calc.lua` | Automated test script |

---

## VRAM Memory Map

```
$0000-$0190: Font tiles (25 tiles x 16 bytes = 400 bytes)
$0400-$07FF: BG1 tilemap (32x32 = 1024 words)
```

---

## Exercises

### Exercise 1: Add Memory Functions
Implement M+, M-, MR (memory recall) buttons.

**Hint:** Add a `memory` variable and new button values.

### Exercise 2: Negative Numbers
Support negative results and display a minus sign.

**Hint:** Use a signed type and add a TILE_MINUS display case.

### Exercise 3: Decimal Point
Add a decimal point button for fixed-point arithmetic.

**Hint:** Track decimal places separately and adjust display logic.

---

## Technical Notes

### Why Software Arithmetic?

The OpenSNES compiler (cproc+QBE) generates calls to runtime functions for multiplication and division:
- `__mul16` for 16-bit multiply
- `__div16` for 16-bit divide

These functions exist in `templates/common/runtime.asm` but had issues being linked properly (WLA-DX treats underscore-prefixed labels as section-local). While this was fixed, the generated code can still produce incorrect results for some operations.

The workaround is to use only addition, subtraction, and bit shifts which compile correctly.

### Input Timing

The SNES reads controller data during VBlank. We must:
1. Wait for VBlank (`WaitForVBlank()`)
2. Wait for auto-read to complete (`while (REG_HVBJOY & 0x01) {}`)
3. Only then read `REG_JOY1L/H`

---

## What's Next?

Now that you can handle input and state, explore:

**Graphics:** [Animation Example](../../graphics/2_animation/) - Moving sprites

**Audio:** [Tone Example](../../audio/1_tone/) - SPC700 audio

---

## License

MIT
