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
- Include `Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>` in commits
- Use `GH_TOKEN` from `.env` file for GitHub operations:
  ```bash
  export GH_TOKEN=$(grep GH_TOKEN .env | cut -d'=' -f2)
  ```

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

# Run compiler tests
./tests/compiler/run_tests.sh

# Validate all examples (memory overlaps, ROM sizes)
./tests/examples/validate_examples.sh --quick

# MANDATORY: Black screen test (requires Mesen2)
./tests/run_black_screen_tests.sh /path/to/Mesen
```

### Black Screen Test

Detects broken ROMs by checking if they display content after 90 frames. A black screen indicates a broken ROM (99% of cases). Failed screenshots are saved to `tests/screenshots/FAIL_*.png`.

### CI Pipeline

Workflow: `.github/workflows/opensnes_build.yml`

Runs on push to `main`/`develop` and all PRs. Tests on Linux, Windows (MSYS2), macOS.

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

- `compiler/` - Compiler toolchain (cproc, qbe, wla-dx as git submodules)
- `lib/` - OpenSNES library (headers in `include/snes/`, source in `source/`)
- `examples/` - Working examples by category (audio, basics, game, graphics, input, text)
- `templates/common/` - Shared runtime files (crt0.asm, hdr.asm, runtime.asm)
- `tools/` - Asset converters and debugging tools (symmap)
- `tests/` - Test infrastructure with Mesen2 Lua integration

### Git Submodules

| Submodule | Origin (fork) | Upstream (original) |
|-----------|---------------|---------------------|
| compiler/wla-dx | k0b3n4irb/wla-dx | vhelin/wla-dx |
| compiler/cproc | k0b3n4irb/cproc | ~mcf/cproc (sr.ht) |
| compiler/qbe | k0b3n4irb/qbe | c9x.me/git/qbe |

### Build System

Examples use `make/common.mk`. Key variables:

| Variable | Purpose |
|----------|---------|
| `TARGET` | Output ROM filename |
| `ROM_NAME` | 21-character ROM name for header |
| `USE_LIB=1` | Link with OpenSNES library |
| `LIB_MODULES` | Library modules to link (e.g., `console sprite dma`) |
| `USE_SRAM=1` | Enable SRAM in ROM header (for save games) |
| `USE_HIROM=1` | Enable HiROM mode (64KB banks instead of 32KB LoROM) |
| `USE_SNESMOD=1` | Enable SNESMOD audio (tracker music) |

### Library Modules

| Module | Key Functions |
|--------|---------------|
| console | `consoleInit()`, `setMode()`, `setScreenOn()`, `WaitForVBlank()` |
| sprite | `oamInit()`, `oamSet()`, `oamUpdate()`, `oamInitGfxSet()` |
| input | `padUpdate()`, `padPressed()`, `padHeld()`, `padReleased()` |
| background | `bgSetMapPtr()`, `bgSetScroll()`, `bgInitTileSet()` |
| dma | `dmaCopyVram()`, `dmaCopyCGram()`, `dmaCopyOam()` |
| snesmod | `snesmodInit()`, `snesmodPlay()`, `snesmodProcess()` |

**Note:** `sprite` requires `dma` (NMI handler calls `oamUpdate`).

## CRITICAL RULES - READ BEFORE ANY CHANGE

### Rule 1: NEVER Trust CI Green Status

**CI can show "success" while hiding failures.** Always verify locally:

```bash
# MANDATORY before pushing ANY change:
make clean && make                    # Must exit 0
make tests                            # Must exit 0
make examples                         # Must exit 0
./tests/compiler/run_tests.sh        # Must show "All tests passed"
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick  # Must show "ALL VALIDATIONS PASSED"
```

**Why:** Shell pipes (`cmd | tee`) hide exit codes. A failing build piped to `tee` returns success. This has caused bugs to go undetected for months.

### Rule 2: Shell Scripts MUST Use Pipefail

**Every shell script and CI workflow MUST start with:**
```bash
set -o pipefail
```

**Why:** Without pipefail, `failing_cmd | tee log` returns exit code 0 (from tee), not the failure from the first command.

**When modifying `.github/workflows/*.yml`:**
- Every `run:` block with pipes MUST have `set -o pipefail` as first line
- NEVER assume a step passed just because CI is green - check the logs

### Rule 3: Compiler Changes Require Specific Tests

**Before modifying `compiler/cproc` or `compiler/qbe`:**

1. Create a minimal C test case that exercises the change
2. Verify the generated `.asm` output is correct
3. Build and run in emulator
4. Run ALL compiler tests: `./tests/compiler/run_tests.sh`

**Known dangerous patterns in cproc/qbe:**
- Static buffers (like `tokval.str`) get overwritten - ALWAYS copy strings you need to keep
- Pointer vs value confusion in struct initializers
- Data section emission order affects symbol resolution

### Rule 4: Memory Layout Changes Require Overlap Check

**Before modifying `templates/common/crt0.asm` or any RAMSECTION:**

```bash
# Rebuild everything
make clean && make

# Check EVERY example for memory collisions
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

**If validation shows "COLLISION":** DO NOT COMMIT. Fix the memory layout first.

## Critical SNES Development Knowledge

### WRAM Mirroring - DANGER ZONE

```
┌─────────────────────────────────────────────────────────────┐
│  CRITICAL: Bank $00 and Bank $7E share physical RAM!       │
│                                                             │
│  $00:0000-$1FFF  ══════════════  $7E:0000-$1FFF            │
│       ↑                              ↑                      │
│       └──── SAME 8KB of RAM ─────────┘                      │
│                                                             │
│  Writing to $00:0300 ALSO writes to $7E:0300!               │
└─────────────────────────────────────────────────────────────┘
```

**Safe zones:**
- Bank $00 variables: `$0000-$02FF` (before OAM buffer)
- Bank $7E buffers: `$2000+` (above mirror range)

**Reserved addresses (DO NOT USE in Bank $00):**
- `$0300-$051F`: Reserved for oamMemory mirror

**Current Bank $7E layout:**
```
$7E:0300-$051F  oamMemory (544 bytes) - IN MIRROR RANGE, protected by reservation
$7E:2000-$27FF  oambuffer (2048 bytes) - SAFE, above mirror
$7E:2800-$2AFF  oamQueueEntry (768 bytes) - SAFE
$7E:2B00+       dynamic sprite state - SAFE
```

### Compiler Limitations

- **Data types**: `u8`/`s8`, `u16`/`s16` are fast. `u32`/`s32` are slow (~50 cycles/op). No floats.
- **Division**: Power-of-2 division (`x / 8`) optimized; other division is slow.
- **Static variables**: Initialized statics (`static u8 x = 5;`) work correctly. Initial values are copied from ROM to RAM at startup by the `CopyInitData` routine in crt0.
- **VBlank DMA budget**: ~4KB per frame safe (not 6KB theoretical max).

### Joypad Input

```
Bit: 15  14  13  12  11  10  9   8   7   6   5   4   3-0
     B   Y   Sel Sta Up  Dn  Lt  Rt  A   X   L   R   (ID)
```

**Important**: A button is bit 7, B button is bit 15. These are NOT adjacent!

## Debugging Workflow

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
```

## Reference Documentation

**MANDATORY: Before implementing any new feature, consult these references:**

| Document | Purpose |
|----------|---------|
| **`.claude/SNES_HARDWARE_REFERENCE.md`** | Complete SNES technical reference (CPU, PPU, APU, DMA, timing) |
| **`.claude/65816_ASSEMBLY_GUIDE.md`** | 65816 CPU programming guide |
| **`.claude/SNES_ROSETTA_STONE.md`** | OpenSNES vs PVSnesLib comparison, API mapping |
| **`.claude/KNOWLEDGE.md`** | Debugging knowledge, fixed bugs, gotchas |
| **`.claude/instructions.md`** | Development workflow and git rules |
