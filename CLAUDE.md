# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenSNES is a modern, open-source SDK for Super Nintendo (SNES) development, forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib) with a focus on **ease of use** and **developer experience**.

### Mission

Make SNES development accessible to anyone with:
- Basic C programming knowledge
- Curiosity about how the SNES hardware works

The SDK should feel familiar to modern developers while respecting SNES constraints. No arcane setup, no cryptic errors, no guessing what went wrong.

### What Makes OpenSNES Different

| PVSnesLib | OpenSNES |
|-----------|----------|
| tcc-based compiler | QBE-based compiler (cc65816) with better diagnostics |
| Complex build setup | Simple `make` with sensible defaults |
| Scattered documentation | Consolidated guides and working examples |
| Trial-and-error debugging | symmap overlap checker, black screen tests |
| Windows-centric | Cross-platform (Linux, macOS, Windows) |

### Current Status: v0.1.0 (Alpha)

The SDK is functional and approaching production-ready. All core modules work, 30 examples build and run, CI passes on all platforms. See [ROADMAP.md](ROADMAP.md) for detailed status.

### Components

- **C Compiler (cc65816)**: QBE-based compiler targeting the 65816 CPU
- **Assembler (WLA-DX)**: Industry-standard assembler for 65816 and SPC700
- **Hardware Library**: Clean C API for PPU, input, DMA, sprites, text, and audio
- **Asset Tools**: gfx4snes, font2snes, smconv

## Build Commands

```bash
# Clone with submodules (required for compiler toolchain)
git clone --recursive https://github.com/OpenSNES/opensnes.git
cd opensnes

make                    # Build everything (compiler, tools, library, examples)
make compiler           # Build cc65816 (cproc + QBE w65816 backend) and WLA-DX
make tools              # Build asset tools (gfx4snes, font2snes)
make lib                # Build OpenSNES library
make examples           # Build all example ROMs
make tests              # Build test ROMs
make docs               # Build Doxygen documentation
make clean              # Clean all build artifacts

# Build a single example
cd examples/text/1_hello_world && make
make -C examples/graphics/5_mode1  # Alternative: build from any directory

# Check for memory overlaps after building
python3 tools/symmap/symmap.py --check-overlap game.sym
```

## Testing

### Local Validation (Run Before Every Commit)

```bash
# Full build - must pass
make clean && make

# Verify example count (CI expects 30)
find examples -name "*.sfc" | wc -l

# Run compiler tests
./tests/compiler/run_tests.sh

# Validate all examples (memory overlaps, ROM sizes)
./tests/examples/validate_examples.sh --quick

# MANDATORY: Black screen test (requires Mesen2)
./tests/run_black_screen_tests.sh /path/to/Mesen
```

### Black Screen Test (Mandatory)

The black screen test detects broken ROMs by checking if they display content after initialization. A black screen usually indicates a broken ROM (99% of cases).

```bash
# Run on all examples
./tests/run_black_screen_tests.sh /path/to/Mesen

# With verbose output
./tests/run_black_screen_tests.sh /path/to/Mesen --verbose

# Save screenshots for inspection
./tests/run_black_screen_tests.sh /path/to/Mesen --save-screenshots
```

**How it works:**
1. Loads each ROM in Mesen2
2. Waits 90 frames (1.5 seconds) for initialization
3. Takes a screenshot and measures PNG size
4. PASS if PNG > 300 bytes (screen has content)
5. FAIL if PNG < 300 bytes (black screen compresses very small)

**Failed screenshots** are saved to `tests/screenshots/FAIL_*.png` for debugging.

### CI Pipeline

Workflow: `.github/workflows/opensnes_build.yml`

Runs on push to `main`/`develop` and all PRs. Tests on Linux, Windows (MSYS2), macOS.

### Mesen2 Tests

```bash
./tests/run_tests.sh /path/to/Mesen

# Test a single ROM
cd /path/to/Mesen && ./Mesen /path/to/rom.sfc --testrunner --lua /path/to/test.lua
```

## Architecture

### Toolchain Pipeline

```
C Code → cproc → QBE IR → w65816 backend → 65816 Assembly (.asm)
                                                    ↓
                                          wla-65816 (assembler)
                                                    ↓
                                          wlalink (linker)
                                                    ↓
                                                .sfc ROM
```

### Directory Structure

- `compiler/` - Compiler toolchain (cproc, qbe, wla-dx as git submodules with fork/upstream remotes)
- `lib/` - OpenSNES library (headers in `include/snes/`, source in `source/`)
- `examples/` - Working examples organized by category (text, graphics, audio, basics, game)
- `templates/common/` - Shared runtime files (crt0.asm, hdr.asm, runtime.asm)
- `tools/` - Asset converters (gfx4snes, font2snes, smconv) and debugging tools (symmap)
- `tests/` - Test infrastructure with Mesen2 Lua integration
- `.claude/skills/` - Automation skills (build, test, new-example, analyze-rom, spc700-audio, submodules)

### Git Submodules

The compiler toolchain uses git submodules with a fork/upstream structure:

| Submodule | Origin (fork) | Upstream (original) |
|-----------|---------------|---------------------|
| compiler/wla-dx | k0b3n4irb/wla-dx | vhelin/wla-dx |
| compiler/cproc | k0b3n4irb/cproc | ~mcf/cproc (sr.ht) |
| compiler/qbe | k0b3n4irb/qbe | c9x.me/git/qbe |

Use `/submodules check` to see upstream updates, `/submodules sync <name>` to merge them.

### Build System

Examples use `make/common.mk` which provides standard build rules. Key variables:

| Variable | Purpose |
|----------|---------|
| `TARGET` | Output ROM filename |
| `ROM_NAME` | 21-character ROM name for header |
| `USE_LIB=1` | Link with OpenSNES library |
| `LIB_MODULES` | Library modules to link (e.g., `console sprite dma background`) |
| `USE_SRAM=1` | Enable SRAM in ROM header (for save games) |
| `USE_SNESMOD=1` | Enable SNESMOD audio (tracker music) |
| `SOUNDBANK_SRC` | Impulse Tracker files for SNESMOD |

### Example Makefile

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := mygame.sfc
ROM_NAME := "MY GAME              "

CSRC     := main.c
ASMSRC   := data.asm
USE_LIB  := 1
LIB_MODULES := console sprite dma background

include $(OPENSNES)/make/common.mk
```

### Library Modules

| Module | Key Functions |
|--------|---------------|
| console | `consoleInit()`, `setMode()`, `setScreenOn()`, `WaitForVBlank()` |
| sprite | `oamInit()`, `oamSet()`, `oamUpdate()`, `oamHide()`, `oamInitGfxSet()` |
| input | `padUpdate()`, `padPressed()`, `padHeld()`, `padReleased()` |
| background | `bgSetMapPtr()`, `bgSetScroll()`, `bgInitTileSet()`, `bgSetGfxPtr()` |
| dma | `dmaCopyVram()`, `dmaCopyCGram()`, `dmaCopyOam()` |
| interrupt | `nmiSet()`, `nmiSetBank()` (VBlank callbacks) |
| audio | `audioInit()`, `audioPlaySample()` |
| snesmod | `snesmodInit()`, `snesmodPlay()`, `snesmodProcess()` |

**Note:** `sprite` requires `dma` (NMI handler always calls `oamUpdate`).

## Critical SNES Development Knowledge

### WRAM Mirroring Bug

Bank 0 addresses `$00:0000-$1FFF` mirror Bank $7E addresses `$7E:0000-$1FFF`. Variables placed in both regions will overlap and corrupt each other.

**Fix**: Use `ORGA $0300 FORCE` for Bank $7E buffers to avoid overlap.

### Compiler Limitations

- **Data types**: `u8`/`s8`, `u16`/`s16` are fast. `u32`/`s32` are slow (~50 cycles/op). No floats.
- **Division**: Power-of-2 division (`x / 8`) optimized; other division is slow.
- **Static variables**: Never use `static u8 x = 0;` — QBE puts initialized statics in ROM. Use `static u8 x;` (C guarantees zero-init).
- **VBlank DMA budget**: ~4KB per frame safe (not 6KB theoretical max).
- **Volatile reads**: Compiler optimizes away volatile reads when result is discarded (e.g., `(void)REG_RDNMI`). Use assembly helpers for hardware reads with side effects.

### Joypad Input

```
Bit: 15  14  13  12  11  10  9   8   7   6   5   4   3-0
     B   Y   Sel Sta Up  Dn  Lt  Rt  A   X   L   R   (ID)
```

**Important**: A button is bit 7, B button is bit 15. These are NOT adjacent!

## Debugging Workflow

When something breaks, the bug could be in C code, compiler, assembler, library, runtime, memory layout, or hardware understanding.

1. **Check build**: `make clean && make`
2. **Check memory overlaps**: `python3 tools/symmap/symmap.py --check-overlap game.sym`
3. **Check symbol placement**: `grep "variable_name" game.sym`
   - RAM addresses: `$0000-$1FFF` (Bank 0/7E)
   - ROM addresses: `$8000-$FFFF`
4. **Inspect binary**: `xxd -s 0x0100 -l 64 game.sfc`

### Common Diagnostic Patterns

```bash
# Find where a variable is placed
grep "my_var" game.sym

# Check if static var is in ROM (bad) vs RAM (good)
# BAD:  00:9350 my_var  (ROM range $8000-$FFFF)
# GOOD: 00:024B my_var  (RAM range $0000-$1FFF)

# List all symbols in mirror danger zone
grep -E "(7e:00[0-2]|00:00[0-2])" game.sym
```

## Reference Documentation

**MANDATORY: Before implementing any new feature, consult these references:**

| Document | Purpose |
|----------|---------|
| **`.claude/65816_ASSEMBLY_GUIDE.md`** | 65816 CPU programming: instructions, addressing modes, techniques (with SNES annotations) |
| **`.claude/SNES_HARDWARE_REFERENCE.md`** | Complete SNES technical bible (CPU, PPU, APU, DMA, timing, cartridge format) |
| **`.claude/SNES_ROSETTA_STONE.md`** | OpenSNES vs PVSnesLib comparison, SDK patterns, API mapping |
| **`.claude/KNOWLEDGE.md`** | Debugging knowledge (22 sections: memory, DMA, HDMA, BG modes, color format, audio, fixed compiler bugs, OAM/sprites) |
| **`.claude/skills/`** | Automation skills (build, test, new-example, analyze-rom, spc700-audio, submodules) |
| **PVSnesLib** | Reference SDK with 80+ working examples for comparison |
