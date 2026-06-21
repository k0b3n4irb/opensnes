# luna-test — Luna-backed test harness (migration, Phase 1)

Prototype replacement for `tools/opensnes-emu` (snes9x-WASM) + Mesen2, built on
[luna](https://github.com/k0b3n4irb/luna) — a cycle-accurate Rust SNES emulator
that runs headless, detects/executes SA-1 + Super FX (GSU) + DSP-1, and exposes
the full machine state via CLI / API / MCP.

See the full plan: `/tmp/luna_migration_report_2026-06-20.md` and
`.claude/notes/chantiers/luna_migration.md`.

## Status

**Active backend.** luna is now the **sole** test backend: the snes9x-WASM +
Mesen2 harness (`tools/opensnes-emu`) was removed. `make tests` runs corpus
liveness coverage + full-corpus visual regression (56 examples) + functional
probes. Compile-time cc65816 checks live in `devtools/compiler-tests/`.

## Requirements

- The pinned luna binary — version in `tools/luna-test/luna.version` (currently
  **`v0.3.2`**). Resolution order: `$LUNA_BIN` → `luna` on `PATH` →
  `tools/luna-test/vendor/luna-<version>-linux-<arch>/luna`. Install with
  `scripts/install-luna.sh` (downloads the pinned tag + verifies its `.sha256`).
- Python 3 (stdlib only — consistent with `devtools/*.py`). **No Node, no
  Emscripten, no WASM, no Mesen2, no xvfb.**

## Usage

```bash
export LUNA_BIN=/path/to/luna             # or put `luna` on PATH
python3 tools/luna-test/luna_runner.py --list      # show the manifest
python3 tools/luna-test/luna_runner.py --update    # (re)write baselines
python3 tools/luna-test/luna_runner.py             # compare → exit 0/1
python3 tools/luna-test/luna_runner.py --only sa1  # one label substring
```

## How it works

For each example the runner calls `luna run -n <steps> --screenshot <png>` and
keys the regression on the **SHA-256 of the rendered PNG**. luna's headless
framebuffer is byte-deterministic run-to-run (verified in the Phase 0 spike), so
the hash is a stable, drift-free gate; the PNG is kept next to it for human
diffing (decision #1: hash gate **+** PNG debug). The runner computes the visual
verdict itself; luna v0.3.0 also provides `--assert BANK:OFFSET=HEX` (+ `-aram`/
`-vram`) for direct WRAM assertions, and `--print-fbhash` for a cross-arch-stable
framebuffer key (see the cross-arch note below).

Baselines live in `baselines/`: `<label>.png` + a single `baselines.json`
manifest (`sha256`, `steps`, `rom_sha256`, `luna_version`).

## Cross-arch baseline key

The regression key is luna's **`--print-fbhash`** (v0.3.0) — a hash of the
pre-PNG pixels luna documents as **cross-architecture-stable**. So the baselines
committed here (captured on aarch64) are expected to match on the x86_64 CI
runner, and the CI visual step is a **hard gate** (no `continue-on-error`). The
PNG is kept only for human diffing, not hashed. If a future luna release ever
breaks fbhash cross-arch stability, that's a luna bug — regenerate baselines with
`luna_runner.py --update` on the CI arch as a stopgap and report it.

## Not yet migrated (tracked in the chantier note)

Runtime/WRAM probes (`luna state --peek`), lag detection (`state.stats`), input
sequences (`--input`), `luna bench` corpus run, the MCP swap (`luna mcp`), the
full 56-example manifest, and the CI rewrite. Mouse/Super Scope coverage is
**dropped** (decision #2) — `input/mouse` + `input/superscope` get boot+visual
validation only.

## Hardening tests (luna v0.3.0 capabilities)

Beyond visual/coverage/probes, the harness exercises axes the old snes9x harness
never could (see `/tmp/luna_test_hardening_ideas.md` for the full list):

- **Audio** (`probes/audio.py`, H5) — SNESMOD examples must have ≥1 active SPC
  voice + non-silent PCM (`--audio-out`); the SFX driver must be alive.
- **WRAM-state regression** (`wram_regress.py`, `make test-wram`, H7) — per-frame
  `wram-trace` hash stream vs a committed baseline; catches runtime-state
  regressions invisible to the framebuffer. Cross-arch stable.
- **VBlank DMA budget** (`probes/dma_budget.py`, H2) — estimates steady-state
  VRAM-DMA bytes/frame and flags > ~4 KB/VBlank. Estimate pending luna L13
  (frame column on `--dma-trace`).
