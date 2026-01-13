# SNES OAM (Object Attribute Memory)

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
Bit 3: Palette (high bit, with bits 1-3 from palette select)
Bits 1-3: Palette select (0-7)
Bit 0: Tile number (bit 8, for second tile page)
```

### Extended Table (2 bits per sprite)

```
Bit 1: Size (0=small, 1=large)
Bit 0: X position (bit 8, for X > 255)
```

## Critical: OAM Write Buffer

**This is the most important thing to understand about OAM writes.**

The SNES uses a **16-bit internal write buffer** for OAM:

1. First write to `$2104` → goes to internal **low byte buffer only**
2. Second write to `$2104` → goes to **high byte buffer AND commits both bytes to OAM**

### The Problem

If you only write one byte (e.g., just the X position), **nothing happens**. The byte sits in the internal buffer but is never committed to actual OAM.

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

To hide a sprite, set its Y position to 240 (off-screen):

```c
REG_OAMADDL = sprite_id * 4;
REG_OAMADDH = 0;
REG_OAMDATA = 0;    // X (don't care)
REG_OAMDATA = 240;  // Y = 240 hides sprite
```

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

1. **Writing single bytes** - Always write in pairs
2. **Writing outside VBlank** - Can cause visual corruption
3. **Forgetting extended table** - Large sprites need size bit set
4. **X position > 255** - Need to set bit 0 in extended table

## See Also

- [Registers](REGISTERS.md) - Full register reference
- [PPU](PPU.md) - Graphics system overview
- [Animation Example](/examples/graphics/2_animation/) - Working sprite animation code
