# Mode 0 Example

Four independent background layers with separate palettes.

## Learning Objectives

After this lesson, you will understand:
- Mode 0 configuration and capabilities
- 2bpp tile format
- Using four background layers simultaneously
- Multi-layer parallax scrolling

## Prerequisites

- Completed Mode 1 example
- Understanding of tilemaps and palettes

---

## What This Example Does

Displays four background layers scrolling at different speeds:
- Each layer has its own 4-color palette
- Creates deep parallax effect
- Demonstrates Mode 0's unique capability

```
+----------------------------------------+
|  ★ ★ ★ STARS ★ ★ ★        BG4 (slowest)|
|                                        |
|  ~~~ CLOUDS ~~~              BG3       |
|                                        |
|  ▒▒▒ MOUNTAINS ▒▒▒           BG2       |
|                                        |
|  ███ GROUND ███          BG1 (fastest) |
+----------------------------------------+
```

**Controls:** None (automatic parallax animation)

---

## Code Type

**C with Assembly Helpers**

| Component | Type |
|-----------|------|
| Graphics loading | Assembly (`load_mode0_graphics`) |
| Register setup | Assembly (`setup_mode0_regs`) |
| Scroll animation | Assembly (`update_mode0_scroll`) |
| Main loop | C |

---

## Mode 0 Overview

Mode 0 is unique in providing four background layers:

| Layer | Color Depth | Palette | Priority |
|-------|-------------|---------|----------|
| BG1 | 2bpp | Colors 0-3 | Highest |
| BG2 | 2bpp | Colors 4-7 | |
| BG3 | 2bpp | Colors 8-11 | |
| BG4 | 2bpp | Colors 12-15 | Lowest |

Each layer has only 4 colors, but they're completely independent.

### Mode 0 Use Cases

- Deep parallax backgrounds
- Layered UI elements
- Games needing many independent layers
- Text overlays with separate palettes

---

## 2bpp Tile Format

Each 8x8 tile uses 16 bytes:

```
Row 0: Bitplane 0, Bitplane 1
Row 1: Bitplane 0, Bitplane 1
...
Row 7: Bitplane 0, Bitplane 1
```

Pixel colors are 0-3 based on the two bitplane values.

---

## Layer Configuration

### VRAM Layout

```
$0000-$07FF: BG1 tilemap
$0800-$0FFF: BG2 tilemap
$1000-$17FF: BG3 tilemap
$1800-$1FFF: BG4 tilemap
$2000-$3FFF: Tile graphics
```

### Setting Up Each Layer

```c
/* BG1 - Foreground */
REG_BG1SC = 0x00;    /* Tilemap at $0000 */
REG_BG12NBA = 0x22;  /* Tiles at $2000 */

/* BG2 */
REG_BG2SC = 0x08;    /* Tilemap at $0800 */

/* BG3 */
REG_BG3SC = 0x10;    /* Tilemap at $1000 */
REG_BG34NBA = 0x22;  /* Tiles at $2000 */

/* BG4 */
REG_BG4SC = 0x18;    /* Tilemap at $1800 */
```

---

## Parallax Scrolling

Each layer scrolls independently:

```c
static u16 scroll = 0;

scroll++;

/* Each layer scrolls at different speed */
bgSetScroll(0, scroll, 0);       /* BG1: full speed */
bgSetScroll(1, scroll >> 1, 0);  /* BG2: half speed */
bgSetScroll(2, scroll >> 2, 0);  /* BG3: quarter speed */
bgSetScroll(3, scroll >> 3, 0);  /* BG4: eighth speed */
```

---

## Build and Run

```bash
cd examples/graphics/8_mode0
make clean && make

# Run in emulator
/path/to/Mesen mode0.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Main loop |
| `helpers.asm` | Graphics and register setup |
| `data.asm` | Tile and tilemap data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Interactive Scroll

Add D-pad control:
```c
if (pad & KEY_RIGHT) scroll_x++;
if (pad & KEY_LEFT) scroll_x--;
```

### Exercise 2: Vertical Parallax

Create a vertically scrolling scene:
```c
bgSetScroll(0, 0, scroll_y);
bgSetScroll(1, 0, scroll_y >> 1);
```

### Exercise 3: Layer Toggle

Toggle layers on/off with buttons:
```c
static u8 layer_mask = 0x0F;
if (pressed & KEY_A) layer_mask ^= 0x01;  /* Toggle BG1 */
REG_TM = layer_mask;
```

### Exercise 4: Different Palettes

Give each layer a distinct color scheme:
```c
/* Colors 0-3: BG1 - Earth tones */
/* Colors 4-7: BG2 - Greens */
/* Colors 8-11: BG3 - Blues */
/* Colors 12-15: BG4 - Dark purples */
```

---

## Technical Notes

### Mode 0 vs Mode 1

| Aspect | Mode 0 | Mode 1 |
|--------|--------|--------|
| BG layers | 4 | 3 |
| Colors per layer | 4 | 16/16/4 |
| Total BG colors | 16 | 36 |
| Best for | Many layers | Rich graphics |

### Priority

In Mode 0, layer priority can be adjusted with the priority bit in tilemap entries. Default order (back to front): BG4, BG3, BG2, BG1.

### Palette Organization

```
Colors 0-3:   BG1 palette
Colors 4-7:   BG2 palette
Colors 8-11:  BG3 palette
Colors 12-15: BG4 palette
```

Color 0 of each palette is typically transparent.

---

## What's Next?

**Parallax:** [Parallax Example](../9_parallax/) - Advanced scrolling

**Mode 3:** [Mode 3 Example](../10_mode3/) - 256-color graphics

---

## License

Code: MIT
