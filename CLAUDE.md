# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Design philosophy — read first

Before proposing or accepting a refactor, consult **`PHILOSOPHY.md`**
at the repo root. OpenSNES is a 2D game engine, not a thin C-over-asm
wrapper, and five principles guide every API decision (sane defaults
with escape hatches, hidden quirks with documented escape, opt-in
modules, type-safe boundaries, predictable performance). Use them as
acceptance criteria when evaluating a change. The "Non-goals" section
of that doc is also load-bearing — it lists patterns the project
deliberately refuses (no GC equivalent, no monolithic engine class,
no `printf` in core lib, no mandatory framework lifecycle).

## Build Commands

```bash
make                    # Build everything: compiler → tools → library → examples
make compiler           # Build cc65816 (cproc+QBE) and WLA-DX assembler/linker
make tools              # Build gfx4snes, font2snes, smconv
make lib                # Build OpenSNES library (LoROM + HiROM + SA-1 + SuperFX)
make examples           # Build all example ROMs
make clean              # Clean all build artifacts
make clean && make      # Full rebuild (REQUIRED after compiler or runtime changes)
```

Build a single example:
```bash
cd examples/text/hello_world && make
```

## Testing

All testing goes through opensnes-emu (snes9x WASM):
```bash
cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
```

Runs 7 phases: preconditions, compiler tests (60 C→ASM pattern checks), build, static analysis, runtime execution, visual regression, lag detection.

See `.claude/rules/testing.md` for the mandatory 3-pillar validation workflow (opensnes-emu + Mesen2 + full rebuild). Changes are classified A/B/C/D by impact scope.

## Compilation Pipeline

```
main.c → cproc (C11 frontend) → QBE w65816 (codegen) → wla-65816 (assembler) → wlalink (linker) → game.sfc
```

The `bin/cc65816` wrapper orchestrates cproc→QBE→wla-65816. QBE emits 65816 assembly, sed-transformed (`.byte`→`.db`, `.word`→`.dw`) before assembly.

## Architecture

### Key Directories

- **compiler/** — Three git submodules: `cproc/` (C frontend), `qbe/` (65816 backend), `wla-dx/` (assembler/linker). Also builds `wla-superfx` for SuperFX GSU code.
- **lib/** — Hardware library. C sources in `lib/source/*.c`, ASM in `lib/source/*.asm`, headers in `lib/include/snes/`. Built as separate LoROM, HiROM, SA-1, and SuperFX object sets.
- **templates/** — ROM bootstrap: `crt0.asm` (startup + NMI handler), `hdr*.asm` (ROM headers), `runtime.asm` (math routines, now in lib/source/), `memmap*.inc` (memory maps). These are the single source of truth — examples don't duplicate them.
- **make/common.mk** — Universal build rules included by every example. Handles graphics conversion, multi-file C compilation, SNESMOD audio, SA-1/SuperFX/HiROM mode selection, module linking.
- **tools/** — `gfx4snes` (PNG→SNES tiles), `smconv` (IT→SPC700), `opensnes-emu/` (debug emulator + test suite)
- **examples/** — 54 ROMs organized by category (text, graphics, input, audio, maps, memory, games)

### Enhancement Chip Support

- **SA-1** (`USE_SA1=1`): Same 65816 ISA at 10.74 MHz. Shares I-RAM ($3000-$37FF) with main CPU. Per-example `sa1_boot.asm` for custom coprocessor code. See `docs/tutorials/sa1.md`.
- **SuperFX** (`USE_SUPERFX=1`): Custom RISC ISA (GSU). Two-stage build: `.sfx` → `wla-superfx` → `wlalink -b` → `.sfx.bin` → `.incbin`. GSU code is assembly-only (no C compiler). **Validate with Mesen2** — it is the most accurate emulator currently available for GSU. **Do not use snes9x** for SuperFX validation: snes9x does not detect the GSU chip in our ROM headers (the example shows "GSU: NOT DETECTED"), so the snes9x-based CI test suite can only confirm boot, not GSU execution. P3.4 in `ROADMAP.md` tracks adding a Mesen2-headless CI path for real coverage.

### Example Makefile Pattern

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := game.sfc
ROM_NAME := GAME NAME
USE_LIB  := 1
LIB_MODULES := console sprite dma input   # only link what you need
CSRC := main.c
include $(OPENSNES)/make/common.mk
```

## Code Style

- C: 4 spaces, K&R braces, snake_case functions/vars, UPPER_CASE constants
- Use fixed-width types: `u8`, `u16`, `s16`, `u32` (from `snes.h`). `unsigned int` = 4 bytes, `unsigned long` = 8 bytes on this target.
- ASM: labels at column 0, instructions indented with tab, `.section` for organization
- Commits: [Conventional Commits](https://www.conventionalcommits.org/) — `feat(scope):`, `fix(scope):`, `perf(scope):`, etc.
- Scopes: `lib`, `compiler`, `runtime`, `tools`, `examples`, `build`
- IMPORTANT: Do NOT add `Co-Authored-By` trailers for AI tools in commit messages.

## Critical Constraints

IMPORTANT: These are **silent failures** — no error messages, just wrong behavior.
The canonical, public-facing list with severity tags and mitigation notes lives
in `KNOWN_LIMITATIONS.md` at the repo root. Keep this section in sync.

- **VRAM writes only work during VBlank or forced blank** — the PPU silently ignores writes during active display
- **VBlank DMA budget**: ~4KB max per frame. Larger transfers need force blank (`setScreenOff/On`) or multi-frame splitting
- **Bank $00 overflow**: `static const` arrays get SUPERFREE sections. If bank $00 fills (32KB), data silently spills to bank $01+ but C code reads from bank $00 → garbage. Combine related const arrays.
- **`sta.l $0000,x` always reads bank $00** — all C RAM must be below $2000
- **cc65816 pushes args LEFT-TO-RIGHT** (not right-to-left like tcc816/PVSnesLib) — ASM functions ported from PVSnesLib have swapped stack offsets. See `compiler/ABI.md` for the full calling convention reference.
- **`data_init_end.o` MUST be linked last** — it's the sentinel for the DMA copy loop
- **WRAM data port ($2180-$2183) is NOT safe in NMI** — silent corruption if NMI fires mid-sequence
- **`volatile` in loops crashes QBE** — use globals instead of `volatile` keyword
- **WLA-DX loses .ACCU/.INDEX tracking after branch merges** — always add explicit `.ACCU 8`/`.ACCU 16` after every `rep`/`sep` in hand-written ASM

## Auto-Loaded Rules

The `.claude/rules/` directory contains mandatory rules automatically loaded by context:
- `testing.md` — 3-pillar test workflow, change classification (A/B/C/D)
- `commits.md` — Never add Co-Authored-By trailers
- `compiler.md` — Compiler architecture, build, constraints
- `templates.md` — Templates & build system, memory layout, linker order
- `new_example.md` — Example checklist: init order, Doxygen docs, README + screenshot mandatory
- `porting.md` — PVSnesLib→OpenSNES porting guide, API mapping, argument order
- `nmi_audit.md` — NMI handler rules, must consult before crt0.asm changes
- `regression_method.md` — Mandatory bisection-based debugging, never guess
- `release.md` — Release workflow, CHANGELOG format, version tagging
- `doc_consistency.md` — Anchored doc/code claims (version macros, ROADMAP status, examples count). Run `make lint-docs` before any release commit; must consult before editing version strings or example counts.
- `bank0_budget.md` — Bank $00 ROM hard-fail ratchet (`BANK0_FAIL_THRESHOLD`); must consult before adding const data or tuning the threshold.

## Strategic Planning

`.claude/STRUCTURAL_DEFECTS.md` is the maintainer-internal catalogue of
defects that require deliberate chantiers (multi-day to multi-month).
Carries effort estimates, risk profiles, an interaction matrix, and
investigation logs that the public `KNOWN_LIMITATIONS.md` and
`ROADMAP.md` deliberately omit. Consult before starting a non-trivial
chantier; update the catalogue (status, investigation log, references)
when shipping or deferring an entry. The public docs absorb only the
high-level outcome (severity, headline, mitigation reference).
