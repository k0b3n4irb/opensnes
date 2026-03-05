# [Example Name]

> **One-line summary**: what this example does, visible on screen.

![Screenshot](screenshot.png)
<!-- Real Mesen2 capture. Remove this line if no screenshot yet. -->

## Controls

| Button | Action |
|--------|--------|
| D-Pad  | ...    |
| A      | ...    |

<!-- Remove this section if the example has no interactivity. -->

## Build & Run

```bash
make -C examples/category/example_name
# Open the .sfc in Mesen2
```

## What You'll Learn

List the **SNES hardware concepts** taught by this example, not C functions:

- How VRAM stores tiles and tilemaps
- The difference between video modes (Mode 0 vs Mode 1)
- Why VRAM writes must happen during VBlank

<!-- 3 to 6 points. Each point = a hardware or SDK concept the reader
     didn't know before and will understand after reading this README. -->

## Walkthrough

The heart of the README. Explain the code **in execution order**, not file order.
Each section corresponds to a logical step.

### 1. Initialization

Explain what `consoleInit()` and `setMode()` do in the context of this example.
Don't repeat the API docs — explain **why** this choice is made here.

```c
// Direct excerpt from this example's main.c
consoleInit();
setMode(BG_MODE1, 0);  // Mode 1: 2 layers 4bpp + 1 layer 2bpp
```

> **Why Mode 1?** It's the most common mode on the SNES (Super Mario World,
> Street Fighter II). Good balance between colors per layer (16) and number
> of layers (3).

### 2. Loading Graphics

Show real code, then explain the **data format** and the **hardware mechanism**.

```c
// 4bpp tiles: 32 bytes per tile (8 rows x 4 bitplanes)
REG_VMAIN = 0x80;  // Auto-increment on high byte write
REG_VMADDL = 0x00;
REG_VMADDH = 0x00;
for (i = 0; i < sizeof(tiles); i += 2) {
    REG_VMDATAL = tiles[i];
    REG_VMDATAH = tiles[i + 1];
}
```

> **How does it work?** VRAM is organized in 16-bit words. `VMAIN = 0x80`
> configures auto-increment on the high byte write (`VMDATAH`). Each pair
> of writes fills one VRAM word.

<!-- Continue with as many sections as needed.
     Each section = one logical block of the program.
     Always: REAL code excerpt -> WHY explanation -> hardware detail if relevant. -->

### N. Main Loop

```c
while (1) {
    WaitForVBlank();
    // ...
}
```

Explain what happens each frame and why.

## Tips & Tricks

Practical tips and common pitfalls **specific to this example**:

- **VRAM and VBlank**: VRAM writes are silently ignored during active display.
  If your display is corrupted, make sure you write during `WaitForVBlank()`.

- **Palette color 0**: always the backdrop. If your screen is black but tiles are
  loaded, check that color 0 isn't set to black.

<!-- 2 to 5 tips. Each one should be:
     1. A concrete problem the reader might encounter
     2. Related to THIS example's code (not general theory)
     3. With the solution or explanation -->

## Go Further

Concrete modifications to try on this example:

- **Change the colors**: modify the `REG_CGDATA` values and see what happens.
  Format is BGR555 (5 bits per channel, little-endian).

- **Add animation**: in the main loop, increment a variable each frame and use
  it to change the scroll offset or palette.

- **Next example**: [Next Example Name](../next/) to learn [concept].

<!-- 2 to 4 suggestions. The last one points to the next example in progression. -->

## Under the Hood: The Build

Explain the Makefile: what tools are used, what modules are needed and why.

```makefile
TARGET      := example.sfc
USE_LIB     := 1
LIB_MODULES := console sprite dma
```

| Module | Why it's here |
|--------|--------------|
| `console` | ... |
| `sprite` | ... |
| `dma`     | ... |

If graphics tools are used (gfx4snes, font2snes, smconv), explain what they do
and why the flags are set the way they are.

## Technical Reference

| Register | Address | Role in this example |
|----------|---------|---------------------|
| VMAIN    | $2115   | VRAM increment mode |
| VMADDL/H | $2116-17 | VRAM write address |
| VMDATAL/H | $2118-19 | VRAM data |
| BG1SC    | $2107   | BG1 tilemap location |
| TM       | $212C   | Active layers |

<!-- Only registers USED in this example's code.
     No exhaustive table — just what's relevant here. -->

## Files

| File | What's in it |
|------|-------------|
| `main.c` | Full source code |
| `data.asm` | Graphics data (if present) |
| `Makefile` | `LIB_MODULES := console sprite dma` |

---

<!-- NOTES FOR THE README AUTHOR (remove from final file):

PRINCIPLES:
- Code shown is ALWAYS a real excerpt from main.c, never pseudo-code
- Each explanation answers "why", not "what" (the code already shows "what")
- Hardware concepts are explained WHEN they appear in code,
  not in a separate theory section
- Tone is technical but accessible: a C developer who doesn't know
  the SNES should be able to follow

LENGTH:
- No hard limit, but every line should add value
- A hello_world might be 60 lines, a breakout might be 300
- If a section gets long, it might be two examples in one

SCREENSHOTS:
- Real PNG capture from Mesen2 (File > Save Screenshot)
- Named screenshot.png in the same folder
- Multiple captures OK if the example has multiple visual states

CROSS-REFERENCES:
- Use relative paths: [Mode 1](../../backgrounds/mode1/)
- "Go Further" points to the next example in progression
- Don't create circular dependencies
-->
