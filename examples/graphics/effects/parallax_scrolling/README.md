# Parallax Scrolling

Multi-speed horizontal scrolling using HDMA — one background image, three scroll zones.

## What This Example Shows

- Splitting the screen into horizontal zones with different scroll speeds
- Using HDMA to update BG1HOFS per scanline group
- Creating a depth illusion with a single background layer
- Updating HDMA table values at runtime from C

## How It Works

### The Parallax Technique

Real parallax scrolling uses multiple BG layers at different speeds. This example
achieves the same depth effect with a **single layer** by using HDMA to change the
horizontal scroll register at different screen positions.

The screen is divided into three zones:

| Zone | Scanlines | Speed | Visual |
|------|-----------|-------|--------|
| Top | 72 lines | +1 px/frame | Stars/sky (distant, slow) |
| Middle | 88 lines | +2 px/frame | Midground pattern |
| Bottom | 64 lines | +4 px/frame | Grass/ground (close, fast) |

### HDMA Scroll Table

The HDMA table lives in RAM so it can be updated each frame:

```
Byte 0: 72          Line count (top zone)
Byte 1-2: scroll    16-bit horizontal offset
Byte 3: 88          Line count (middle zone)
Byte 4-5: scroll    16-bit horizontal offset
Byte 6: 64          Line count (bottom zone)
Byte 7-8: scroll    16-bit horizontal offset
Byte 9: 0x00        End marker
```

HDMA mode `1REG_2X` writes 2 bytes to BG1HOFS ($210D) — the low byte and
high byte of the scroll position. The PPU latches scroll values on each
write, so the HDMA effectively splits the screen at the specified line counts.

### Why This Works

BG scroll registers (BG1HOFS, etc.) are "write-twice" latched registers.
HDMA writes the two bytes of the scroll value automatically each scanline group,
maintaining the correct scroll position for that zone. When the line count
expires, HDMA advances to the next entry and writes the new scroll value.

## Key Concepts

- **HDMA** (Horizontal-blanking DMA): transfers data to PPU registers once per
  scanline during HBlank, enabling per-scanline effects
- **BG1HOFS** ($210D): BG1 horizontal scroll offset (write-twice register)
- **Parallax**: visual depth effect where closer objects scroll faster
- **64x32 tilemap**: 512 pixels wide — enough for seamless horizontal wrapping

## Building and Running

```bash
make -C examples/graphics/effects/parallax_scrolling
# Open parallax_scrolling.sfc in Mesen2
```

The background scrolls automatically in three horizontal bands at different speeds.

## Ported From

PVSnesLib `snes-examples/graphics/Effects/ParallaxScrolling/` by Alekmaul.
Adapted to use OpenSNES `hdmaParallax()` API instead of PVSnesLib `setParallaxScrolling()`.
