# Window — HDMA Triangle Masking

A triangle-shaped window mask driven by HDMA, showing how per-scanline window
boundary updates create non-rectangular shapes. Press **A**, **X**, or **B** to
apply the window to different background layers.

Ported from PVSnesLib "Window" example.

## Controls

| Button | Action |
|--------|--------|
| **A** | Window on BG1 only |
| **X** | Window on BG2 only |
| **B** | Window on both BG1 and BG2 |

## How It Works

### SNES Window System

The SNES PPU has two hardware windows — rectangular regions defined by left and
right X coordinates. These windows can mask (hide) portions of any background
layer or sprite layer. Window operations cost zero CPU time because they're
handled entirely by the PPU.

### Making Non-Rectangular Shapes with HDMA

A single window is always a horizontal band. But by changing the left/right
boundaries **every scanline** using HDMA, you can create any shape — circles,
triangles, or custom silhouettes.

This example uses two HDMA channels:
- **Channel 6** writes to WH0 ($2126) — window 1 left boundary
- **Channel 7** writes to WH1 ($2127) — window 1 right boundary

### The Triangle Tables

The HDMA tables define a diamond/triangle shape centered on screen:

```
Line  0-59:  WH0=255, WH1=0   -> left > right -> window disabled (no mask)
Line 60-91:  WH0 shrinks 127->96, WH1 grows 129->160 -> triangle expands
Line 92-123: WH0 grows 96->127, WH1 shrinks 160->129 -> triangle contracts
Line 124+:   WH0=255, WH1=0   -> window disabled again
```

The tables use **repeat mode** (bit 7 set in the line count) because each
scanline needs a different value. Non-repeat mode would write once and hold,
which doesn't work when the shape changes per line.

### Inversion: Show Inside

By default, TMW (main screen window) hides pixels **inside** the window.
We set the **invert** flag so that pixels **outside** the window are hidden
instead — revealing only the triangle-shaped area.

### Switching Layers

When you press A/X/B, the code:
1. Disables HDMA channels
2. Resets all window registers (`windowInit()`)
3. Enables window 1 on the new layer(s) with inversion
4. Sets TMW for the new layer(s)
5. Re-enables HDMA

## VRAM Layout

| Region | Content |
|--------|---------|
| $0000 | BG1 tilemap (32x32) |
| $1000 | BG2 tilemap (32x32) |
| $4000 | BG1 tiles (4bpp, 16 colors) |
| $6000 | BG2 tiles (4bpp, 16 colors) |

## Key Registers

| Register | Value | Purpose |
|----------|-------|---------|
| $2123 (W12SEL) | varies | Enable + invert window 1 per BG |
| $212E (TMW) | varies | Main screen window mask |
| $2126 (WH0) | HDMA | Window 1 left boundary |
| $2127 (WH1) | HDMA | Window 1 right boundary |

## Files

| File | Purpose |
|------|---------|
| `main.c` | HDMA triangle tables, window setup, input loop |
| `data.asm` | Two backgrounds (BG1 + BG2) graphics data |
| `res/bg1.png` | Background 1 (palette slot 0) |
| `res/bg2.png` | Background 2 (palette slot 1) |
