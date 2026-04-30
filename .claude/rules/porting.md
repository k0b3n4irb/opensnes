---
paths:
  - "examples/**/*"
---

# PVSnesLib → OpenSNES Porting Guide

CRITICAL: Do NOT blindly copy PVSnesLib code. The init order, argument convention, and APIs differ.

## Argument Push Order

PVSnesLib (tcc816) pushes arguments **RIGHT-TO-LEFT** (C convention).
OpenSNES (cc65816) pushes arguments **LEFT-TO-RIGHT**.

This means ASM functions ported from PVSnesLib have **swapped stack offsets**:
- PVSnesLib: first arg at highest stack offset
- OpenSNES: first arg at **6,s** (after PHP+PHB), second at 8,s, etc.

## Initialization Order

PVSnesLib code often calls `setScreenOn()` too early. OpenSNES requires:

```
consoleInit() → setMode() → palettes → tiles → tilemap → BG pointers
→ sprites (if used) → text (if used) → setMainScreen() → setScreenOn()
```

`setScreenOn()` is ALWAYS last. Everything before it runs during force blank.

## API Mapping

| PVSnesLib | OpenSNES | Notes |
|-----------|----------|-------|
| `consoleInit()` | `consoleInit()` | Same |
| `setMode(BG_MODE1, 0)` | `setMode(BG_MODE1, 0)` | Same |
| `bgSetGfxPtr(0, 0x2000)` | `bgSetGfxPtr(0, 0x2000)` | Same |
| `bgSetMapPtr(0, 0x6800, SC_32x32)` | `bgSetMapPtr(0, 0x6800, SC_32x32)` | Same |
| `oamSet(...)` (many args) | `oamSet(...)` | CHECK arg order — push direction differs |
| `WaitForVBlank()` | `WaitForVBlank()` | Same |
| `consoleDrawText(x,y,str)` | `textPrintAt(x,y,str)` + `textFlush()` | Different API — must flush |

## gfx4snes Flags

Common conversion flags (based on PVSnesLib's gfx4snes):
```bash
gfx4snes -i sprite.png -p -t -s 16         # 16x16 sprites
gfx4snes -i bg.png -p -t -m                 # BG with tilemap
gfx4snes -i bg.png -p -t -m -R             # BG with tilemap, no tile reduction
```

## Common Porting Mistakes

| PVSnesLib pattern | Problem in OpenSNES | Fix |
|-------------------|---------------------|-----|
| `setScreenOn()` before all VRAM loads | Garbage on first frame | Move `setScreenOn()` to end |
| Stack offset 7,s for first arg | Wrong value (off by one) | First arg is at **6,s** |
| `consoleDrawText()` | API doesn't exist | Use `textPrintAt()` + `textFlush()` |
| Sprite palette at CGRAM 0 | Wrong colors | Sprite palettes start at CGRAM 128 |
