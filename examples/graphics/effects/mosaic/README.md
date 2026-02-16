# Mosaic — PPU Pixel Block Effect

## What This Example Shows

How to use the SNES mosaic register to create a pixelation effect, and how to combine
it with brightness fading for screen transitions. Press any button to cycle through
4 transition steps: fade out, fade in, mosaic out, mosaic in.

## Prerequisites

Read `effects/fading` first (brightness control basics).

## Controls

| Button | Action |
|--------|--------|
| Any button | Advance to next transition step |

The cycle repeats: fade out -> fade in -> mosaic out -> mosaic in -> (loop)

## How It Works

### 1. Fade effect (brightness)

```c
void doFadeOut(void) {
    for (i = 15; i > 0; i--) {
        setBrightness(i);
        WaitForVBlank(); WaitForVBlank();
    }
    setBrightness(0);
}
```

`setBrightness()` writes to register $2100 (INIDISP). Values 0-15 control the
master brightness — 15 is full, 0 is black. Two VBlank waits per step gives a
smooth ~0.5 second transition.

### 2. Mosaic effect (pixelation)

```c
void doMosaicOut(void) {
    for (i = 0; i < 16; i++) {
        REG_MOSAIC = (i << 4) | MOSAIC_BG1;
    }
}
```

Register $2106 (MOSAIC) controls the effect:
- **Bits 7-4**: Block size (0 = 1x1 pixel, 15 = 16x16 pixel blocks)
- **Bits 3-0**: Which backgrounds are affected (bit 0 = BG1, etc.)

Increasing the block size from 0 to 15 makes pixels progressively larger until
the image is a grid of colored squares.

### 3. Combining both for transitions

Classic SNES transition pattern:
1. Mosaic out (image becomes blocky)
2. Fade to black (blocky image fades)
3. *Load new scene during black*
4. Fade in (new scene appears, still blocky)
5. Mosaic in (blocks shrink to reveal the scene)

This gives a polished "dissolve" effect used in many RPGs.

## SNES Concepts

- **Mosaic register ($2106)**: Per-background pixelation with 16 levels. Applied by
  the PPU in hardware — zero CPU cost. Each BG can be independently mosaic'd.
- **Not a blur**: Mosaic replaces pixels with the top-left pixel of each block.
  It's a decimation effect, not a filter. The result looks like a low-resolution
  version of the original image.
- **Sprites are unaffected**: Mosaic only applies to background layers. Sprites always
  render at full resolution. Some games use this creatively (mosaic the BG while
  sprites remain sharp).

## Next Steps

- `effects/fading` — Brightness fading in detail
- `effects/hdma_wave` — More advanced PPU effects with HDMA
