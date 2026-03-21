# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make                    # Build everything: compiler → tools → library → examples
make compiler           # Build cc65816 (cproc+QBE) and WLA-DX assembler/linker
make tools              # Build gfx4snes, font2snes, smconv
make lib                # Build OpenSNES library (LoROM + HiROM)
make examples           # Build all 52 example ROMs
make clean              # Clean all build artifacts
make clean && make      # Full rebuild (REQUIRED after compiler or runtime changes)
```

Build a single example:
```bash
cd examples/text/hello_world && make        # build one ROM
cd examples/text/hello_world && make clean  # clean one example
```

## Testing

All testing goes through opensnes-emu (snes9x WASM):
```bash
cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
```

This runs 7 phases: preconditions, compiler tests (60 C→ASM pattern checks), build, static analysis, runtime execution, visual regression, lag detection.

See `.claude/rules/testing.md` (auto-loaded) for the mandatory test workflow — changes are classified A/B/C/D and require opensnes-emu + user Mesen2 validation before commit.

## Architecture

### Compilation Pipeline

```
main.c → cproc (C11 frontend) → QBE w65816 (codegen) → wla-65816 (assembler) → wlalink (linker) → game.sfc
```

The `bin/cc65816` wrapper script orchestrates cproc→QBE→wla-65816. The QBE backend emits 65816 assembly which gets sed-transformed (`.byte`→`.db`, `.word`→`.dw`, etc.) before assembly.

### Key Directories

- **compiler/** — Three git submodules: `cproc/` (C frontend), `qbe/` (code generator with custom 65816 backend), `wla-dx/` (assembler/linker)
- **lib/** — 28-module hardware library. C sources in `lib/source/*.c`, ASM in `lib/source/*.asm`, headers in `lib/include/snes/`. Built as separate LoROM and HiROM object sets.
- **templates/** — ROM bootstrap: `crt0.asm` (startup + NMI handler + system vars), `hdr.asm` / `hdr_hirom.asm` (ROM headers + memory maps), `runtime.asm` (math/utility routines)
- **make/common.mk** — Universal build rules (478 lines). Every example includes this. Handles graphics conversion, multi-file C compilation, SNESMOD audio, module linking.
- **tools/** — `gfx4snes` (PNG→SNES tiles/palettes/tilemaps), `smconv` (IT→SPC700), `opensnes-emu/` (debug emulator + test suite)
- **examples/** — 52 ROMs organized by category (text, graphics, input, audio, maps, memory, games)

### Example Makefile Pattern

Every example's Makefile follows this structure:
```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := game.sfc
ROM_NAME := "GAME NAME           "   # exactly 21 chars
USE_LIB  := 1
LIB_MODULES := console sprite dma input   # only link what you need
CSRC := main.c
# USE_FASTROM := 1            # optional: ~33% faster ROM access
include $(OPENSNES)/make/common.mk
```

### Memory Layout (LoROM)

```
$00:0000-007D   Compiler registers (tcc__r0-r10h)
$00:0080        NMI callback pointer
$00:0100+       System variables (vblank_flag, oam_update_flag, frame_count)
$7E:0300-03FF   OAM buffer (128 sprites × 4 bytes)
$7E:0400-1FFF   Available WRAM
$8000-FFFF      ROM code/data (32KB per bank)
```

### Data Initialization

Initialized C data flows: `.data_init` sections in ROM → DMA copied to WRAM at boot by crt0.asm. The `data_init_end.o` object MUST be linked last (sentinel for the copy loop).

### Linker Object Order

`combined.obj` (crt0+runtime+ASM) → C objects → library objects → `data_init_end.o`

## Code Style

- C: 4 spaces, K&R braces, snake_case functions/vars, UPPER_CASE constants, PascalCase types
- Use fixed-width types: `u8`, `u16`, `s16`, `u32` (from `snes.h`). `unsigned int` = 4 bytes, `unsigned long` = 8 bytes on this target.
- ASM: labels at column 0, instructions indented with tab, `.section` for organization
- Commits: [Conventional Commits](https://www.conventionalcommits.org/) format — `feat(scope):`, `fix(scope):`, `perf(scope):`, etc.

## Critical Constraints

- **VRAM writes only work during VBlank or forced blank** — the PPU silently ignores writes during active display
- **VBlank DMA budget**: ~4KB max per frame. Larger transfers need force blank (`setScreenOff/On`) or multi-frame splitting
- **Bank $00 overflow**: `static const` arrays get SUPERFREE sections. If bank $00 fills (32KB), data silently spills to bank $01+ but C code reads from bank $00 → garbage. Combine related const arrays.
- **`sta.l $0000,x` always reads bank $00** — all C RAM must be below $2000
- **cc65816 pushes args LEFT-TO-RIGHT** (not right-to-left like tcc816/PVSnesLib) — ASM functions ported from PVSnesLib have swapped stack offsets

## Auto-Loaded Rules

The `.claude/rules/` directory contains mandatory rules that are automatically loaded:
- `testing.md` — 3-pillar test workflow, change classification, opensnes-emu usage
- `porting.md` — PVSnesLib→OpenSNES porting guide, gfx4snes flags, API mapping
- `nmi_audit.md` — NMI/VBlank audit plan (must consult before crt0.asm changes)
- `regression_method.md` — Mandatory bisection-based debugging method
- `release.md` — Release workflow (CHANGELOG, validation, tagging)
