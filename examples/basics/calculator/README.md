# Calculator

> A working 4-function calculator on a 16-bit console from 1990.
> D-pad to move, A to press buttons. Try 255 × 255.

![Screenshot](screenshot.png)

## Controls

| Button | Action |
|--------|--------|
| D-Pad | Move cursor between buttons |
| A | Press selected button |

## Build & Run

```bash
make -C examples/basics/calculator
# Open calculator.sfc in Mesen2
```

## What You'll Learn

- How to read joypad input with edge detection (press once = one action, not sixty)
- Building a cursor-based UI with background tiles
- Why the 65816 can't divide and what you do instead
- The `x*10 = (x<<3) + (x<<1)` trick that every SNES programmer knows

---

## Walkthrough

### 1. The Display Is Made of Tiles

Just like [Hello World](../../text/hello_world/), the entire interface — numbers, operators,
brackets, title — is drawn with background tiles. The font contains 25 hand-crafted
characters:

```c
#define TILE_0     1    /* Digits 0-9 */
#define TILE_PLUS  11
#define TILE_MINUS 12
#define TILE_MUL   13   /* * */
#define TILE_DIV   14   /* / */
#define TILE_EQ    15   /* = */
#define TILE_C     16   /* Clear */
#define TILE_LBRACK 17  /* [ — cursor left */
#define TILE_RBRACK 18  /* ] — cursor right */
```

The buttons are laid out in a 4×4 grid, just like a real calculator:

```c
static const u8 button_tiles[16] = {
    TILE_7, TILE_8, TILE_9, TILE_DIV,   /* Row 0 */
    TILE_4, TILE_5, TILE_6, TILE_MUL,   /* Row 1 */
    TILE_1, TILE_2, TILE_3, TILE_MINUS,  /* Row 2 */
    TILE_0, TILE_C, TILE_EQ, TILE_PLUS   /* Row 3 */
};
```

Each button is a single tile. The cursor? Two more tiles — `[` and `]` — placed on
either side of the selected button. Move the cursor, erase the old brackets, draw new ones.

### 2. Reading the Joypad (Without Going Crazy)

Here's the trap: the SNES reads joypads 60 times per second. If you check "is A pressed?"
you'll get `true` for every frame the player holds the button — that's 10+ frames for a
quick tap. Press "5" once and you get "5555555".

The fix is **edge detection**:

```c
pad = REG_JOY1L | (REG_JOY1H << 8);
pad_pressed = pad & ~pad_prev;   /* Only bits that just turned ON */
pad_prev = pad;
```

`pad_pressed` is only set on the *first* frame of a press. Hold A for a full second and
`pad_pressed` fires exactly once. This is the standard pattern for any button-driven UI
on the SNES.

> **Why read `REG_JOY1L | (REG_JOY1H << 8)` instead of using `padPressed()`?**
> Both work, but direct register reads make the timing explicit. The NMI handler reads
> joypads every frame into `pad_keys[]`, but here we want to control exactly when
> we sample input. For a calculator where timing doesn't matter much, either approach
> is fine. For a fighting game, you'd want the NMI approach.

### 3. The Cursor Dance

Moving the cursor is a two-step operation: erase the old one, draw the new one.
Skip either step and you get ghost brackets:

```c
/* Save old position */
old_x = cursor_x;
old_y = cursor_y;

/* Update position from D-pad */
if (pad_pressed & KEY_LEFT)  { if (cursor_x > 0) cursor_x--; }
if (pad_pressed & KEY_RIGHT) { if (cursor_x < 3) cursor_x++; }

/* If cursor moved: erase old, draw new */
if (cursor_x != old_x || cursor_y != old_y) {
    cursor_x = old_x; cursor_y = old_y;
    draw_cursor(0);   /* Erase: write spaces over brackets */
    /* Recalculate new position... */
    draw_cursor(1);   /* Draw: write [ ] around new button */
}
```

> **Why not just redraw every frame?** You could, but that means VRAM writes every frame
> for something that only changes when the player moves. On the SNES, VRAM writes can
> only happen during VBlank (~6300 cycles). Every write you skip is budget you keep
> for other things.

### 4. No Division? No Problem.

The 65816 CPU has no multiply or divide instruction. When you write `x * y` in C,
the compiler generates a call to `__mul16` — a software routine that does repeated
addition. It works, but it's slow.

For the calculator display, we need to extract individual digits from a number. On a
modern CPU you'd use `val / 10` and `val % 10`. On the 65816, we use repeated subtraction:

```c
d4 = 0;
while (val >= 10000) { val = val - 10000; d4++; }
d3 = 0;
while (val >= 1000)  { val = val - 1000; d3++; }
d2 = 0;
while (val >= 100)   { val = val - 100; d2++; }
d1 = 0;
while (val >= 10)    { val = val - 10; d1++; }
d0 = (u8)val;
```

For a 5-digit number, this does at most 45 iterations. A hardware divide on a modern
CPU takes 1 cycle. Welcome to 1990.

And when the user types a new digit, we need `display_value * 10 + new_digit`. Instead
of calling the multiply routine:

```c
/* x*10 = x*8 + x*2 = (x<<3) + (x<<1) */
display_value = (val << 3) + (val << 1) + digit;
```

A shift is 2 cycles. A multiply is ~50+. This trick shows up in virtually every SNES
game that displays numbers.

### 5. The State Machine

The calculator has exactly four variables:

```c
static u16 display_value;   /* What's on screen right now */
static u16 accumulator;     /* The stored value from before an operator */
static u8 pending_op;       /* Which operation is waiting (0=none, 1=+, 2=-, 3=*, 4=/) */
static u8 new_number;       /* Should the next digit start a new number? */
```

The flow is simple:
1. Type digits → builds `display_value`
2. Press an operator → saves `display_value` into `accumulator`, remembers the op
3. Type more digits → builds a new `display_value`
4. Press `=` → computes `accumulator OP display_value`, shows result

Multiplication is repeated addition, division is repeated subtraction:

```c
/* Multiply */
while (b > 0) { result = result + a; b = b - 1; }

/* Divide */
while (a >= b) { a = a - b; result = result + 1; }
```

### 6. The VBlank Gotcha

This one bit us during development. The `update_display()` function writes tiles to
VRAM — but if a multiplication takes many loop iterations (say, 255 × 255 = 65025,
that's 255 iterations), it can take longer than one frame. By the time we try to write
VRAM, we're in active display and the PPU ignores us.

The fix:

```c
static void update_display(void) {
    vblank_flag = 0;       /* Clear any stale flag */
    WaitForVBlank();       /* Wait for a fresh VBlank */
    /* Now we're safe to write VRAM */
    for (x = 0; x < 5; x++) {
        write_tile(DISPLAY_X + x, DISPLAY_Y, TILE_SPACE);
    }
    /* ... write digits ... */
}
```

Without that `vblank_flag = 0`, the function would see a stale flag from a VBlank that
already passed, return immediately, and write to VRAM during rendering. The display would
show the right answer for `8 × 3` (fast) but garble `9 × 5` (slow). Intermittent bugs
are the worst kind.

---

## Tips & Tricks

- **Display shows wrong number?** Check your digit extraction loop bounds. `val >= 10000`
  catches 5-digit numbers, but `val >= 65535` won't (that's the u16 max — there's no
  sixth digit to extract).

- **Cursor leaves trails?** You're drawing the new cursor without erasing the old one.
  Always erase first, then draw.

- **Button press fires multiple times?** You forgot the edge detection. Make sure you're
  using `pad & ~pad_prev`, not just `pad`.

- **Multiplication gives wrong result for large numbers?** `u16` wraps at 65535.
  `300 × 300 = 90000` doesn't fit — you'll get `90000 - 65536 = 24464`. That's correct
  hardware behavior, not a bug.

---

## Go Further

- **Add negative numbers:** Use bit 15 as a sign flag. Display a minus tile before the
  digits when negative.

- **Add a backspace key:** Remove the last digit by doing `display_value / 10`. Yes,
  you'll need that slow division — but only for one number at a time.

- **Make it beep:** Play a BRR sample on each button press. Check out
  [sfx_demo](../../audio/sfx_demo/) for how to trigger sounds.

- **Next example:** [Collision Demo](../collision_demo/) — basic game logic with
  sprite-to-sprite collision.

---

## Under the Hood: The Build

### The Makefile

```makefile
TARGET      := calculator.sfc
ROM_NAME    := SNES CALCULATOR
USE_LIB     := 1
LIB_MODULES := console input sprite dma
CSRC        := main.c
```

No `ASMSRC`, no graphics tools, no sound tools. Everything — the font, the UI layout,
the math logic — lives in a single `main.c`. For a project this size (~500 lines),
that's perfectly fine. The single-file approach means the build is just
`cc65816 main.c` → assemble → link → done.

### Why These Modules?

| Module | Why it's here |
|--------|--------------|
| `console` | `consoleInit()`, `WaitForVBlank()`, NMI handler setup. The foundation. |
| `input` | Declares `pad_keys[]` and `pad_keysdown[]` buffers that the NMI handler fills. Even though we read `REG_JOY1L/H` directly in this example, the NMI handler still tries to write to those buffers — without the input module, the symbols are missing. |
| `sprite` | OAM buffer. Required by console's NMI handler (it DMAs OAM every frame). |
| `dma` | DMA transfer functions. Required by sprite (OAM DMA) and used internally by console. |

> **The dependency chain matters.** `console` needs `sprite`, `sprite` needs `dma`,
> and `input` is needed for the joypad buffers the NMI handler writes to. Miss any one
> of these and the linker fails with unhelpful "undefined symbol" errors. The build
> system does not auto-resolve these — you must list them all explicitly.

### The Pipeline

```
main.c → cc65816 → main.c.asm → wla-65816 → main.c.obj → wlalink → calculator.sfc
```

The linker combines your code with: `crt0.asm` (CPU init, calls `main()`),
`runtime.asm` (software `__mul16`, `__div16`, `__mod16`), and the 4 library modules.
The final ROM is under 64 KB — small enough for a single LoROM bank.

---

## Technical Reference

| Register | Address | Role in this example |
|----------|---------|---------------------|
| BGMODE   | $2105   | Mode 0 (4 layers, all 2bpp) |
| BG1SC    | $2107   | BG1 tilemap at $0400 |
| BG12NBA  | $210B   | BG1 tile data at $0000 |
| TM       | $212C   | Enable BG1 only |
| HVBJOY   | $4212   | Wait for joypad auto-read to finish |
| JOY1L/H  | $4218-19 | Joypad 1 data |

## Files

| File | What's in it |
|------|-------------|
| `main.c` | Everything — font, UI, state machine, input (~496 lines) |
| `Makefile` | `LIB_MODULES := console input sprite dma` |
| `debug.lua` | Mesen2 debug script for live inspection |
| `test_calc.lua` | Automated test: verifies calculations in emulator |
