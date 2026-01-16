# Calculator Example

A simple 4-function calculator demonstrating SNES programming:
- Direct hardware register access
- VBlank synchronization via polling
- Joypad input with edge detection
- 16-bit arithmetic operations (add, subtract, multiply, divide)

## Build and Run

```bash
cd examples/basics/1_calculator
make clean && make

# Test with emulator
/path/to/Mesen calculator.sfc
```

**Controls:**
- D-pad: Move cursor between buttons
- A button: Press selected button
- Buttons: 0-9, +, -, *, /, C (clear), = (equals)

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Main program (calls assembly loop) |
| `calc_loop.asm` | Calculator logic in 65816 assembly |
| `Makefile` | Build configuration |
| `project_hdr.asm` | ROM header and vectors |

---

## How It Works

### Hardware Initialization
The calculator initializes directly without using library functions:
- Sets Mode 0 (2bpp backgrounds)
- Configures BG1 tilemap and tiles
- Uploads font tiles to VRAM
- Enables NMI and auto-joypad

### Main Loop
1. Wait for VBlank (poll $4212)
2. Wait for auto-read complete
3. Read joypad with edge detection
4. Process D-pad for cursor movement
5. Process A button for button presses
6. Update display and cursor brackets

### Arithmetic
- 16-bit values (0-65535)
- Shift-and-add multiplication
- Binary long division
- No floating point

---

## Testing

Automated tests verify this example works correctly:

```bash
# Run from project root
cd tests
./run_tests.sh examples
```

Or run the specific test in Mesen2:
```
tests/examples/calculator/test_calculator.lua
```

### Test Coverage

- ROM boots and reaches `main()`
- `calc_main_loop` is called
- Hardware initialization (`InitHardware`)
- Calculator variables initialized correctly
- D-pad input moves cursor
- VBlank handler is operational
- No WRAM mirror overlaps

See [snesdbg](../../../tools/snesdbg/) for the debug library used in tests.

---

## Historical Note

This example was fixed in January 2026. The original code used library functions
(`consoleInit`, `setMode`, `padUpdate`, etc.) that didn't exist. The fix rewrote
the code to use direct hardware register access, making it self-contained.

---

## See Also

- [hello_world](../../text/1_hello_world/) - Working text example
- [animation](../../graphics/2_animation/) - Sprite animation example
- [KNOWLEDGE.md](../../../.claude/KNOWLEDGE.md) - SNES development knowledge base
