# luna-test — Luna-backed test harness (migration, Phase 1)

Prototype replacement for `tools/opensnes-emu` (snes9x-WASM) + Mesen2, built on
[luna](https://github.com/k0b3n4irb/luna) — a cycle-accurate Rust SNES emulator
that runs headless, detects/executes SA-1 + Super FX (GSU) + DSP-1, and exposes
the full machine state via CLI / API / MCP.

See the full plan: `/tmp/luna_migration_report_2026-06-20.md` and
`.claude/notes/chantiers/luna_migration.md`.

## Status

**Phase 1 (prototype).** One component so far: a deterministic visual-regression
runner over a first batch of 6 examples (basic render, sprites, scrolling map,
audio, Super FX, SA-1). Runs **alongside** the existing snes9x/Mesen2 suite —
nothing is removed yet (decision #6: temporary double-run to prove the value-add).

## Requirements

- The pinned luna binary, **`v0.2.0`** (decision #4). Resolution order:
  `$LUNA_BIN` → `luna` on `PATH` → `tools/luna-test/vendor/luna-v0.2.0-linux-<arch>/luna`.
  Download: `gh release download v0.2.0 --repo k0b3n4irb/luna --pattern '*linux-<arch>*'`
  (verify the `.sha256`).
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
diffing (decision #1: hash gate **+** PNG debug). The runner computes pass/fail
itself — luna has no `--assert`/exit-code flag yet (decision #3).

Baselines live in `baselines/`: `<label>.png` + a single `baselines.json`
manifest (`sha256`, `steps`, `rom_sha256`, `luna_version`).

## ⚠️ Open validation item — cross-machine / cross-arch byte-stability

The baselines committed here were generated with the **aarch64** v0.2.0 binary.
Run-to-run determinism on one machine is verified; **cross-arch / cross-build
byte-identity of the PNG is NOT yet verified** and is the #1 thing the temporary
double-run CI period (decision #6) must confirm. If x86_64 ↔ aarch64 PNGs
differ, the fallback strategy is one of: (a) baseline on a single canonical arch
in CI; (b) switch the regression key from the encoded-PNG hash to a hash of the
raw RGBA framebuffer / luna's own `framebuffer_hash()`; (c) a small perceptual
tolerance for the debug diff while keeping an exact key per arch.

## Not yet migrated (tracked in the chantier note)

Runtime/WRAM probes (`luna state --peek`), lag detection (`state.stats`), input
sequences (`--input`), `luna bench` corpus run, the MCP swap (`luna mcp`), the
full 56-example manifest, and the CI rewrite. Mouse/Super Scope coverage is
**dropped** (decision #2) — `input/mouse` + `input/superscope` get boot+visual
validation only.
