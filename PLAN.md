# OpenSNES Development Plan

## Goal
Enable SNES game development in **pure C** without requiring assembly knowledge.

Users should be able to write:
```c
#include <snes.h>

int main(void) {
    consoleInit();
    oamInit();

    while (1) {
        padUpdate();
        if (padHeld(0) & KEY_RIGHT) player_x++;

        oamSet(0, player_x, player_y, tile, 0, 3, 0);

        WaitForVBlank();
        oamUpdate();
    }
}
```

---

## Current State

### What Works
| Component | Status |
|-----------|--------|
| Compiler (cproc + qbe) | Binaries exist in `bin/` |
| Assembler (wla-65816) | Working |
| Linker (wlalink) | Working |
| 5 Example ROMs | Working (but don't use library) |
| Test framework (snesdbg) | Working |

### What Doesn't Work
| Component | Issue |
|-----------|-------|
| Library build | Compiler output not compatible with wla-65816 |
| Examples | Use direct registers/ASM instead of library |

### Root Cause
The `cc65816` compiler generates assembly that isn't fully compatible with wla-65816:
- `.dsb` directive syntax differs
- Some directives need conversion (`.byte` → `.db`, etc.)
- Post-processing in Makefile is incomplete

---

## Phase 1: Fix Compiler Output

### 1.1 Analyze Compiler Output
- [ ] Compile a simple C file and examine output
- [ ] Identify all wla-65816 incompatibilities
- [ ] Document required transformations

### 1.2 Fix Post-Processing
- [ ] Update `lib/Makefile` sed transformations
- [ ] Handle `.dsb` directive properly
- [ ] Handle all wla-65816 syntax differences
- [ ] Create reusable asm-fix script

### 1.3 Verify Compiler Works
- [ ] Compile `lib/source/console.c` successfully
- [ ] Compile `lib/source/input.c` successfully
- [ ] Compile `lib/source/sprite.c` successfully
- [ ] Create minimal test case: "Hello from C library"

**Test:** `make -C lib` succeeds and produces `libopensnes.a`

---

## Phase 2: Build the Library

### 2.1 Complete Library Build
- [ ] Fix all compilation errors in library sources
- [ ] Build `libopensnes.a` archive
- [ ] Install library to `bin/`

### 2.2 Fix crt0.asm Integration
- [ ] Ensure crt0.asm provides required symbols (`vblank_flag`, `frame_count`)
- [ ] Verify NMI handler integration
- [ ] Test startup sequence with library

### 2.3 Verify Library Functions
Create minimal test ROMs for each module:
- [ ] `test_console.c` - Test `consoleInit()`, `WaitForVBlank()`
- [ ] `test_input.c` - Test `padUpdate()`, `padPressed()`, `padHeld()`
- [ ] `test_sprite.c` - Test `oamInit()`, `oamSet()`, `oamUpdate()`

**Test:** All test ROMs boot and function correctly in Mesen2

---

## Phase 3: Rewrite Examples Using Library

### 3.1 hello_world (Pure C, No ASM)
**Before:** Direct register access
**After:** Library calls

```c
#include <snes.h>

int main(void) {
    consoleInit();
    // Use library text functions
    textInit();
    textPrint(10, 14, "HELLO WORLD!");
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
}
```

- [ ] Rewrite using library functions
- [ ] Verify text displays correctly
- [ ] Update README.md
- [ ] Update test_hello_world.lua

### 3.2 custom_font (Pure C, No ASM)
- [ ] Rewrite using library functions
- [ ] Test custom font loading
- [ ] Update README.md
- [ ] Update test_custom_font.lua

### 3.3 animation (Pure C with Minimal ASM)
**Goal:** Eliminate custom ASM helpers, use library

```c
#include <snes.h>

int main(void) {
    consoleInit();
    oamInitEx(OBJ_SIZE16_L32, 0);

    // Load sprites using library DMA
    dmaCopyVRAM(sprites_tiles, 0x0000, sizeof(sprites_tiles));
    dmaCopyCGRAM(sprites_pal, 128, sizeof(sprites_pal));

    while (1) {
        padUpdate();

        if (padHeld(0) & KEY_RIGHT) monster_x++;
        if (padHeld(0) & KEY_LEFT) monster_x--;

        oamSet(0, monster_x, monster_y, tile, 0, 3, flipx ? 0x40 : 0);

        WaitForVBlank();
        oamUpdate();
    }
}
```

- [ ] Rewrite using library sprite functions
- [ ] Remove helpers.asm (or reduce to data only)
- [ ] Verify animation works
- [ ] Update test_animation.lua

### 3.4 tone (C with SPC ASM)
**Note:** SPC700 audio driver must remain in ASM (different CPU)

```c
#include <snes.h>
#include <snes/audio.h>  // Future addition

int main(void) {
    consoleInit();
    spcInit();

    while (1) {
        padUpdate();

        if (padPressed(0) & KEY_A) {
            spcPlay();
        }

        WaitForVBlank();
    }
}
```

- [ ] Keep SPC driver in ASM (required)
- [ ] Rewrite main.c to use library
- [ ] Create audio.h wrapper for SPC functions
- [ ] Update test_tone.lua

### 3.5 calculator (Pure C)
**Challenge:** Currently 99% ASM

- [ ] Rewrite calculator logic in C
- [ ] Use library for input, display, VBlank
- [ ] Keep arithmetic in C (demonstrates 65816 math)
- [ ] Update test_calculator.lua

**Test:** All 5 examples pass automated tests using library

---

## Phase 4: Automated Testing

### 4.1 Library Unit Tests
Create Lua tests for each library function:

```
tests/
├── lib/
│   ├── test_console.lua
│   ├── test_input.lua
│   ├── test_sprite.lua
│   └── test_dma.lua
└── examples/
    ├── hello_world/
    ├── custom_font/
    ├── animation/
    ├── tone/
    └── calculator/
```

- [ ] Test consoleInit() sets correct registers
- [ ] Test WaitForVBlank() waits for NMI
- [ ] Test padPressed() detects new presses
- [ ] Test padHeld() detects held buttons
- [ ] Test oamSet() writes correct OAM data
- [ ] Test oamUpdate() triggers DMA

### 4.2 Integration Tests
- [ ] Build all examples from clean state
- [ ] Run symmap to verify no WRAM overlaps
- [ ] Run all Mesen2 tests
- [ ] Verify ROM headers are correct

### 4.3 CI Pipeline (Future)
- [ ] GitHub Actions workflow
- [ ] Build on push/PR
- [ ] Run tests with headless Mesen2

---

## Phase 5: Documentation

### 5.1 API Documentation
- [ ] Complete Doxygen comments in all headers
- [ ] Generate HTML documentation
- [ ] Add usage examples to each function

### 5.2 Tutorial
- [ ] "Getting Started" guide
- [ ] "Your First SNES Game" tutorial
- [ ] Explain what each library function does
- [ ] Show common patterns (game loop, input, sprites)

### 5.3 Example README Updates
Each example should document:
- What it demonstrates
- Library functions used
- How to modify/extend it

---

## Success Criteria

1. **Library compiles:** `make -C lib` produces `libopensnes.a`
2. **Examples use library:** No direct register access in C code
3. **Examples are Pure C:** Minimal or no custom ASM (except SPC)
4. **Tests pass:** All automated tests pass in Mesen2
5. **No WRAM overlaps:** symmap reports clean for all examples

---

## Execution Order

```
Phase 1: Fix Compiler Output
    │
    ▼
Phase 2: Build the Library
    │
    ▼
Phase 3: Rewrite Examples ──────┐
    │                           │
    │   3.1 hello_world         │
    │   3.2 custom_font         │
    │   3.3 animation           │ (can be parallel)
    │   3.4 tone                │
    │   3.5 calculator          │
    │                           │
    ▼───────────────────────────┘
Phase 4: Automated Testing
    │
    ▼
Phase 5: Documentation
```

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Compiler bugs | Keep working ASM backup, report issues upstream |
| Library too slow | Profile critical paths, optimize in ASM if needed |
| Missing library features | Add as needed, document gaps |
| Test flakiness | Add timeouts, retry logic, better synchronization |

---

## Notes

- **Preserve working state:** Always commit before major changes
- **Incremental progress:** Get each phase working before next
- **Test early, test often:** Run tests after each change
- **Document everything:** Update docs as code changes
