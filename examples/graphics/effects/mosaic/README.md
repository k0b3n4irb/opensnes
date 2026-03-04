# Mosaic -- PPU Pixel Block Effect

![Screenshot](screenshot.png)

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

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/graphics/effects/mosaic
```

Then open `mosaic.sfc` in your emulator (Mesen2 recommended).

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
master brightness -- 15 is full, 0 is black. Two VBlank waits per step gives a
smooth ~0.5 second transition.

### 2. Mosaic effect (pixelation)

```c
void doMosaicOut(void) {
    mosaicEnable(MOSAIC_BG1);
    mosaicFadeOut(3);
}
```

The library functions `mosaicEnable()` and `mosaicFadeOut()` write to register
$2106 (MOSAIC). The parameter `3` controls the speed of the transition.

Register $2106 layout:
- **Bits 7-4**: Block size (0 = 1x1 pixel, 15 = 16x16 pixel blocks)
- **Bits 3-0**: Which backgrounds are affected (bit 0 = BG1, bit 1 = BG2, etc.)

Increasing the block size from 0 to 15 makes pixels progressively larger until
the image is a grid of colored squares.

### 3. Combining both for transitions

Classic SNES transition pattern used in many RPGs:
1. Mosaic out (image becomes blocky)
2. Fade to black (blocky image fades)
3. Load new scene during black
4. Fade in (new scene appears, still blocky)
5. Mosaic in (blocks shrink to reveal the scene)

This gives a polished "dissolve" effect.

## SNES Concepts

### Mosaic Register ($2106)

The SNES PPU provides per-background pixelation with 16 levels. The effect is
applied entirely in hardware -- zero CPU cost. Each BG layer can be independently
mosaic'd by setting its corresponding bit in the low nibble.

### Not a Blur

Mosaic replaces pixels with the top-left pixel of each NxN block. It is a
decimation effect, not a filter. The result looks like a low-resolution version
of the original image.

### Sprites Are Unaffected

Mosaic only applies to background layers. Sprites always render at full resolution.
Some games use this creatively -- mosaic the BG while sprites remain sharp during
a transition, making characters stand out against the dissolving background.

## Project Structure

| File | Purpose |
|------|---------|
| `main.c` | Fade/mosaic transition logic, button-driven state machine |
| `data.asm` | Background tiles, tilemap, and palette data |
| `res/opensnes.png` | Source background image |
| `Makefile` | `LIB_MODULES := console dma background sprite input mosaic` |

## Going Further

- **Scene transition**: Load two different backgrounds. Use the fade/mosaic
  combination to transition between them -- mosaic out scene A, load scene B
  tiles during force blank, mosaic in scene B.

- **Per-layer mosaic**: Enable mosaic on BG1 but not BG2. This lets you pixelate
  the foreground while the background stays sharp, or vice versa.

- **Explore related examples**:
  - `effects/fading` -- Brightness fading in detail
  - `effects/hdma_wave` -- More advanced PPU effects with HDMA
