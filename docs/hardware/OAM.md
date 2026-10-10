# SNES OAM (Object Attribute Memory) {#oam}

OAM stores sprite attributes for the PPU. Understanding how OAM works is critical for sprite programming.

## OAM Structure

OAM is **544 bytes** total:

| Section | Bytes | Content |
|---------|-------|---------|
| Main Table | 0-511 | 128 sprites × 4 bytes each |
| Extended Table | 512-543 | 128 sprites × 2 bits each |

### Main Table (4 bytes per sprite)

| Byte | Content |
|------|---------|
| 0 | X position (lower 8 bits) |
| 1 | Y position |
| 2 | Tile number (lower 8 bits) |
| 3 | Attributes (see below) |

### Attribute Byte (Byte 3)

```
Bit 7: Vertical flip
Bit 6: Horizontal flip
Bits 5-4: Priority (0-3)
Bits 3-1: Palette (0-7)
Bit 0: Tile number bit 8 (second tile page)
```

### Extended Table (2 bits per sprite)

```
Bit 1: Size (0=small, 1=large)
Bit 0: X position (bit 8, for X > 255)
```

## Critical: OAM Write Buffer

**This is the most important thing to understand about OAM writes.**

For the **main table** (bytes 0-511), the SNES uses a **16-bit internal
write buffer**:

1. Write to `$2104` at an even address → goes to the internal **low byte buffer only**
2. Write at the following odd address → goes to the **high byte AND commits both bytes to OAM**

The **extended table** (bytes 512-543) is the exception: writes there
apply **immediately**, byte by byte, with no pair-latching. (Confirmed
by the register references — the latch logic only applies below
internal address $200.)

### The Problem

For the main table, if you only write one byte (e.g., just the X position), **nothing happens**. The byte sits in the internal buffer but is never committed to actual OAM.

```c
// WRONG - Only writes X, never commits to OAM!
REG_OAMADDL = 0;
REG_OAMADDH = 0;
REG_OAMDATA = x;  // Goes to buffer, but no commit!
```

### The Solution

**Always write bytes in pairs:**

```c
// CORRECT - Writes X and Y, commits both to OAM
REG_OAMADDL = 0;
REG_OAMADDH = 0;
REG_OAMDATA = x;    // X goes to low byte buffer
REG_OAMDATA = y;    // Y goes to high byte, COMMITS both X and Y!
REG_OAMDATA = tile; // Tile goes to low byte buffer
REG_OAMDATA = attr; // Attr goes to high byte, COMMITS both!
```

### Why PVSnesLib "Just Works"

PVSnesLib uses a **shadow buffer** approach:
1. All sprite functions write to a RAM buffer (`oamMemory`)
2. The VBlank handler DMAs the entire 544-byte buffer to OAM
3. DMA always transfers complete data, avoiding the pair-write issue

For direct OAM writes without a shadow buffer, you must respect the pair-write requirement.

## OAM Registers

| Register | Address | Description |
|----------|---------|-------------|
| OAMADDL | $2102 | OAM address (low byte) |
| OAMADDH | $2103 | OAM address (high byte) + priority rotation |
| OAMDATA | $2104 | OAM data write |

### Setting OAM Address

```c
// Set OAM address to sprite N (N = 0-127)
REG_OAMADDL = N * 4;  // Each sprite is 4 bytes
REG_OAMADDH = 0;
```

For the extended table:
```c
// Set OAM address to extended table
REG_OAMADDL = 0;
REG_OAMADDH = 1;  // Bit 0 = 1 selects high table (bytes 512+)
```

## Sprite Sizes

`$2101` (OBJSEL) configures sprite sizes:

| Value | Small | Large |
|-------|-------|-------|
| 0x00 | 8x8 | 16x16 |
| 0x20 | 8x8 | 32x32 |
| 0x40 | 8x8 | 64x64 |
| 0x60 | 16x16 | 32x32 |
| 0x80 | 16x16 | 64x64 |
| 0xA0 | 32x32 | 64x64 |

## Hiding Sprites

To hide a sprite, set its Y position to 240 **and** set its X-position
high bit (placing it at X=256, off-screen to the right):

```c
REG_OAMADDL = sprite_id * 4;
REG_OAMADDH = 0;
REG_OAMDATA = 0;    // X low byte (don't care)
REG_OAMDATA = 240;  // Y = 240
// ...and set bit 0 of the sprite's extended-table pair (X bit 8).
```

Y=240 alone only hides sprites up to 16 px tall: sprite Y wraps
vertically, so a 32x32 sprite at Y=240 shows its last ~16 rows at the
**top of the screen**, and no Y value can fully hide a 64x64. This is
exactly what the lib's `oamHide()` / `oamClear()` do — prefer them over
manual pokes.

One refinement worth knowing: a sprite at X=256 is still treated as
X=0 for the per-scanline range/time evaluation. A hidden *large*
sprite whose Y wraps onto visible lines therefore still consumes the
32-sprites / 34-slivers budget on those lines — if the top of your
screen drops sprites mysteriously, audit your hidden large sprites.

## Sprite Y: no correction {#oam_sprite_y}

A sprite's OAM `Y` is the picture line of its top row, 0 being the first
visible line. The PPU does draw a sprite one scanline below its `Y`, and it
never outputs scanline 0: the two cancel. The arbiter, word for word
(SNESdev wiki, *Sprites*, <https://snes.nesdev.org/wiki/Sprites>):

> Like the NES, sprites appear 1 line lower than their Y value, however
> because the first line of rendering is always hidden on SNES, a sprite
> with Y=0 will appear to begin on the first visible line. However, a
> background with Y scroll of 0 will appear to have its top pixel cut off
> by the hidden line.

So only the **background** needs a correction, and the library applies it:
`bgSetScroll()` writes `y - 1` to `BGnVOFS`. Every sprite API stores the
`y` it is given, and a direct write to `oamMemory[id * 4 + 1]` stores `y`
too. A sprite and the background it stands on take the same camera value.

Measured on luna (2026-10-09): an 8x8 sprite with OAM `Y` = 0 covers
picture lines 0 to 7; with `Y` = 255 it covers lines 0 to 6, its top row
lost on the scanline that is never output.

**History, because code written against it exists.** From 2026-04-27 to
2026-10-09 `oamSet()`, `oamSetY()`, `oamSetXY()` and the `oamSetFast`
macros stored `y - 1`, on a reading of the first half of the sentence
above without the second; the dynamic engine followed on 2026-09-26. Every
sprite was drawn one line too high, and since `bgSetScroll()` got its
`- 1` (2026-09-12) one line above a background scrolled to the same `y`.
Code that subtracted 1 by hand before a direct `oamMemory[]` write must
stop; code that added 1 to a `y` passed to `oamSet()` to line a sprite up
with the background must stop too.

Sentinel values that hide a sprite (`OBJ_HIDE_Y = 240`) are positions like
any other: 240 is below the last visible line (223).

The X axis has no such subtlety.

## Timing

OAM writes should be done during **VBlank** or **forced blank** (screen off). Writing during active display can cause visual glitches.

```c
// Wait for VBlank before OAM updates
while (REG_HVBJOY & 0x80) {}   // Wait for VBlank to end
while (!(REG_HVBJOY & 0x80)) {} // Wait for VBlank to start
// Now safe to write OAM
```

## Complete Example

```c
// Update sprite 0 position and animation frame
void update_sprite(u8 x, u8 y, u8 tile, u8 attr) {
    // Wait for VBlank
    while (REG_HVBJOY & 0x80) {}
    while (!(REG_HVBJOY & 0x80)) {}

    // Set OAM address to sprite 0
    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    // Write all 4 bytes (2 pairs)
    REG_OAMDATA = x;    // Pair 1: X
    REG_OAMDATA = y;    //         Y (commits X,Y)
    REG_OAMDATA = tile; // Pair 2: Tile
    REG_OAMDATA = attr; //         Attr (commits tile,attr)
}
```

## Common Pitfalls

1. **Writing single bytes** - Always write the main table in pairs (extended-table writes are immediate)
2. **Writing outside VBlank** - Can cause visual corruption
3. **Forgetting extended table** - Large sprites need size bit set
4. **X position > 255** - Need to set bit 0 in extended table

## See Also

- [Registers](REGISTERS.md) - Full register reference
- [SNES Graphics Guide](../SNES_GRAPHICS_GUIDE.md) - Graphics system overview
- [Animated Sprite Example](/examples/sprites/animated_sprite/) - Working sprite animation code
