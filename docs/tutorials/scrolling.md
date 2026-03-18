# Scrolling & Parallax Tutorial {#tutorial_scrolling}

This tutorial covers SNES background scrolling techniques, from basic offset control to continuous tile streaming.

## How BG Scrolling Works

The SNES PPU has two scroll registers per background layer: one horizontal (BGnHOFS) and one vertical (BGnVOFS). These registers define which pixel of the tilemap appears at the top-left corner of the screen. The PPU reads these registers once per frame during rendering.

OpenSNES provides `bgSetScroll()` to set these values. The function writes to shadow variables and marks the background as dirty; the NMI handler then commits the values to hardware during VBlank.

### Wrap Behavior

Scroll offsets are 10 bits wide (0-1023). The tilemap wraps seamlessly:

| Tilemap Size | Pixel Dimensions | Wrap Point |
|--------------|-----------------|------------|
| SC_32x32 | 256 x 256 | Wraps at 256 |
| SC_64x32 | 512 x 256 | Wraps at 512 horizontally, 256 vertically |
| SC_32x64 | 256 x 512 | Wraps at 256 horizontally, 512 vertically |
| SC_64x64 | 512 x 512 | Wraps at 512 |

When the scroll offset exceeds the tilemap dimensions, the PPU wraps back to the beginning. This means a 32x32 tilemap scrolled to X=260 displays the same content as X=4.

## Basic BG Scrolling

Use `bgSetScroll(bg, x, y)` to set the scroll offset for a background layer. The `bg` parameter is 0-indexed (0 = BG1, 1 = BG2, etc.).

```c
#include <snes.h>

u16 scroll_x = 0;
u16 scroll_y = 0;

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    /* ... load tiles, palettes, tilemap ... */

    bgSetMapPtr(0, 0x0000, SC_32x32);
    REG_TM = TM_BG1;
    setScreenOn();

    while (1) {
        /* Auto-scroll: 1 pixel per frame horizontally */
        scroll_x++;
        bgSetScroll(0, scroll_x, scroll_y);

        WaitForVBlank();
    }
    return 0;
}
```

You can also set horizontal and vertical scroll independently:

```c
bgSetScrollX(0, scroll_x);   /* Horizontal only */
bgSetScrollY(0, scroll_y);   /* Vertical only */
```

To read back the current scroll position (from the shadow variables):

```c
u16 cur_x = bgGetScrollX(0);
u16 cur_y = bgGetScrollY(0);
```

## D-PAD Controlled Scrolling

Read the joypad state with `padHeld()` and update the scroll position based on which directions are held. Use `s16` for scroll variables so negative values wrap correctly.

```c
#include <snes.h>

s16 scroll_x = 0;
s16 scroll_y = 0;

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    /* ... load tiles, palettes, tilemap ... */

    bgSetMapPtr(0, 0x0000, SC_64x64);
    REG_TM = TM_BG1;
    setScreenOn();

    while (1) {
        u16 pad = padHeld(0);

        if (pad & KEY_LEFT)  scroll_x -= 2;
        if (pad & KEY_RIGHT) scroll_x += 2;
        if (pad & KEY_UP)    scroll_y -= 2;
        if (pad & KEY_DOWN)  scroll_y += 2;

        bgSetScroll(0, scroll_x, scroll_y);
        WaitForVBlank();
    }
    return 0;
}
```

The tilemap wraps automatically, so the player can scroll in any direction indefinitely on a SC_64x64 map (512x512 pixels). For a larger world, see Continuous Scrolling below.

## Mixed Scroll (Static + Moving Layers)

A common technique is scrolling one background while keeping another fixed. This works well for a moving pattern behind a static logo or HUD.

```c
s16 scrX = 0, scrY = 0;

while (1) {
    /* BG1 scrolls diagonally */
    scrX++;
    scrY++;
    bgSetScroll(0, scrX, scrY);

    /* BG2 stays fixed at (0, 0) - no bgSetScroll call needed */

    WaitForVBlank();
}
```

See `examples/graphics/backgrounds/mixed_scroll/` for a complete example where a shader pattern auto-scrolls behind a static logo.

## Parallax Scrolling

Parallax creates a depth illusion by scrolling background layers at different speeds. Distant layers move slowly; near layers move quickly.

### Simple Parallax (Multiple BGs)

The easiest approach uses separate BG layers, each scrolled at a different rate:

```c
s16 scroll_x = 0;

while (1) {
    scroll_x++;

    /* BG1 (foreground): full speed */
    bgSetScroll(0, scroll_x, 0);

    /* BG2 (background): half speed */
    bgSetScroll(1, scroll_x >> 1, 0);

    WaitForVBlank();
}
```

This is limited to the number of available BG layers (Mode 1 has 3 layers).

### HDMA Parallax (Per-Scanline Scroll)

For more zones than available BG layers, use HDMA to rewrite the scroll register at different scanline positions. This splits a single BG into multiple horizontal bands, each scrolling at its own speed.

```c
#include <snes.h>

/* HDMA table in RAM: [line_count] [scroll_lo] [scroll_hi] per zone */
u8 scroll_table[10];

int main(void) {
    consoleInit();

    /* ... load tiles, palette, tilemap ... */

    bgSetMapPtr(0, 0x0000, SC_64x32);
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    setScreenOn();

    /* Define 3 scroll zones */
    scroll_table[0] = 72;                  /* Top: 72 scanlines */
    *(u16 *)&scroll_table[1] = 0;          /* Initial offset */
    scroll_table[3] = 88;                  /* Middle: 88 scanlines */
    *(u16 *)&scroll_table[4] = 0;
    scroll_table[6] = 64;                  /* Bottom: 64 scanlines */
    *(u16 *)&scroll_table[7] = 0;
    scroll_table[9] = 0x00;               /* End of table */

    /* Start HDMA on channel 6, targeting BG1 horizontal scroll */
    hdmaParallax(HDMA_CHANNEL_6, 0, scroll_table);
    hdmaEnable(1 << HDMA_CHANNEL_6);

    while (1) {
        /* Each zone scrolls at a different speed */
        *(u16 *)&scroll_table[1] += 1;    /* Slow (sky) */
        *(u16 *)&scroll_table[4] += 2;    /* Medium (midground) */
        *(u16 *)&scroll_table[7] += 4;    /* Fast (foreground) */

        WaitForVBlank();
    }
    return 0;
}
```

See `examples/graphics/effects/parallax_scrolling/` for the complete implementation.

**Important:** The scroll table must live in RAM (not `const`) because the main loop updates it every frame. HDMA reads from the table during rendering, so always update the values *before* `WaitForVBlank()`.

## Continuous / Infinite Scrolling

For worlds larger than the tilemap (e.g., a side-scroller level), you must stream new tile columns or rows into VRAM as the player scrolls. The tilemap wraps in hardware, so you overwrite the column that just scrolled off-screen with the next column from your map data.

### The Streaming Pattern

1. Scroll the BG offset as normal
2. When the scroll crosses an 8-pixel tile boundary, DMA a new column (or row) of tilemap entries into VRAM
3. The PPU wraps the tilemap, so the newly written column appears seamlessly at the leading edge

The `continuous_scroll` example demonstrates this with player-controlled scrolling and a character sprite. Key elements from the example:

```c
typedef struct {
    s16 player_x;
    s16 player_y;
    s16 bg1_scroll_x;
    s16 bg1_scroll_y;
    s16 bg2_scroll_x;
    s16 bg2_scroll_y;
} GameState;

GameState game = {20, 100, 0, 32, 0, 32};
```

The main loop reads input, moves the player, and applies auto-scrolling when the player crosses a screen threshold:

```c
/* Auto-scroll when player reaches the edge of the visible area */
if (game.player_x > SCROLL_THRESHOLD_RIGHT && game.bg1_scroll_x < MAX_SCROLL_X) {
    game.bg1_scroll_x += 1;
    game.bg2_scroll_x += 1;
    game.player_x -= 1;      /* Push player back to stay centered */
}

/* Update hardware scroll */
bgSetScroll(0, game.bg1_scroll_x, game.bg1_scroll_y);
bgSetScroll(1, game.bg2_scroll_x, game.bg2_scroll_y);
```

See `examples/graphics/backgrounds/continuous_scroll/` for the full implementation including sprite setup, dual-layer parallax, and tilemap loading.

### Streaming Tips

- **DMA budget**: VBlank allows roughly 4KB of DMA. A single column of a 32-tile-high tilemap is 64 bytes (32 entries x 2 bytes), well within budget.
- **Stream +32 columns, not +31**: the 33rd partial tile at the right edge is visible, so always stream one column ahead.
- **Use SC_64x32 or SC_64x64**: wider tilemaps give you a 64-column circular buffer, reducing the streaming frequency.

## VBlank Timing

Scroll registers (`BG1HOFS`, `BG1VOFS`, etc.) are latched by the PPU at the start of each frame. Writing them during active display produces tearing or no visible effect.

OpenSNES handles this automatically: `bgSetScroll()` writes to shadow variables and sets a dirty flag. The NMI handler checks the dirty flag during VBlank and commits only the changed values to hardware. You do not need to manually time your scroll writes.

The typical frame loop is:

```c
while (1) {
    /* 1. Update game logic and scroll positions */
    scroll_x += 2;
    bgSetScroll(0, scroll_x, 0);

    /* 2. WaitForVBlank triggers NMI, which writes scroll to hardware */
    WaitForVBlank();
}
```

**Important:** Always set scroll values *before* `WaitForVBlank()`. If you set them after, the update is delayed by one frame.

Also set initial scroll values *before* `setScreenOn()` to avoid a single-frame glitch where the tilemap appears at (0, 0) before your intended position takes effect:

```c
bgSetScroll(0, initial_x, initial_y);
setScreenOn();   /* First visible frame already has correct scroll */
```

## Next Steps

- @ref tutorial_graphics "Graphics & Backgrounds"
- @ref tutorial_sprites "Sprites & Animation"
- @ref tutorial_input "Input & Controllers"
