# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenSNES is a modern, open-source SDK for Super Nintendo (SNES) development, forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib) with a focus on **ease of use** and **developer experience**.

### Components

- **C Compiler (cc65816)**: QBE-based compiler targeting the 65816 CPU
- **Assembler (WLA-DX)**: Industry-standard assembler for 65816 and SPC700
- **Hardware Library**: Clean C API for PPU, input, DMA, sprites, text, and audio
- **Asset Tools**: gfx4snes, font2snes, smconv

## Git Workflow

- **ALWAYS** work on the `develop` branch, never directly on `main`
- Changes to `main` must be done via GitHub Pull Request only
- Use conventional commit format (feat, fix, chore, docs, refactor, test)
- Include `Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>` in commits
- Use `GH_TOKEN` from `.env` file for GitHub operations:
  ```bash
  export GH_TOKEN=$(grep GH_TOKEN .env | cut -d'=' -f2)
  ```

### Submodules

| Submodule | Can Modify? | Push To |
|-----------|-------------|---------|
| `compiler/qbe` | YES | k0b3n4irb/qbe |
| `compiler/cproc` | YES | k0b3n4irb/cproc |
| `compiler/wla-dx` | NO | Never (read-only) |

For submodule commits: commit inside the submodule first, push it, then `git add compiler/<sub>` in the main repo.

## Build Commands

```bash
make                    # Build everything (compiler, tools, library, examples)
make compiler           # Build cc65816 (cproc + QBE w65816 backend) and WLA-DX
make tools              # Build asset tools (gfx4snes, font2snes)
make lib                # Build OpenSNES library
make examples           # Build all example ROMs
make tests              # Build test ROMs
make install            # Install binaries to bin/
make docs               # Generate Doxygen documentation
make release            # Create SDK release package (zip)
make clean              # Clean all build artifacts
make help               # Show all targets

# Build a single example
make -C examples/text/1_hello_world

# Check for memory overlaps after building
python3 tools/symmap/symmap.py --check-overlap game.sym
```

### Single-File C Limitation

Only a single C source file (`main.c` by default) is supported per project. Workarounds:
1. `#include` other C files directly into main.c
2. Add additional logic in assembly via the `ASMSRC` variable
3. Set `CSRC` to a different single C file

## Testing

### Local Validation (Run Before Every Commit)

```bash
make clean && make                                                      # Full build
./tests/compiler/run_tests.sh                                           # Compiler tests
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick      # Example validation
```

### Test Scripts

| Script | Purpose | Options |
|--------|---------|---------|
| `./tests/compiler/run_tests.sh` | Compiler regression tests (40+ cases) | `-v`, `--allow-known-bugs` |
| `./tests/examples/validate_examples.sh` | Memory overlaps, ROM sizes | `--quick`, `--verbose` |
| `./tests/run_black_screen_tests.sh <mesen>` | Detect broken ROMs via emulator | `--verbose`, `--save-screenshots` |
| `./tests/ci/check_memory_overlaps.sh` | CI memory overlap checking | |

**Note:** `validate_examples.sh` requires `OPENSNES_HOME=$(pwd)` to be set.

### CI Pipeline

Workflow: `.github/workflows/opensnes_build.yml` — runs on push to `main`/`develop` and all PRs. Tests on Linux, Windows (MSYS2), macOS.

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

### Key Directories

- `compiler/qbe/w65816/` — The 65816 code generator backend (emit.c, abi.c, isel.c)
- `compiler/cproc/` — C frontend that emits QBE IR
- `lib/include/snes/` — Public library headers
- `lib/source/` — Library implementation (mixed C and assembly)
- `templates/common/` — Shared runtime: crt0.asm (startup/init), hdr.asm (LoROM header), hdr_hirom.asm, runtime.asm (math helpers), data_init_start/end.asm (static initializer wrappers)
- `make/common.mk` — Build rules included by all example Makefiles
- `tools/symmap/symmap.py` — Symbol map analyzer for memory overlap checking

### Build System (make/common.mk)

Examples use `make/common.mk`. Key variables:

| Variable | Purpose |
|----------|---------|
| `OPENSNES` | **Required.** Absolute path to SDK root |
| `TARGET` | Output ROM filename |
| `ROM_NAME` | 21-character ROM name for header |
| `CSRC` | C source file (default: `main.c`, single file only) |
| `ASMSRC` | Additional assembly files to link |
| `USE_LIB=1` | Link with OpenSNES library |
| `LIB_MODULES` | Library modules to link (e.g., `console sprite dma`) |
| `USE_HIROM=1` | Enable HiROM mode (64KB banks instead of 32KB LoROM) |
| `USE_SRAM=1` | Enable SRAM in ROM header (for save games) |
| `GFXSRC` | PNG files to convert with gfx4snes |
| `SPRITE_SIZE` | Tile size for gfx4snes (default: 8) |
| `BPP` | Bits per pixel for graphics (default: 4) |
| `USE_SNESMOD=1` | Enable SNESMOD audio |
| `SOUNDBANK_SRC` | IT files for SNESMOD audio |

### Library Module Dependencies

The build system does NOT auto-resolve dependencies:

```
sprite  → dma       (OAM buffer DMA'd in NMI)
text    → dma       (Font loading uses DMA)
snesmod → console   (Requires NMI handling)

Common combos:
  Basic game:   console sprite dma input background
  With audio:   console sprite dma input snesmod
  With effects: console sprite dma hdma
```

### Library Modules

| Module | Key Functions |
|--------|---------------|
| console | `consoleInit()`, `setMode()`, `setScreenOn()`, `WaitForVBlank()` |
| sprite | `oamInit()`, `oamSet()`, `oamUpdate()`, `oamInitGfxSet()` |
| input | `padUpdate()`, `padPressed()`, `padHeld()`, `padReleased()` |
| background | `bgSetMapPtr()`, `bgSetScroll()`, `bgInitTileSet()` |
| dma | `dmaCopyVram()`, `dmaCopyCGram()`, `dmaCopyOam()` |
| snesmod | `snesmodInit()`, `snesmodPlay()`, `snesmodProcess()` |

## CRITICAL RULES

### Rule 1: NEVER Trust CI Green Status

**CI can show "success" while hiding failures.** Shell pipes (`cmd | tee`) hide exit codes. Always verify locally:

```bash
make clean && make
./tests/compiler/run_tests.sh
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

### Rule 2: Shell Scripts MUST Use Pipefail

Every shell script and CI `run:` block with pipes MUST start with `set -o pipefail`.

### Rule 3: Compiler Changes Require Full Validation

Before modifying `compiler/cproc` or `compiler/qbe`:
1. Create a minimal C test case
2. Verify generated `.asm` output: `./bin/cc65816 test.c -o test.asm`
3. Run ALL compiler tests: `./tests/compiler/run_tests.sh`
4. Rebuild and validate: `make clean && make && OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick`

**Known dangerous patterns in cproc/qbe:**
- Static buffers (like `tokval.str`) get overwritten — ALWAYS copy strings you need to keep
- `tokval.str` for quoted strings INCLUDES the quote characters (`section ".rodata"` → `"\".rodata\""`)
- Data section emission order affects symbol resolution

### Rule 4: Memory Layout Changes Require Overlap Check

After modifying `templates/common/crt0.asm` or any RAMSECTION, rebuild everything and run `validate_examples.sh`. If it shows "COLLISION", DO NOT COMMIT.

### Rule 5: Test-Driven Development Protocol

Every change MUST follow the validation protocol:
@.claude/TESTING_PROTOCOL.md

## SNES Development Knowledge

### WRAM Mirroring

```
Bank $00:0000-$1FFF  ═══  Bank $7E:0000-$1FFF   (SAME 8KB of RAM)
```

- Bank $00 variables: `$0000-$02FF` (before OAM buffer)
- Bank $7E buffers: `$2000+` (above mirror range)
- `$0300-$051F` in Bank $00: Reserved for oamMemory mirror — DO NOT USE

### Compiler Limitations

- **Data types**: `u8`/`u16` fast. `u32` = `unsigned int` (4 bytes, ~50 cycles/op). `unsigned long` = 8 bytes (NOT 32-bit!). No floats.
- **Division**: Power-of-2 optimized; others are slow.
- **Static variables**: `static u8 x = 5;` works — ROM→RAM copy at startup via `CopyInitData` in crt0.
- **VBlank DMA budget**: ~4KB per frame safe.
- **Signed right shift (`>>`)** — FIXED: The backend now emits `cmp.w #$8000` + `ror a` for `sar` (arithmetic shift), which correctly sign-extends negative values. Unsigned `shr` still uses `lsr`.

### Joypad Input

```
Bit: 15  14  13  12  11  10  9   8   7   6   5   4   3-0
     B   Y   Sel Sta Up  Dn  Lt  Rt  A   X   L   R   (ID)
```

A button is bit 7 (0x0080), B button is bit 15 (0x8000). NOT adjacent!

## Debugging

1. **Check build**: `make clean && make`
2. **Check memory overlaps**: `python3 tools/symmap/symmap.py --check-overlap game.sym`
3. **Check symbol placement**: `grep "my_var" game.sym`
   - RAM: `$0000-$1FFF` (good) — ROM: `$8000-$FFFF` (bad for mutable vars)
4. **Inspect binary**: `xxd -s 0x0100 -l 64 game.sfc`

## Reference Documentation

| Document | Purpose |
|----------|---------|
| `.claude/SNES_HARDWARE_REFERENCE.md` | Complete SNES technical reference (CPU, PPU, APU, DMA, timing) |
| `.claude/65816_ASSEMBLY_GUIDE.md` | 65816 CPU programming guide |
| `.claude/SNES_ROSETTA_STONE.md` | OpenSNES vs PVSnesLib comparison, API mapping |
| `.claude/KNOWLEDGE.md` | Debugging knowledge, fixed bugs, gotchas |
| `.claude/instructions.md` | Development workflow and git rules |
| `.claude/C9X_QBE_DOC.md` | QBE compiler backend documentation |
| `.claude/WLA-DX.md` | WLA-DX assembler documentation |
