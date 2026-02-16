# Transparent Window — Color Math + HDMA

A semi-transparent darkened rectangle rendered over a background image using
SNES color math and HDMA-driven window boundaries. No input — the effect is
static.

Ported from PVSnesLib "TransparentWindow" example by Digifox.

## How It Works

### The Technique

This combines three SNES hardware features:

1. **HDMA** changes the window boundaries per scanline to define a rectangle
2. **Window masking** restricts color math to the rectangular region
3. **Color math** subtracts a fixed color inside the window, darkening pixels

### Window for Color Math (Not for BG Masking)

The key insight: windows can control **where color math applies** independently
from where they mask background layers.

- `windowEnable(WINDOW_1, WINDOW_MATH)` enables the window for color math
- TMW is left at 0 — no background layers are masked by the window
- The background is fully visible everywhere
- But inside the window rectangle, color math darkens the pixels

### HDMA Rectangle

Two HDMA channels define the rectangle shape:

```
Lines  0-95:  WH0=255, WH1=0  -> window disabled (no darkening)
Lines 96-207: WH0=40,  WH1=216 -> rectangle from x=40 to x=216
Lines 208+:   WH0=255, WH1=0  -> window disabled again
```

Since WH0/WH1 registers hold their value (unlike scroll registers),
**non-repeat mode** is used — one write per segment, held for N scanlines.

### Color Math Configuration

| Register | Value | Effect |
|----------|-------|--------|
| CGWSEL ($2130) | condition=INSIDE | Apply math inside window only |
| CGWSEL ($2130) | source=FIXED | Use fixed color (not subscreen) |
| CGADSUB ($2131) | op=SUB, BG2 | Subtract from BG2 |
| COLDATA ($2132) | R=12, G=12, B=12 | Subtract intensity 12 (moderate darkening) |

## VRAM Layout

| Region | Content |
|--------|---------|
| $0000 | BG2 tilemap (32x32) |
| $4000 | BG2 tiles (4bpp, 16 colors) |

## Files

| File | Purpose |
|------|---------|
| `main.c` | HDMA tables, color math setup, window config |
| `data.asm` | Background graphics data |
| `res/background.png` | Source background image |
