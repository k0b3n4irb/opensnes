# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenSNES is a modern, open-source SDK for Super Nintendo (SNES) development, forked from PVSnesLib. Write game logic in C11 (compiled via cproc + QBE with a custom 65816 backend), link with a hardware library, and produce .sfc ROMs that run on emulators or real SNES hardware.

## Git Workflow

- **ALWAYS** work on `develop`, never directly on `main`
- Changes to `main` via GitHub Pull Request only
- Conventional commits: `feat(scope):`, `fix(scope):`, `docs:`, `refactor:`, `test:`, `chore:`
- Do NOT include any `Co-Authored-By` trailer in commit messages
- Use `GH_TOKEN` from `.env` for GitHub operations: `export GH_TOKEN=$(grep GH_TOKEN .env | cut -d'=' -f2)`

### Submodules

| Submodule | Modifiable? | Notes |
|-----------|-------------|-------|
| `compiler/qbe` | YES | Push to k0b3n4irb/qbe. Commit inside submodule first, push, then `git add compiler/qbe` in main repo |
| `compiler/cproc` | YES | Push to k0b3n4irb/cproc. Same workflow |
| `compiler/wla-dx` | NO | Read-only, never modify |

**Gotcha**: `git stash` in the parent repo does NOT affect submodule working trees.

## Build Commands

```bash
make                    # Build everything (compiler, tools, library, examples)
make compiler           # Build cc65816 + WLA-DX
make tools              # Build gfx4snes, font2snes, smconv
make lib                # Build the library
make examples           # Build all 25 example ROMs
make docs               # Generate Doxygen docs (requires doxygen)
make clean              # Clean all artifacts
make -C examples/games/breakout   # Build a single example
```

**Compiler rebuild gotcha**: `make compiler` may not recompile changed QBE .c files. Use `cd compiler/qbe && make clean && make` then copy the binary to `bin/`.

### Multiple C Source Files

The build system supports multiple C files per project via the `CSRC` variable:
```makefile
CSRC := main.c engine/player.c ui/hud.c
```
If `CSRC` is not set, it defaults to `main.c`. Assembly files can also be added via `ASMSRC`.

## Testing

### Before Every Commit

```bash
make clean && make                                                      # Full rebuild
./tests/unit/run_unit_tests.sh                                          # Unit tests (19 modules)
./tests/compiler/run_tests.sh --allow-known-bugs                        # Compiler tests (53/54)
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick      # Example validation (25/25)
```

**`validate_examples.sh` requires `OPENSNES_HOME=$(pwd)` to be set.**

### Test Scripts

| Script | Purpose |
|--------|---------|
| `./tests/unit/run_unit_tests.sh` | 19 unit test modules (~385 runtime + 119 compile-time assertions). Options: `-v` |
| `./tests/compiler/run_tests.sh` | 54 compiler regression tests (use `--allow-known-bugs` to tolerate known failures). Options: `-v`, `--allow-known-bugs` |
| `./tests/examples/validate_examples.sh` | Memory overlaps + ROM size checks. Options: `--quick`, `--verbose` |
| `./tests/ci/check_memory_overlaps.sh` | CI-specific overlap checking |

### Compiler Changes Require Full Validation

1. Create a minimal C test case
2. Verify generated `.asm`: `./bin/cc65816 test.c -o test.asm`
3. Run ALL compiler tests
4. `make clean && make && OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick`

## Architecture

### Toolchain Pipeline

```
C source → cproc (C11 frontend) → QBE IR → w65816 backend → 65816 Assembly (.asm)
                                                                    ↓
                                                          wla-65816 (assembler)
                                                                    ↓
                                                          wlalink (linker) ← crt0.asm, runtime.asm, library
                                                                    ↓
                                                                .sfc ROM
```

### Key Directories

| Path | What it is |
|------|-----------|
| `compiler/qbe/w65816/emit.c` | **The big one** (95KB). 65816 code generator with 13 optimization phases |
| `compiler/qbe/w65816/abi.c` | ABI / calling conventions |
| `compiler/cproc/` | C11 frontend (emits QBE IR) |
| `templates/common/crt0.asm` | Startup code, NMI handler, WRAM layout (26KB) |
| `templates/common/runtime.asm` | Math helpers (__mul16, __div16, __mod16) |
| `make/common.mk` | Shared build rules — all example Makefiles include this |
| `lib/include/snes/` | 23 public library headers |
| `lib/source/` | Library implementation (mixed C and assembly) |
| `devtools/symmap/symmap.py` | Symbol map analyzer (memory overlap checking) |

### Build System (make/common.mk)

Example Makefiles set variables then `include $(OPENSNES)/make/common.mk`:

| Variable | Purpose |
|----------|---------|
| `OPENSNES` | **Required.** Absolute path to SDK root |
| `TARGET` | Output ROM filename |
| `ROM_NAME` | 21-character ROM name for SNES header |
| `USE_LIB=1` | Link with OpenSNES library |
| `LIB_MODULES` | Which library modules to link (e.g., `console sprite dma input`) |
| `USE_HIROM=1` | HiROM mode (64KB banks) |
| `USE_SRAM=1` | Enable battery-backed save RAM |
| `GFXSRC` | PNG files to convert with gfx4snes |
| `USE_SNESMOD=1` | Enable SNESMOD audio |

**Module dependencies are NOT auto-resolved:**
```
sprite  → dma       (OAM DMA in NMI)
text    → dma       (font loading)
snesmod → console   (NMI handling)

Common combos:
  Basic game:   console sprite dma input background
  With audio:   console sprite dma input snesmod
  With effects: console sprite dma hdma
```

## Critical Rules

### VRAM Writes Must Happen During VBlank

The SNES PPU **silently ignores** VRAM writes during active display. Any function doing VRAM writes must ensure VBlank timing (`WaitForVBlank()` or force blank `REG_INIDISP = 0x80`).

### Memory Layout Changes Require Overlap Check

After modifying `crt0.asm` or any RAMSECTION: rebuild everything, run `validate_examples.sh`. If "COLLISION" appears, DO NOT COMMIT.

### All C-Accessible RAM Must Be in Bank $00, Address < $2000

The compiler generates `sta.l $0000,x` which always accesses bank $00. Bank $00 $0000-$1FFF mirrors $7E:0000-$1FFF (WRAM). Bank $00 $2000+ is I/O / open bus.

### Shell Scripts Must Use Pipefail

CI pipes (`cmd | tee`) hide exit codes. Every script with pipes must use `set -o pipefail`.

## SNES Development Knowledge

### WRAM Mirroring

```
Bank $00:0000-$1FFF  ===  Bank $7E:0000-$1FFF   (same 8KB)
```

- `$0000-$02FF`: C variables
- `$0300-$051F`: Reserved for oamMemory (DO NOT USE)
- `$7E:2000+`: Large buffers (not C-accessible via compiler-generated code)

### Compiler Type Sizes

| Type | Size | Notes |
|------|------|-------|
| `u8` / `unsigned char` | 1 byte | Fast |
| `u16` / `unsigned short` | 2 bytes | Fast, native register width |
| `u32` / `unsigned int` | 4 bytes | Slow (~50 cycles/op) |
| `unsigned long` | **8 bytes** | NOT 32-bit! Use `unsigned int` for 32-bit |
| `float` | N/A | Not supported. Use fixed-point math |

### Joypad Bit Layout

```
Bit: 15  14  13  12  11  10  9   8   7   6   5   4   3-0
     B   Y   Sel Sta Up  Dn  Lt  Rt  A   X   L   R   (ID)
```

### VBlank DMA Budget

~4KB per frame safe. NMI handler overhead (~8600+ cycles) reduces available time. For large transfers, use force blank (`REG_INIDISP = 0x80`) before DMA, restore after.

### Known cproc/QBE Gotchas

- `tokval.str` includes quote characters: `section ".rodata"` appears as `"\".rodata\""` in code
- Static buffers get overwritten — copy strings you need to keep
- `emit.c` has 13 optimization phases; understand the phase interactions before modifying

## Debugging

```bash
make clean && make                                          # Rebuild from scratch
python3 devtools/symmap/symmap.py --check-overlap game.sym    # Check memory overlaps
grep "my_var" game.sym                                      # Check symbol placement
xxd -s 0x0100 -l 64 game.sfc                                # Inspect ROM binary
./bin/cc65816 test.c -o test.asm                             # Inspect generated assembly
```

## Reference Documentation

Local reference files in `.claude/` (not tracked in git):

| Document | Purpose |
|----------|---------|
| `.claude/SNES_HARDWARE_REFERENCE.md` | Complete SNES technical reference (CPU, PPU, APU, DMA, timing) |
| `.claude/65816_ASSEMBLY_GUIDE.md` | 65816 CPU programming guide |
| `.claude/SNES_ROSETTA_STONE.md` | OpenSNES vs PVSnesLib API mapping |
| `.claude/C9X_QBE_DOC.md` | QBE compiler backend documentation |
| `.claude/WLA-DX.md` | WLA-DX assembler documentation |
| `.claude/TESTING_PROTOCOL.md` | Full testing protocol (TDD, change classification, regression analysis) |
| `.claude/PORTING_PLAN.md` | PVSnesLib → OpenSNES example porting tracker (25 done, 11 remaining) |
