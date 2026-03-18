# New Example Checklist (Auto-loaded)

## MANDATORY: Before writing ANY new example

Read an existing working example that uses the same features. Copy its
initialization sequence. Do NOT write from memory — the init order matters
and getting it wrong produces black screens or VRAM garbage.

## Initialization Sequence

Every example follows this order. Steps can be skipped if not needed,
but the ORDER must be preserved.

```c
int main(void) {
    // 1. HARDWARE INIT — always first, clears VRAM/OAM/CGRAM
    consoleInit();

    // 2. VIDEO MODE — before any VRAM setup
    setMode(BG_MODE1, 0);  // or BG_MODE0, BG_MODE3, etc.

    // 3. PALETTE — set colors while screen is off
    setColor(0, 0x0000);            // or dmaCopyCGram()
    dmaCopyCGram(palette, 0, 32);   // BG palette
    dmaCopyCGram(sprpal, 128, 32);  // Sprite palette (CGRAM 128+)

    // 4. TILES — load tile graphics to VRAM
    WaitForVBlank();                // ensure VBlank for safe DMA
    dmaCopyVram(tiles, 0x0000, size);

    // 5. TILEMAP — load or build tilemap in VRAM
    dmaCopyVram(tilemap, 0x0400, size);

    // 6. BG POINTERS — tell PPU where tiles and tilemap are
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x0400, SC_32x32);

    // 7. SPRITES (if used)
    oamInit();  // or oamInitEx() for custom sizes
    // Load sprite tiles to VRAM (typically $4000+)
    // Set REG_OBJSEL for sprite size and name table base

    // 8. TEXT MODULE (if used)
    textInit();                     // allocate off-screen buffer
    textLoadFont(0x0000);           // 2bpp font to VRAM
    // bgSetGfxPtr/bgSetMapPtr for text BG layer

    // 9. SCREEN ENABLE — which layers are visible
    setMainScreen(LAYER_BG1);       // or REG_TM = TM_BG1 | TM_OBJ;

    // 10. SCREEN ON — always LAST, after all setup
    WaitForVBlank();                // sync before turning on
    setScreenOn();

    // 11. MAIN LOOP
    while (1) {
        WaitForVBlank();
        // game logic, input, sprite updates...
    }
    return 0;
}
```

## Common Mistakes (that I keep making)

| Mistake | Symptom | Fix |
|---------|---------|-----|
| Forget `consoleInit()` | VRAM garbage, random tiles | Always call first |
| `setScreenOn()` before VRAM setup | Flicker, garbage frame | Screen on LAST |
| DMA outside VBlank | Data silently lost | `WaitForVBlank()` before `dmaCopyVram()` |
| Wrong text API | Compile error | Use `textInit`+`textPrintAt`+`textFlush`, NOT `consolePrintString` |
| Text without `textFlush()` | Text invisible | `textFlush()` triggers DMA to VRAM |
| `textLoadFont4bpp` without module | Link error | Add `text4bpp` to LIB_MODULES |
| Sprite palette at CGRAM 0 | Wrong colors | Sprite palettes start at CGRAM 128 |
| Missing LIB_MODULES | Link error | Match every `#include <snes/X.h>` with module X |
| `volatile` in loops | QBE SSA crash | Use globals instead of `volatile` locals |

## Makefile Template

```makefile
OPENSNES := $(shell cd ../../.. && pwd)   # adjust depth to match location

TARGET     := example_name.sfc
ROM_NAME   := EXAMPLE NAME

USE_LIB    := 1
LIB_MODULES := console dma background    # only link what you use

CSRC       := main.c

include $(OPENSNES)/make/common.mk
```

## LIB_MODULES Quick Reference

| Feature | Modules needed |
|---------|---------------|
| Basic BG display | `console dma background` |
| BG + sprites | `console dma background sprite` |
| BG + sprites + input | `console dma background sprite input` |
| Text overlay (2bpp) | add `text` |
| Text on 4bpp BG | add `text text4bpp` |
| HDMA effects | add `hdma` |
| Audio (SNESMOD) | add `snesmod` (no audio module name) |
| Collision | add `collision` |
| Mode 7 | add `mode7` |
| Profiling | add `profile` |
| Save game | add `sram` |

## Assembly Function Stack Offsets

When writing assembly functions called from C (via JSL):

```
After PHP + PHB inside the function:
  1,s = saved B (PHB)
  2,s = saved P (PHP)
  3,4,5,s = return address (JSL, 3 bytes)
  6,s = first argument (low byte)
  7,s = first argument (high byte)
```

**First 16-bit argument is at 6,s** (not 7,s). Getting this wrong makes
the function work for argument=0 but fail for any other value.

## Doxygen Documentation

Every new example main.c MUST have:
1. `@file` header with `@brief`, `@ingroup examples`, `@par SNES Concepts`,
   `@par What to Observe`, `@par Modules Used`, `@see`
2. `/** @brief */` on every function (including static)
3. `/** @brief */` or `/**< */` on important defines, globals, and data arrays
4. Pedagogical comments explaining WHY, not just WHAT

## Before Committing

1. `make clean && make` — full rebuild must pass
2. Test in opensnes-emu: `node test/run-all-tests.mjs --quick`
3. Test in Mesen2 — ask user for visual validation
4. Verify no `Co-Authored-By` in commit message
