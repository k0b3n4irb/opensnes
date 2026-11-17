# CLAUDE.md

Guidance for Claude Code in this repository. Rules in `.claude/rules/*.md` load by the files you touch (compiler, testing, release, partners, debugging, docs…).

## Design philosophy

Before a refactor, read **`PHILOSOPHY.md`** (five principles and the "Non-goals" are acceptance criteria): OpenSNES is a 2D game engine, not a thin C-over-asm wrapper.

## Build and test

```bash
make                  # compiler → tools → library → examples
make compiler|tools|lib|examples
make clean && make    # REQUIRED after a compiler or runtime change
cd examples/text/print_string && make     # one example
scripts/install-luna.sh                   # pinned luna (testing/luna.version)
python3 testing/luna_runner.py --coverage|--compare
make test-manifests                       # functional probes
make tests                                # all of the above
```

All testing goes through luna (cycle-accurate, runs SA-1 / Super FX / DSP-1). Changes are classed A/B/C/D (`.claude/rules/testing.md`). Pipeline: `main.c → cproc → QBE w65816 → wla-65816 → wlalink → game.sfc`, driven by `bin/cc65816`.

## Layout

`compiler/` (cproc, qbe, wla-dx submodules), `lib/` (`source/`, `include/snes/`), `templates/` (crt0, headers, memmap: single source of truth), `make/common.mk` (build rules of every example), `tools/` (`opensnes-*`), `examples/`.

Example Makefile: `OPENSNES := $(shell cd ../../.. && pwd)`, `TARGET`, `ROM_NAME`, `USE_LIB := 1`, `LIB_MODULES := …`, `CSRC := main.c`, `include $(OPENSNES)/make/common.mk`. SA-1: `USE_SA1=1`; SuperFX: `USE_SUPERFX=1` (`docs/tutorials/sa1.md`).

## Style and commits

C: 4 spaces, K&R, snake_case, UPPER_CASE constants, `u8 u16 s16 u32` (`unsigned int` 2 bytes, `unsigned long` 4). ASM: labels at column 0, tab-indented instructions. Commits: Conventional Commits `type(scope): desc`, one author `k0b3n4irb`, no tool identity, no `Co-Authored-By`; `make hooks` installs the lint (`.claude/rules/commits.md`).

## Silent failures (public list: `KNOWN_LIMITATIONS.md`)

- VRAM writes only in VBlank or forced blank; ~4 KB DMA per VBlank.
- Bank $00 ROM is code only; C const reads are far reads (`opensnes-rom check`).
- Plain C RAM is below $2000; `FAR` above (`docs/tutorials/far_ram.md`).
- cc65816 pushes args LEFT-TO-RIGHT (`compiler/ABI.md`).
- The WRAM data port ($2180-$2183) is not safe in NMI.
- Vertical scroll is off by one on hardware: the lib writes `y - 1`; HDMA tables carry it themselves.
- WLA-DX loses `.ACCU/.INDEX` after branch merges: explicit `.ACCU 8/16` after every `rep`/`sep`.
- A SNESMOD effect too big for the SPC RAM overwrites the echo buffer.

## Rules and planning

Bug seen in an example: name the layer (compiler, lib, templates, build) and fix there, never a workaround in the example; a regression is bisected (`.claude/rules/debugging.md`). Never name the private game in this repository (`.claude/rules/partners.md`). Class A/B changes (compiler, lib, templates) are written by an Opus agent in its own worktree and re-validated by the session, every gate. The strongest model gets one bounded, written task at a time (`.claude/rules/fable_calls.md`). Project knowledge goes to `.claude/notes/` (`memory_routing.md`). Chantiers: `.claude/STRUCTURAL_DEFECTS.md`.
