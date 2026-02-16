# Mode 7 Perspective — Pseudo-3D Ground Effect

## What This Example Shows

How to combine Mode 7 with HDMA to create a **perspective ground plane** — the same
technique used in F-Zero, Mario Kart, and Pilotwings. The screen is split in two:
a static sky on top and a scaled ground that recedes into the distance.

## Prerequisites

Read `backgrounds/mode7` first (Mode 7 basics), then `effects/hdma_wave` (HDMA
per-scanline programming).

## Controls

| Button | Action |
|--------|--------|
| D-PAD | Scroll the ground plane |

## How It Works

### 1. Split-screen with HDMA

The screen is divided by 4 HDMA channels running every frame:

| Channel | Target | Effect |
|---------|--------|--------|
| Ch1 | BGMODE ($2105) | Switches from Mode 3 (sky) to Mode 7 (ground) |
| Ch2 | TM ($212C) | Switches from BG2 (sky tiles) to BG1 (Mode 7 ground) |
| Ch3 | M7A ($211B) | Per-scanline horizontal scale (perspective) |
| Ch4 | M7D ($211E) | Per-scanline vertical scale (perspective) |

Top 96 scanlines: Mode 3 displays a static sky background on BG2.
Bottom 128 scanlines: Mode 7 displays the ground with per-scanline scaling.

### 2. Perspective math

The key to the 3D illusion: scanlines near the top of the ground area (the
"horizon") use a large scale value (small objects = far away), while scanlines
near the bottom use a small scale value (large objects = close up).

This is a pre-computed table in the assembly data — each of the 128 ground
scanlines has its own M7A/M7D scale pair.

### 3. Scrolling

```c
if (pad0 & KEY_LEFT)  sx--;
if (pad0 & KEY_RIGHT) sx++;
if (pad0 & KEY_UP)    sy--;
if (pad0 & KEY_DOWN)  sy++;
asm_setupHdmaPerspective(sx, sy);
```

The scroll values are passed to the assembly helper, which writes them to the
Mode 7 scroll registers (M7HOFS/M7VOFS) and re-enables the HDMA channels.

## SNES Concepts

- **Mode switching mid-frame**: The SNES allows changing BGMODE via HDMA between
  scanlines. This is how one screen shows two completely different video modes.
- **Per-scanline scaling**: By writing M7A/M7D every scanline via HDMA repeat mode,
  each horizontal line of the ground has a different scale — creating the depth illusion.
- **HDMA budget**: 4 simultaneous HDMA channels is feasible but uses significant
  bus time per scanline. Adding more channels risks running out of HBlank time.

## Next Steps

- `effects/gradient_colors` — Another HDMA technique (color gradients)
- `games/likemario` — Scrolling backgrounds with sprites (different approach)
