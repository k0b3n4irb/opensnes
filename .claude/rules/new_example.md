# New Example Checklist (Auto-loaded)

## MANDATORY: Before writing OR PORTING any example

1. **Read a working example** that uses the same features. Copy its init sequence.
   Do NOT write from memory — the init order matters and getting it wrong
   produces black screens or VRAM garbage.

   **When porting from PVSnesLib: do NOT blindly copy the PVSnesLib init order.**
   PVSnesLib code may violate our rules (e.g., `setScreenOn()` before all VRAM
   is ready). Always reorder the ported code to match OUR init sequence below.
   The rules apply to ALL code — new or ported.

2. **Consult docs/** — it is the source of truth for all SNES concepts:
   - Graphics & backgrounds: `docs/tutorials/graphics.md`, `docs/SNES_GRAPHICS_GUIDE.md`
   - Sprites & animation: `docs/tutorials/sprites.md`, `docs/tutorials/animation.md`
   - Scrolling & parallax: `docs/tutorials/scrolling.md`
   - Collision detection: `docs/tutorials/collision.md`
   - Input handling: `docs/tutorials/input.md`
   - Audio & music: `docs/tutorials/audio.md`, `docs/SNES_SOUND_GUIDE.md`
   - Game states: `docs/tutorials/game_states.md`
   - Hardware registers: `docs/hardware/REGISTERS.md`
   - Memory map: `docs/hardware/MEMORY_MAP.md`
   - OAM / sprites: `docs/hardware/OAM.md`
   - Troubleshooting: `docs/TROUBLESHOOTING.md`
   - Code style: `docs/CODE_STYLE.md`
   - Getting started: `docs/GETTING_STARTED.md`

## Initialization Order

The exact init sequence depends on the example type. Find it in the
relevant tutorial above. The universal rule is:

```
consoleInit() → setMode() → palettes → tiles → tilemap → BG pointers
→ sprites (if used) → text (if used) → setMainScreen() → setScreenOn()
```

`setScreenOn()` is ALWAYS last. Everything before it runs during force blank.

## Common Mistakes

| Mistake | Symptom | Fix |
|---------|---------|-----|
| Forget `consoleInit()` | VRAM garbage | Always call first |
| `setScreenOn()` before VRAM setup | Flicker, garbage frame | Screen on LAST |
| DMA outside VBlank | Data silently lost | `WaitForVBlank()` before DMA |
| Wrong text API | Compile error | `textInit`+`textPrintAt`+`textFlush` |
| Text without `textFlush()` | Text invisible | `textFlush()` triggers DMA |
| `textLoadFont4bpp` without module | Link error | Add `text4bpp` to LIB_MODULES |
| Sprite palette at CGRAM 0 | Wrong colors | Sprite palettes start at CGRAM 128 |
| `mapLoad()` called after `setScreenOn()` | Garbage tiles on first frame | `mapLoad()` before `setScreenOn()` — it flushes VRAM internally |
| Missing LIB_MODULES | Link error | Match `#include` with module |
| `volatile` in loops | QBE SSA crash | Use globals instead |
| ASM stack offset 7,s | Works for arg=0 only | First arg is at **6,s** after PHP+PHB |

## Makefile

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET     := example_name.sfc
ROM_NAME   := EXAMPLE NAME
USE_LIB    := 1
LIB_MODULES := console dma background    # only link what you use
CSRC       := main.c
include $(OPENSNES)/make/common.mk
```

For LIB_MODULES combinations, see `docs/GETTING_STARTED.md`.

## Doxygen Documentation

Every new example main.c MUST have:
1. `@file` header with `@brief`, `@ingroup examples`, `@par SNES Concepts`,
   `@par What to Observe`, `@par Modules Used`, `@see`
2. `/** @brief */` on every function, define, global, and data array
3. Pedagogical comments explaining WHY, not just WHAT

See existing examples in `docs/examples_group.md` for the format.

## Before Committing

1. `make clean && make` — full rebuild must pass
2. `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick`
3. Ask user for Mesen2 validation
4. No `Co-Authored-By` in commit message
