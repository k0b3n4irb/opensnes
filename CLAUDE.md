# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenSNES is a modern, open-source SDK for Super Nintendo (SNES) development. It provides a QBE-based C compiler (cc65816), WLA-DX assembler, and a library for SNES hardware abstraction.

## Build Commands

```bash
make                    # Build everything (compiler, tools, library, examples)
make compiler           # Build cc65816 (cproc + QBE w65816 backend) and WLA-DX
make tools              # Build asset tools (gfx2snes, font2snes)
make lib                # Build OpenSNES library
make examples           # Build all example ROMs
make clean              # Clean all build artifacts

# Build a single example
cd examples/text/1_hello_world && make

# Check for memory overlaps after building
python3 tools/symmap/symmap.py --check-overlap game.sym
```

## Testing

### CRITICAL: Test After Every Change

**Goal: 100% test coverage.** After ANY code change:
1. Run local validation (see below)
2. Fix any failures before committing
3. Update CI workflow if adding new tests

### Local Validation (Run Before Every Commit)

```bash
# Full build - must pass
make clean && make

# Validate all examples (memory overlaps, ROM sizes)
./tests/examples/validate_examples.sh --quick

# Run compiler tests
./tests/compiler/run_tests.sh

# Verify example count (should be 20)
find examples -name "*.sfc" | wc -l
```

### CI Pipeline (GitHub Actions)

Workflow: `.github/workflows/opensnes_build.yml`

Runs on every push to `main`/`develop` and all PRs. Tests on Linux, Windows, macOS.

| Check | What it validates |
|-------|-------------------|
| Build | Toolchain, library, all 19 examples |
| Validate | Memory overlaps, ROM sizes, required symbols |
| Count | Fails if < 19 examples built |
| Compiler | Regression tests for cc65816 |

### Mesen2 Tests

```bash
# Run tests with Mesen2 emulator
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

The QBE w65816 backend outputs WLA-DX compatible assembly directly (no post-processing).

### Directory Structure

- `compiler/` - Compiler toolchain (cproc, qbe, wla-dx as submodules)
- `lib/` - OpenSNES library (headers in `include/snes/`, source in `source/`)
- `examples/` - Working examples organized by category (text, graphics, audio, basics)
- `templates/common/` - Shared runtime files (crt0.asm, hdr.asm, runtime.asm)
- `tools/` - Asset converters (gfx2snes, font2snes, smconv) and debugging tools (symmap, snesdbg)
- `tests/` - Test infrastructure with Mesen2 Lua integration

### Build System

Examples use `make/common.mk` which provides standard build rules. Key variables:
- `TARGET` - Output ROM filename
- `ROM_NAME` - 21-character ROM name for header
- `USE_LIB=1` - Link with OpenSNES library
- `LIB_MODULES` - Library modules to link (console, sprite, input, audio, snesmod)
- `USE_SNESMOD=1` - Enable SNESMOD audio (tracker music)
- `SOUNDBANK_SRC` - Impulse Tracker files for SNESMOD

### Library Modules

| Module | Functions |
|--------|-----------|
| console | `consoleInit()`, `setMode()`, `setScreenOn()`, `WaitForVBlank()` |
| sprite | `oamInit()`, `oamSet()`, `oamUpdate()`, `oamHide()` |
| input | `padUpdate()`, `padPressed()`, `padHeld()` |
| audio | `audioInit()`, `audioPlaySample()` |
| snesmod | `snesmodInit()`, `snesmodPlay()`, `snesmodProcess()` |

## Critical SNES Development Knowledge

### WRAM Mirroring Bug

Bank 0 addresses `$00:0000-$1FFF` mirror Bank $7E addresses `$7E:0000-$1FFF`. Variables placed in both regions will overlap and corrupt each other.

**Fix**: Use `ORGA $0300 FORCE` to place Bank $7E buffers above the mirror range:
```asm
.RAMSECTION ".buffer" BANK $7E SLOT 1 ORGA $0300 FORCE
    buffer dsb 544
.ENDS
```

### Compiler Limitations

- **Data types**: `u8`/`s8`, `u16`/`s16` are fast. `u32`/`s32` are slow (~50 cycles/op). No floats.
- **Division**: Power-of-2 division (`x / 8`) is optimized. Other division is slow.
- **Multiplication/Division**: Runtime functions `__mul16`/`__div16` may produce incorrect results. Use repeated addition/subtraction for reliable arithmetic.
- **Shift operations**: Avoid in complex expressions; use inline alternatives like `(x << 3) + (x << 1)` for `x * 10`.

### QBE Backend Gotchas

The QBE w65816 backend struggles with:
- Complex switch statements with many cases
- Large static const arrays (1000+ bytes)
- Runtime multiplication/division in some contexts

**Workaround**: Use pre-encoded data arrays with direct indices instead of runtime character conversion or large lookup tables.

### WLA-DX Assembler Notes

- Labels starting with `_` are local to the section (use `.DEFINE` + `.EXPORT` for global aliases)
- Use `.SECTION "name" SUPERFREE` for auto-placed code
- Use `RAMSECTION` with explicit addresses for RAM variables

### OAM/Sprites

- OAM is 544 bytes (512 main + 32 high table)
- Set Y=240 to hide sprites (off-screen on NTSC)
- Use `snesSpriteRam` (not `snesOam`) in Mesen2 Lua scripts

### Joypad Input

The 16-bit joypad value (`JOY1L | (JOY1H << 8)`) has this layout:
```
Bit: 15  14  13  12  11  10  9   8   7   6   5   4   3-0
     B   Y   Sel Sta Up  Dn  Lt  Rt  A   X   L   R   (ID)
```

**Important**: A button is bit 7 (low byte), B button is bit 15 (high byte). These are NOT adjacent!

**Testing**: Always test on accurate emulators (bsnes/OpenEmu) - Mesen may hide hardware accuracy bugs.

## Debugging Workflow

When something breaks, the bug could be in C code, compiler, assembler, library, runtime, memory layout, or hardware understanding.

1. Check build: `make clean && make`
2. Check memory overlaps: `python3 tools/symmap/symmap.py --check-overlap game.sym`
3. Check symbol placement: `grep "variable_name" game.sym` (RAM is $0000-$1FFF, ROM is $8000-$FFFF)
4. Inspect binary: `xxd -s 0x0100 -l 64 game.sfc`

## Reference

- PVSnesLib at `/Users/k0b3/workspaces/github/pvsneslib` has 80+ working examples for comparison
- See `.claude/KNOWLEDGE.md` for comprehensive debugging knowledge and gotchas
- See `.claude/skills/` for automation skills (/build, /test, /new-example, /analyze-rom, /roadmap)
