# Custom Font

> A complete A-Z, 0-9 font with punctuation — all 42 characters hand-coded in hex.
> The logical next step after Hello World.

![Screenshot](screenshot.png)

## Build & Run

```bash
make -C examples/text/custom_font
# Open custom_font.sfc in Mesen2
```

## What You'll Learn

- How to build a reusable character set (not just the 9 letters from Hello World)
- Pre-encoding messages as tile index arrays
- Tilemap address arithmetic for positioning text anywhere on screen
- The alternative path: using `font2snes` to convert a PNG font image

---

## Walkthrough

### 1. A Real Font This Time

[Hello World](../hello_world/) gave us H, E, L, O, W, R, D and !. Useful for exactly
one message. This example builds a proper character set — 42 tiles covering the full
alphabet, digits, and basic punctuation:

```c
/* Tile indices:
 * 0 = space
 * 1-26 = A-Z
 * 27-36 = 0-9
 * 37 = !   38 = .   39 = :   40 = -   41 = =
 */
```

Every tile is still 16 bytes of hand-coded 2bpp data. The letter A, for example:

```c
/* 1: A */
0x18,0x00, 0x3C,0x00, 0x66,0x00, 0x7E,0x00,
0x66,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
```

Row by row: `0x18` = narrow top, `0x3C` = wider, `0x66` = two legs, `0x7E` = crossbar,
then three more `0x66` rows for the body. 42 tiles × 16 bytes = 672 bytes. A tiny price
for the ability to write any message.

### 2. Messages as Tile Arrays

Since this example doesn't use the library's text system, messages are pre-encoded as
arrays of tile indices:

```c
/* "HELLO WORLD!" */
static const u8 msg_hello[] = {
    8,5,12,12,15, 0, 23,15,18,12,4, 37, 0xFF
};
/* H E  L  L  O  SP W  O  R  L  D  !  END */
```

Each number is a tile index: 8 = H (the 8th tile), 5 = E, 12 = L, and so on.
`0xFF` marks the end of the message.

> **Why not use ASCII strings?** The SNES has no character encoding system. There's no
> mapping from 'A' (ASCII 65) to tile 1. You could write a conversion function, but
> on a CPU where every cycle counts, pre-encoding at compile time is free.

### 3. Placing Text on Screen

The tilemap is a 32×32 grid starting at VRAM `$0400`. To place text at a specific row
and column, you calculate the VRAM address:

```c
/* Address = base + row × 32 + column */
addr = 0x0400 + 3*32 + 6;   /* Row 3, column 6 */
REG_VMADDL = addr & 0xFF;
REG_VMADDH = addr >> 8;

msg = msg_title;
i = 0;
while (msg[i] != 0xFF) {
    REG_VMDATAL = msg[i];
    REG_VMDATAH = 0;
    i++;
}
```

This example displays six blocks of text at different positions — a title, "HELLO WORLD!",
the alphabet in three rows, digits, and "OPENSNES" — showing that once you have the font,
placing text anywhere is just math.

### 4. No Library, No Problem

Notice this example doesn't call `consoleInit()` or `setMode()`. It configures the PPU
directly:

```c
REG_BGMODE = 0x00;   /* Mode 0 */
REG_BG1SC = 0x04;    /* Tilemap at $0400 */
REG_BG12NBA = 0x00;  /* Tiles at $0000 */
```

And turns on the screen with a raw register write:

```c
REG_TM = 0x01;         /* Enable BG1 */
REG_INIDISP = 0x0F;    /* Full brightness, display on */
```

This is closer to how real SNES games did it — no library, just registers. Understanding
these raw writes will help you debug any graphics issue, because at the end of the day,
it all comes down to what's in these registers.

### 5. The font2snes Alternative

Typing 672 bytes of hex is educational, but for a real project you'd use a tool.
The Makefile includes support for `font2snes`, which converts a PNG image into tile data:

```bash
font2snes -c assets/myfont.png myfont.h
```

Create a 128×48 PNG with 96 characters laid out in ASCII order (space through ~), and
the tool generates a C header with the tile array ready to `#include`.

---

## Tips & Tricks

- **Message looks shifted?** Double-check your tilemap address math. A common mistake
  is forgetting that rows are 32 tiles wide, not 28 (the visible screen is 32×28 tiles,
  but the tilemap is always 32×32).

- **Wrong letters appear?** Your tile index is off by one. Tile 0 = space, tile 1 = A.
  If you write `1` expecting space, you get A instead.

- **Want lowercase?** Add tiles 42-67 for a-z. Same 2bpp format, just different pixel
  patterns. The font grows from 672 bytes to 1088 — still tiny.

---

## Go Further

- **Colorful text:** Use different palette numbers in the tilemap high byte to give
  each line a different color. Bits 10-12 of the tilemap entry select the palette.

- **Scrolling credits:** Fill the tilemap with text and increment `BG1VOFS` each frame.
  Instant credits roll, no new code needed.

- **Runtime text conversion:** Write a function that maps ASCII codes to tile indices
  (`tile = ch - 'A' + 1` for letters). Then you can use C string literals instead of
  hand-coded arrays.

- **Next example:** [Calculator](../../basics/calculator/) — interactive input handling
  and state management.

---

## Under the Hood: The Build

### No Library

Look at the Makefile — there's no `USE_LIB` and no `LIB_MODULES`. This example builds
without the OpenSNES library entirely. It configures the PPU with raw register writes
(`REG_BGMODE`, `REG_BG1SC`, etc.) and doesn't need `consoleInit()`, `WaitForVBlank()`,
or any library function.

This is how actual SNES games were written: direct hardware access, no middleware.

### The font2snes Tool

The Makefile includes a custom build rule for font conversion:

```makefile
FONT2SNES := $(OPENSNES)/bin/font2snes
FONT_PNG  := assets/myfont.png
FONT_H    := myfont.h

$(FONT_H): $(FONT_PNG)
	$(FONT2SNES) -c $< $@
```

`font2snes` takes a PNG image containing a font grid and outputs a C header with the tile
data as a `const u8` array. The `-c` flag generates C format (as opposed to assembly).

The input image is a 128x48 pixel PNG with 96 characters laid out in ASCII order (space
through `~`), 8x8 pixels per character. The tool reads each 8x8 cell and encodes it as
a 2bpp tile — the exact same format you'd hand-code in hex, but without the tedium.

> **Why does this example hand-code the font in hex then?** For learning. The hand-coded
> version teaches you exactly what a tile looks like in memory. For a real project, use
> `font2snes` and save yourself 672 bytes of manual hex typing.

### The Pipeline (Without Library)

```
main.c  →  cc65816  →  main.c.asm  →  wla-65816  →  main.c.obj  →  wlalink  →  custom_font.sfc
```

Without `USE_LIB`, the linker only pulls in the bare minimum: `crt0.asm` (startup code
that initializes the 65816 CPU and calls your `main()`) and `runtime.asm` (software
multiply/divide helpers). No NMI handler, no OAM buffer, no DMA wrappers. The ROM is
smaller and you control everything — but you're responsible for everything too.

---

## Technical Reference

| Register | Address | Role in this example |
|----------|---------|---------------------|
| BGMODE   | $2105   | Mode 0 (2bpp, 4 layers) |
| BG1SC    | $2107   | BG1 tilemap at $0400 |
| BG12NBA  | $210B   | BG1 tiles at $0000 |
| VMAIN    | $2115   | VRAM auto-increment |
| TM       | $212C   | Enable BG1 |
| INIDISP  | $2100   | Screen on, full brightness |

## Files

| File | What's in it |
|------|-------------|
| `main.c` | Font data (42 tiles), message arrays, display code (~318 lines) |
| `Makefile` | Build config + optional `font2snes` conversion rules |
| `assets/myfont.png` | Source font image (for the font2snes pipeline) |
