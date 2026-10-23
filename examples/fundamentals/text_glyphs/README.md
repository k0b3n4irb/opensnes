# Fundamentals: how a glyph is drawn

> The under-the-hood companion to the Text family. Nine tiles, three colours,
> one message — hand-coded 2bpp bitplanes written straight into VRAM, no
> `text` module. Read [`text/print_string`](../../text/print_string/) for the
> easy way; read this to see what that module hides.

![Screenshot](text_glyphs.png)

## Controls

No interactive controls. The text is displayed statically.

## What You'll Learn

- How the SNES turns pixels into pictures (tiles, tilemaps, palettes — the holy trinity)
- Why you can't just `printf("HELLO WORLD")` on a Super Nintendo
- What "2 bits per pixel" actually means when you're staring at hex bytes
- The difference between VRAM (video memory) and regular RAM

---

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/fundamentals/text_glyphs
```

Then open `text_glyphs.sfc` in [luna](https://github.com/k0b3n4irb/luna) (or
any SNES emulator).

## Walkthrough

### 1. There Is No printf

On a modern computer, printing text is one line of code. On the SNES, there is no
text system. There's no font. There's no framebuffer. The PPU (Picture Processing Unit)
only understands one thing: **tiles**.

A tile is an 8x8 pixel image stored in VRAM. A tilemap is a grid that says "put tile #3
here, tile #7 there." That's it — that's the entire display system. If you want letters
on screen, you need to draw each letter as a tile, upload them to VRAM, and then write
a tilemap that spells out your message.

That's exactly what this example does.

### 2. Drawing Letters in Hex

Each letter is hand-encoded as a 16-byte tile in 2bpp format (2 bits per pixel = 4 possible
colors). Every pair of bytes is one row of 8 pixels: the first byte is bitplane 0, the
second bitplane 1. The letters are the glyphs of the city's font, the one the `text`
module uses (`lib/source/text.asm`):

```c
static const u8 font_tiles[] = {
    /* Tile 1: H */
    0x66, 0x00, 0x66, 0x11, 0x66, 0x11, 0x7E, 0x01,
    0x66, 0x19, 0x66, 0x11, 0x66, 0x11, 0x00, 0x33,
    /* ... */
};
```

Take the first byte of each pair, `0x66` = binary `01100110`. Each `1` is a lit pixel,
each `0` is background. Draw them out and you'll see two vertical bars — the sides of
the letter H. The `0x7E` row (`01111110`) is the crossbar.

> **And the second bytes?** They are bitplane 1. A pixel's colour is its bitplane 1 bit
> times two plus its bitplane 0 bit: colour 1 is plane 0 alone, colour 2 plane 1 alone,
> colour 3 both. Bitplane 0 holds the letter (colour 1, cyan); bitplane 1 holds its
> shadow (colour 2, pink): the row above moved one pixel right (`>> 1`, bit 7 is the
> leftmost pixel), minus the pixels the letter covers. Row 1 of H:
> `(0x66 >> 1) & ~0x66` = `0x33 & 0x99` = `0x11`. No pixel has both bits set, so
> colour 3 never shows. That's how the SNES packs four colours into two bits a pixel.

### 3. Setting Up the PPU

Before writing anything to VRAM, you need to tell the PPU where to find things:

```c
consoleInit();
setMode(BG_MODE0, 0);

bgSetMapPtr(0, 0x0400, BG_MAP_32x32);   /* BG1SC: tilemap at VRAM $0400, 32x32 entries */
bgSetGfxPtr(0, 0x0000);                 /* BG12NBA: BG1 tile graphics at VRAM $0000 */
```

Mode 0 gives you four background layers, each with 4 colors (2bpp). For a simple
text display, one layer is plenty. `bgSetMapPtr` writes `BG1SC`: "the tilemap for
BG1 starts at VRAM word address `$0400`"; `bgSetGfxPtr` writes `BG12NBA`: "the
actual tile graphics start at `$0000`."

> **Why two separate addresses?** Because tiles and tilemaps are different data.
> Tiles are the pixel art. The tilemap is the layout — which tile goes where on
> screen. They can live anywhere in VRAM, and you choose where by setting these
> registers. This separation is what makes the SNES so flexible: you can reuse
> the same tiles with different layouts, or swap layouts without reloading tiles.

### 4. Uploading Tiles to VRAM

VRAM isn't regular memory — you can't just write to it with a pointer. You talk to it
through registers: set the address (`VMADDL`/`VMADDH`), then write data, low byte then
high byte (`VMDATAL`/`VMDATAH`). For a block of bytes, a DMA channel does those writes:

```c
dmaCopyVram((u8 *)font_tiles, 0x0000, 144);   /* VRAM $0000, 144 bytes */
```

VRAM is organized in 16-bit words. With `VMAIN = 0x80` ("auto-increment the address
after the high byte", which the library sets), each low/high pair lands in one word and
the address moves forward by one. 144 bytes = 9 tiles × 16 bytes each = our complete
font. Section 6 does the same writes by hand, for the tilemap.

### 5. Three Colors, Six Bytes

The SNES palette uses 15-bit BGR color (5 bits per channel, `0bbbbbgggggrrrrr`,
little-endian):

```c
static const u8 bg_palette[] = {
    0x61, 0x10,  /* Color 0: the glass, #0a1a22 */
    0x49, 0x73,  /* Color 1: cyan, #48d0e0 */
    0xB4, 0x30,  /* Color 2: pink, #a02860 */
};
dmaCopyCGram((u8 *)bg_palette, 0, 6);
```

Color 0 is the background, color 1 the letters, color 2 their shadow. Cyan is
`#48d0e0`: each 8-bit channel loses its three low bits (red 72 → 9, green 208 → 26,
blue 224 → 28), and `9 | 26 << 5 | 28 << 10` = `$7349`, stored low byte first.

### 6. Spelling the Message

The message is encoded as an array of tile indices:

```c
static const u8 message[] = {
    1, 2, 3, 3, 4,  /* HELLO */
    0,               /* space */
    5, 4, 6, 3, 7,  /* WORLD */
    8,               /* ! */
    0xFF             /* end marker */
};
```

Tile 1 = H, tile 2 = E, tile 3 = L, and so on. To place this at row 14, column 10,
we calculate the tilemap address:

```c
addr = 0x05CA;  /* 0x0400 (tilemap base) + 14×32 + 10 */
REG_VMADDL = addr & 0xFF;
REG_VMADDH = addr >> 8;

i = 0;
while (1) {
    tile = message[i];
    if (tile == 0xFF) break;
    REG_VMDATAL = tile;  /* Tile number */
    REG_VMDATAH = 0;     /* Attributes: palette 0, no flip */
    i++;
}
```

Each tilemap entry is 2 bytes: the low byte picks the tile, the high byte holds
attributes (palette, flip, priority). We set them all to 0 — palette 0, no flipping.

> **What's `0x05CA`?** The tilemap base is `$0400`. Each row is 32 entries wide.
> Row 14 starts at `$0400 + 14×32 = $0400 + $01C0 = $05C0`. Column 10 adds 10
> more: `$05CA`. That math becomes second nature after a few tilemaps.

### 7. Lights On

```c
setMainScreen(LAYER_BG1);   /* TM: show BG1 on the main screen */
setScreenOn();              /* Turn on the display */

while (1) {
    WaitForVBlank();
}
```

`TM` (screen designation) controls which layers are visible. We enable BG1 only.
`setScreenOn()` sets brightness to maximum. The infinite loop with `WaitForVBlank()`
keeps the program alive and synced to 60fps — without it, the CPU would spin wildly
and cause timing issues.

---

## Tips & Tricks

- **Screen is black?** Check that `setMainScreen()` (the `TM` register) has the right layer enabled. If BG1 isn't
  in the TM register, the PPU won't draw it, no matter what's in VRAM.

- **Tiles look wrong?** Count your bytes. 2bpp = 16 bytes per tile. If your array is
  off by even one byte, every subsequent tile shifts and you get garbage.

- **Want to understand 2bpp better?** Take the `0x66` byte and draw it on graph paper.
  Binary: `01100110`. Mark the 1s. Do all 8 rows of the H tile. You'll see the letter
  form before your eyes.

- **Why `setMode(BG_MODE0, 0)` and not Mode 1?** Mode 0 gives 4 layers of 2bpp (4 colors
  each). Mode 1 gives 2 layers of 4bpp + 1 layer of 2bpp. For monochrome text,
  Mode 0 is the natural fit — and you get 3 extra layers free if you want them later.

---

## Go Further

- **Change the message:** Add tiles for your own letters. Each tile is 16 bytes — sketch
  it on graph paper first, convert to hex, add it to `font_tiles[]`.

- **Use colour 3:** set both bitplanes on some pixels (an inner highlight on each
  letter, say) and add a fourth colour to the palette.

- **Two lines of text:** Write a second message at a different tilemap row. Row N starts
  at base address + N×32.

- **Next example:** [Simple Sprite](../../sprites/simple_sprite/) -- display a
  32x32 sprite on screen using OAM.

---

## Under the Hood: The Build

### The Pipeline

```
main.c  →  cc65816  →  main.c.asm  →  wla-65816  →  main.c.obj  →  wlalink  →  text_glyphs.sfc
  C          cproc+       65816          WLA-DX         object        linker       final ROM
source       QBE         assembly       assembler        file
```

`cc65816` is the C compiler — it's actually two programs stitched together: `cproc` (a C11
frontend) parses your C code into intermediate representation, then `QBE` (with a custom
65816 backend) turns that IR into assembly. `wla-65816` assembles it into an object file.
Finally, `wlalink` links everything — your compiled code, the startup code (crt0.asm),
math helpers (runtime.asm), and library modules — into a playable `.sfc` ROM.

### The Makefile

```makefile
TARGET      := text_glyphs.sfc           # Output ROM filename
ROM_NAME    := OPENSNES HELLO WORLD     # 21-char name burned into the ROM header
USE_LIB     := 1                        # Link against the OpenSNES library
LIB_MODULES := console sprite dma background  # Which library pieces to pull in
CSRC        := main.c                   # Your single C source file
```

`CSRC` can list multiple C files (e.g., `CSRC := main.c engine/player.c`).
For this simple example, a single file is sufficient. You can also add assembly
source files via `ASMSRC`.

### Why These Modules?

| Module | Why it's here |
|--------|--------------|
| `console` | Provides `consoleInit()` and `WaitForVBlank()`. Sets up Mode 1, the NMI handler, and basic PPU config. |
| `sprite` | OAM buffer management. Even with no sprites on screen, the NMI handler DMAs the OAM buffer every frame — if the sprite module is missing, the linker can't find the buffer symbols. |
| `dma` | Provides `dmaCopyVram()` and `dmaCopyCGram()`. Without it, you'd write tile data byte-by-byte through registers (which we actually do in this example, but the library's init code uses DMA internally). |

> **"But there are no sprites in Hello World!"** Correct. The dependency chain is:
> `console` → `sprite` → `dma`. The NMI handler (in console) always DMAs the OAM buffer
> (in sprite) to the PPU, and that DMA call lives in the dma module. The build system
> does NOT auto-resolve these dependencies — if you forget `dma`, you get a linker error
> about an undefined `dmaCopyOam` symbol and no hint about why.

---

## Modules Used

`console`, `sprite`, `dma`, `background` (`LIB_MODULES` in the Makefile).

## Technical Reference

| Register | Address | Role in this example |
|----------|---------|---------------------|
| BGMODE   | $2105   | Mode 0 (4 layers, all 2bpp) |
| BG1SC    | $2107   | BG1 tilemap at $0400 |
| BG12NBA  | $210B   | BG1 tile data at $0000 |
| VMAIN    | $2115   | VRAM auto-increment mode |
| VMADDL/H | $2116-17 | VRAM write address |
| VMDATAL/H | $2118-19 | VRAM data port |
| CGADD    | $2121   | Palette address |
| CGDATA   | $2122   | Palette data |
| TM       | $212C   | Enable BG1 on main screen |

## Files

| File | What's in it |
|------|-------------|
| `main.c` | Everything — tiles, palette, tilemap, main loop (~180 lines) |
| `Makefile` | `LIB_MODULES := console sprite dma background` |
