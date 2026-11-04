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
make tools              # Build the opensnes-* tools (tileset, sprite, music, rom…)
make lib                # Build OpenSNES library (LoROM + HiROM + SA-1 + SuperFX)
make examples           # Build all example ROMs
make clean              # Clean all build artifacts
make clean-all          # ...and everything else the tree regenerates (benchmarks, fuzz corpus, zips, docs)
make clean && make      # Full rebuild (REQUIRED after compiler or runtime changes)
```

Build a single example:
```bash
cd examples/text/print_string && make
```

## Testing

All testing goes through **luna** (cycle-accurate native emulator, pinned
binary — no Node/WASM/Mesen2):
```bash
scripts/install-luna.sh                                  # fetch pinned luna (testing/luna.version)
python3 testing/luna_runner.py --coverage        # corpus liveness
python3 testing/luna_runner.py --compare         # visual regression (fbhash; multi-point for animated examples)
make test-manifests                                      # functional probes: luna test manifests (input→WRAM asserts)
make tests                                               # all of the above
```

luna runs SA-1 / Super FX / DSP-1 natively, so there is no chip-ROM side
channel. See `.claude/rules/testing.md` for the validation workflow and
`.claude/notes/chantiers/luna_migration.md` for the migration off snes9x-WASM.
Changes are classified A/B/C/D by impact scope.

## Compilation Pipeline

```
main.c → cproc (C11 frontend) → QBE w65816 (codegen) → wla-65816 (assembler) → wlalink (linker) → game.sfc
```

The `bin/cc65816` driver (compiled, `compiler/cc65816/`) orchestrates cproc→QBE→wla-65816. QBE's w65816 backend emits WLA-DX syntax directly (`.db`, `.dw`, `.SECTION`) — no post-transform.

## Architecture

### Key Directories

- **compiler/** — Three git submodules: `cproc/` (C frontend), `qbe/` (65816 backend), `wla-dx/` (assembler/linker). Also builds `wla-superfx` for SuperFX GSU code.
- **lib/** — Hardware library. C sources in `lib/source/*.c`, ASM in `lib/source/*.asm`, headers in `lib/include/snes/`. Built as separate LoROM, HiROM, SA-1, and SuperFX object sets.
- **templates/** — ROM bootstrap: `crt0.asm` (startup + NMI handler), `hdr*.asm` (ROM headers), `runtime.asm` (math routines, now in lib/source/), `memmap*.inc` (memory maps). These are the single source of truth — examples don't duplicate them.
- **make/common.mk** — Universal build rules included by every example. Handles graphics conversion, multi-file C compilation, SNESMOD audio, SA-1/SuperFX/HiROM mode selection, module linking.
- **tools/** — the `opensnes-*` tools (shipped): `opensnes-tileset`/`-sprite` (PNG→SNES tiles), `opensnes-music` (IT→SPC700), and the other asset tools; the converters' code lives in `tools/lib/`
- **examples/** — 61 ROMs organized by category (basics, fundamentals, text, backgrounds, sprites, scrolling, input, hdma, windows, color, transitions, mode7, maps, memory, audio, chips, games)

### Enhancement Chip Support

- **SA-1** (`USE_SA1=1`): Same 65816 ISA, clocked at 10.74 MHz (effective speed depends on memory and scene: `sa1_starfield` measures ~10.7 MHz from I-RAM, ~8.5 from ROM). Assembly only on the SA-1 side. Shares I-RAM ($3000-$37FF) with main CPU. Per-example `sa1_boot.asm` for custom coprocessor code. See `docs/tutorials/sa1.md`.
- **SuperFX** (`USE_SUPERFX=1`): Custom RISC ISA (GSU). Two-stage build: `.sfx` → `wla-superfx` → `wlalink -b` → `.sfx.bin` → `.incbin`. GSU code is assembly-only (no C compiler). **Validated by luna**, which detects and executes the GSU natively in the headless test harness (the old snes9x-WASM harness could not detect the GSU — "GSU: NOT DETECTED" — and needed a Mesen2 side channel; both are gone since the luna migration).

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

- C: 4 spaces, K&R braces, snake_case functions/vars, UPPER_CASE constants. Fixed-width types `u8`, `u16`, `s16`, `u32` (`snes.h`): `unsigned int` is 2 bytes, `unsigned long` 4 (`compiler/cproc/type.c`, `.claude/rules/compiler.md`).
- ASM: labels at column 0, instructions indented with a tab, `.section` for organisation.
- Commits: Conventional Commits, all authored and committed by `k0b3n4irb <k0b3n4irb@gmail.com>`, no tool identity, no `Co-Authored-By` (`.claude/rules/commits.md`). Scopes: `lib`, `compiler`, `runtime`, `tools`, `examples`, `build`, plus those of `devtools/lint_commits.py`.

## Critical Constraints

Silent failures: no error, just wrong behaviour. The public list with severity and mitigations is `KNOWN_LIMITATIONS.md`; keep this one in sync.

- **VRAM writes only work in VBlank or forced blank**; ~4 KB DMA per VBlank, more needs force blank (`setScreenOff/On`) or splitting. The DMA routines count and flag (`WDM $02`) a transfer made with the screen on outside VBlank.
- **Bank $00 ROM is code only** since #127.3: C const data lives in the asset banks and every C read of it is a far read; casting `const` away and reading through a plain pointer is the one bank-blind path (`opensnes-rom check` fails the link). Code that does not fit goes to bank $01+ by itself (`make test-bank-spill`); the free-space ratchet `BANK0_FAIL_THRESHOLD` is off by default.
- **Plain C RAM lives below $2000; `FAR` is the way above** (`snes/types.h`, `docs/tutorials/far_ram.md`): declare bulk buffers `FAR`; `opensnes-rom check` fails the link on a plain-band overflow.
- **cc65816 pushes args LEFT-TO-RIGHT**: ASM ported from PVSnesLib has swapped stack offsets (`compiler/ABI.md`).
- **`data_init_end.o` provides the data-init terminator**, listed last by `make/common.mk`; `opensnes-rom check` verifies it after every link.
- **The WRAM data port ($2180-$2183) is NOT safe in NMI.**
- **A SNESMOD effect that does not fit the SPC RAM left by the module overwrites the echo buffer**; size effects with `opensnes-music inspect`.
- **`volatile` is honoured by QBE** (each load/store survives); the lib still uses plain globals for NMI handshakes for cycle cost.
- **Vertical scroll is off by one on hardware**: the lib writes `y - 1` to `BGnVOFS`/`M7VOFS` for you; data reaching the register without the lib (HDMA tables on `BGnVOFS`) carries the -1 itself. BG3 in Modes 2/4/6 is the offset-per-tile table, written raw.
- **WLA-DX loses `.ACCU/.INDEX` tracking after branch merges**: add explicit `.ACCU 8/16` after every `rep`/`sep` in hand-written ASM (`devtools/lint_asm.py`).

## Rules and planning

`.claude/rules/*.md` load by context (those without `paths:` on every request): `debugging`, `partners` (luna, snes-rag, the game), `commits`, `memory_routing`; the others by the files you touch (`testing` with the change classes A/B/C/D, `compiler`, `release`, `doc_consistency` and its `make lint-docs` anchors, `new_example`, `hardware_claims`, `luna_tooling`, `abi_lint`, `nmi_audit`, `bank0_budget`, `templates`, `porting`, `two_audiences`). Changes of Class A and B (compiler, library, templates) are written by an Opus agent in its own worktree and re-validated by the session itself, every gate, before it merges; Sonnet is the default for the rest. `.claude/STRUCTURAL_DEFECTS.md` is the maintainer-internal catalogue of chantiers (estimates, risks, interaction matrix): consult it before a non-trivial chantier and update it when shipping or deferring one; the public docs absorb only the outcome.
