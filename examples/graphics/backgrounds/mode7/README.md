# Mode 7 — Rotation and Scaling

## What This Example Shows

How to use the SNES Mode 7 — a special background mode that applies an affine
transformation (rotation + scaling) to a single 128x128 tile layer. This is the
same technique behind F-Zero's ground, Mario Kart's tracks, and Pilotwings' maps.

## Prerequisites

Read `backgrounds/mode1` first (basic background setup). Mode 7 is fundamentally
different from tile-based modes but the initialization flow is similar.

## Controls

| Button | Action |
|--------|--------|
| A | Rotate clockwise |
| B | Rotate counter-clockwise |
| D-PAD Up | Zoom out (see more of the plane) |
| D-PAD Down | Zoom in (magnify) |

## How It Works

### 1. Mode 7 VRAM format

Mode 7 uses a special interleaved VRAM format:
- **Even bytes**: tilemap entries (128x128 = 16,384 tiles)
- **Odd bytes**: pixel data (256 tiles, 8x8, 256 colors each)

This is completely different from Modes 0-3 where tilemap and tile data are in
separate VRAM regions. The loading is handled by `asm_loadMode7Data()` in data.asm.

### 2. Initialize the transformation

```c
setMode(BG_MODE7, 0);
mode7Init();
mode7SetScale(0x0200, 0x0200);
mode7SetAngle(0);
```

`mode7Init()` sets the rotation center to the screen center (128, 128).
Scale `0x0200` = 2.0 in 8.8 fixed-point — this shows the map at a comfortable
zoom level.

### 3. Update the matrix every frame

```c
if (pad0 & KEY_A) {
    angle++;
    mode7SetAngle(angle);
}
```

`mode7SetAngle()` recalculates the 2x2 transformation matrix (registers M7A-M7D)
using the current angle and scale. The angle is 0-255, mapping to 0-360 degrees.

## SNES Concepts

- **Affine transformation**: Mode 7 applies a 2x2 matrix to BG1. The matrix encodes
  rotation, scaling, and skew. The hardware computes the transform per-scanline.
- **One layer only**: Mode 7 supports exactly one background (BG1). No BG2/BG3/BG4.
  Sprites still work normally on top.
- **256-color palette**: Each Mode 7 tile uses 256 colors (8bpp), unlike Modes 0-3
  which use 2/4/8 colors per palette. This gives richer visuals but limits you to
  256 unique tiles.
- **8.8 fixed-point scale**: `0x0100` = 1.0x (normal), `0x0200` = 2.0x (zoomed out),
  `0x0080` = 0.5x (zoomed in). Values below `0x0010` or above `0x0F00` look extreme.

## Next Steps

- `backgrounds/mode7_perspective` — Per-scanline scaling for pseudo-3D (F-Zero style)
- `effects/hdma_wave` — HDMA can modify Mode 7 parameters per scanline
