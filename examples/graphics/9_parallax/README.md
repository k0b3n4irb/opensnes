# Parallax Example

Multi-layer parallax scrolling with automatic animation.

## Learning Objectives

After this lesson, you will understand:
- Advanced parallax techniques
- Independent layer scrolling
- Creating depth perception
- Automatic scroll animation

## Prerequisites

- Completed scrolling example
- Understanding of multiple background layers

---

## What This Example Does

Displays two layers with different scroll behaviors:
- BG1: Static or slow-scrolling foreground
- BG2: Continuously auto-scrolling background
- Creates illusion of depth and movement

```
+----------------------------------------+
|   < < < BACKGROUND SCROLLS < < <       |
|                                        |
|   ═══════ FOREGROUND STATIC ═══════    |
|                                        |
|   < < < BACKGROUND SCROLLS < < <       |
+----------------------------------------+
```

**Controls:** None (automatic animation)

---

## Code Type

**C with Assembly Helpers**

| Component | Type |
|-----------|------|
| Graphics loading | Assembly (`load_parallax_graphics`) |
| Register setup | Assembly (`setup_parallax_regs`) |
| Scroll update | Assembly (`update_parallax`) |
| Main loop | C |

---

## Parallax Theory

### Depth Perception

Real-world observation: distant objects appear to move slower than near objects. We simulate this by:
- Near layers: Fast scroll
- Far layers: Slow scroll
- Background: Slowest or static

### Scroll Ratios

Common parallax ratios:

| Layer | Speed | Effect |
|-------|-------|--------|
| Foreground | 1.0x | Player's ground level |
| Midground | 0.5x | Trees, buildings |
| Background | 0.25x | Mountains |
| Sky | 0.0x | Static horizon |

---

## Implementation

### Auto-Scroll Background

```c
static u16 bg2_scroll = 0;

/* Main loop */
while (1) {
    WaitForVBlank();

    /* BG2 scrolls continuously */
    bg2_scroll++;
    bgSetScroll(1, bg2_scroll, 0);

    /* BG1 stays static */
    bgSetScroll(0, 0, 0);
}
```

### Player-Relative Scroll

```c
/* Background scrolls relative to player movement */
if (player_moving_right) {
    bg1_scroll += 2;  /* Foreground: full speed */
    bg2_scroll += 1;  /* Background: half speed */
}
```

---

## Assembly Optimization

For smooth scrolling, assembly provides consistent timing:

```asm
update_parallax:
    php
    rep #$20

    ; Increment scroll counter
    lda.l scroll_x
    inc a
    sta.l scroll_x

    ; Apply to BG2
    sta.l REG_BG2HOFS
    stz.l REG_BG2HOFS   ; Write high byte

    plp
    rtl
```

---

## Build and Run

```bash
cd examples/graphics/9_parallax
make clean && make

# Run in emulator
/path/to/Mesen parallax.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Main loop and initialization |
| `helpers.asm` | Scroll update routines |
| `data.asm` | Background graphics |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Three-Layer Parallax

Add a third layer with intermediate speed:
```c
bg1_scroll += 4;  /* Fastest */
bg2_scroll += 2;  /* Medium */
bg3_scroll += 1;  /* Slowest */
```

### Exercise 2: Bidirectional Scroll

Scroll in opposite directions:
```c
bg1_scroll += 1;  /* Right */
bg2_scroll -= 1;  /* Left (creates cool effect) */
```

### Exercise 3: Vertical Parallax

Create a vertical climbing scene:
```c
bgSetScroll(0, 0, scroll_y);
bgSetScroll(1, 0, scroll_y >> 1);
```

### Exercise 4: Speed Control

Let player control scroll speed:
```c
static u8 speed = 1;
if (pressed & KEY_UP) speed++;
if (pressed & KEY_DOWN) speed--;
bg2_scroll += speed;
```

---

## Technical Notes

### Smooth Sub-Pixel Scrolling

For very slow scrolling, use fixed-point:
```c
static u32 bg_scroll_fp = 0;  /* 16.16 fixed point */
bg_scroll_fp += 0x8000;       /* 0.5 pixels per frame */
bgSetScroll(1, bg_scroll_fp >> 16, 0);
```

### Seamless Looping

Design tilemaps to loop seamlessly:
- 32-tile wide tilemap = 256 pixels
- After scroll > 256, it wraps automatically

### VBlank Timing

Update scrolls during VBlank to avoid tearing:
```c
WaitForVBlank();
bgSetScroll(0, scroll_x, 0);
bgSetScroll(1, scroll_x >> 1, 0);
```

---

## What's Next?

**Mode 3:** [Mode 3 Example](../10_mode3/) - 256-color backgrounds

**Mode 5:** [Mode 5 Example](../11_mode5/) - Hi-res interlaced

---

## License

Code: MIT
