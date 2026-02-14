# Mode 5 Example

High-resolution interlaced graphics at 512x256.

## Learning Objectives

After this lesson, you will understand:
- Mode 5 hi-res configuration
- Interlaced rendering with main/sub screens
- Trade-offs of high resolution mode
- When to use hi-res graphics

## Prerequisites

- Completed Mode 1 and Mode 3 examples
- Understanding of main/sub screen concepts

---

## What This Example Does

Displays a high-resolution background:
- 512x256 pixel resolution (double horizontal)
- Interlaced rendering using main and sub screens
- Sharper text and fine details

```
+----------------------------------------+
|                                        |
|   HIGH RESOLUTION 512x256 DISPLAY      |
|                                        |
|   Fine detail and sharp text           |
|   using interlaced rendering           |
|                                        |
+----------------------------------------+
```

**Controls:** None (static display)

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| Background setup | Library (`bgSetMapPtr`, `bgInitTileSet`) |
| Scroll init | Library (`bgSetScroll`) |
| Mode config | Direct register access |
| Main loop | C with `WaitForVBlank()` |

---

## Mode 5 Overview

Mode 5 doubles horizontal resolution through interlacing:

| Aspect | Normal Modes | Mode 5 |
|--------|-------------|--------|
| H Resolution | 256 | 512 |
| V Resolution | 224 | 224 (or 448 interlaced) |
| BG1 | 4bpp | 4bpp hi-res |
| BG2 | 2bpp | 2bpp hi-res |

### How Interlacing Works

Mode 5 alternates between main and sub screens:
- Even columns: Main screen
- Odd columns: Sub screen
- Combined: Double horizontal resolution

---

## Mode 5 Setup

```c
/* Set Mode 5 with hi-res */
REG_BGMODE = 0x05;  /* Mode 5 */
REG_SETINI = 0x04;  /* Enable hi-res */

/* Both screens display BG1 */
REG_TM = TM_BG1;    /* Main screen */
REG_TS = TM_BG1;    /* Sub screen */

/* BG1 setup */
bgSetMapPtr(0, 0x1000, SC_32x32);
bgInitTileSet(0, tiles, palette, 0,
              tiles_size, palette_size,
              BG_16COLORS, 0x4000);
```

---

## Hi-Res Tile Format

In Mode 5, tiles are 16 pixels wide (double width):
- Each tile still uses 8x8 pixel data
- Hardware stretches to display 16x8
- Or use two tiles side-by-side for true 16x8

---

## Main/Sub Screen Blending

For proper hi-res display:

```c
/* Enable both screens for BG1 */
REG_TM = 0x01;  /* Main screen: BG1 */
REG_TS = 0x01;  /* Sub screen: BG1 */

/* Set up color addition for blending */
REG_CGWSEL = 0x00;  /* Normal mode */
```

---

## Build and Run

```bash
cd examples/graphics/mode5
make clean && make

# Run in emulator
/path/to/Mesen mode5.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Mode 5 setup and main loop |
| `data.asm` | Hi-res graphics data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Compare Resolutions

Toggle between Mode 1 and Mode 5 to see the difference:
```c
if (pressed & KEY_A) {
    hi_res = !hi_res;
    REG_BGMODE = hi_res ? 0x05 : 0x01;
    REG_SETINI = hi_res ? 0x04 : 0x00;
}
```

### Exercise 2: Text Display

Display sharp text using hi-res mode - great for RPG menus.

### Exercise 3: Add Scrolling

Scroll the hi-res background:
```c
bgSetScroll(0, scroll_x, scroll_y);
/* Note: scroll values work the same */
```

### Exercise 4: Mix with Sprites

Add sprites (which remain at normal resolution):
```c
REG_TM = TM_BG1 | TM_OBJ;
```

---

## Technical Notes

### Resolution Details

| Mode | H-Res | V-Res | Notes |
|------|-------|-------|-------|
| Normal | 256 | 224 | Standard SNES |
| Mode 5 | 512 | 224 | Hi-res horizontal |
| Mode 5+interlace | 512 | 448 | Full interlace |

### Sprite Considerations

Sprites remain at normal resolution in Mode 5. This can cause:
- Sprites appear "chunky" compared to backgrounds
- Design sprites accordingly

### Color Limitations

Hi-res modes have stricter color limits:
- Fewer effective colors due to interlacing
- Plan palette carefully

### Performance

Mode 5 doesn't affect CPU performance - it's purely a PPU mode change.

---

## Games Using Hi-Res

- Secret of Mana (menus)
- Seiken Densetsu 3 (menus)
- RPGs for text clarity
- Some racing games for detail

---

## What's Next?

**Continuous Scroll:** [Continuous Scroll](../continuous_scroll/) - Game camera

**Animation System:** [Animation System](../animation_system/) - Sprite animation

---

## License

Code: MIT
