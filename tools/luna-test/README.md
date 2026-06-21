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

- The pinned luna binary, **`v0.3.0`** (decision #4). Resolution order:
  `$LUNA_BIN` → `luna` on `PATH` → `tools/luna-test/vendor/luna-v0.3.0-linux-<arch>/luna`.
  Download: `gh release download v0.3.0 --repo k0b3n4irb/luna --pattern '*linux-<arch>*'`
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
diffing (decision #1: hash gate **+** PNG debug). The runner computes the visual
verdict itself; luna v0.3.0 also provides `--assert BANK:OFFSET=HEX` (+ `-aram`/
`-vram`) for direct WRAM assertions, and `--print-fbhash` for a cross-arch-stable
framebuffer key (see the cross-arch note below).

Baselines live in `baselines/`: `<label>.png` + a single `baselines.json`
manifest (`sha256`, `steps`, `rom_sha256`, `luna_version`).

## ⚠️ Open validation item — cross-machine / cross-arch byte-stability

The baselines committed here were generated with the **aarch64** v0.3.0 binary,
keyed on the SHA-256 of the encoded PNG. Run-to-run determinism is verified, and
v0.2.0→v0.3.0 produced byte-identical baselines (no re-baseline on the bump).
**Cross-arch (x86_64 ↔ aarch64) byte-identity is the remaining check** (the CI
runs the visual step `continue-on-error` until confirmed). luna v0.3.0 now ships
`--print-fbhash` (`fbhash=<hex>`) — a hash of the pre-PNG pixels described as
cross-architecture-stable — so the clean fix is to switch the runner's key from
the PNG hash to `--print-fbhash`; do that once a multi-arch CI run confirms it.

## Not yet migrated (tracked in the chantier note)

Runtime/WRAM probes (`luna state --peek`), lag detection (`state.stats`), input
sequences (`--input`), `luna bench` corpus run, the MCP swap (`luna mcp`), the
full 56-example manifest, and the CI rewrite. Mouse/Super Scope coverage is
**dropped** (decision #2) — `input/mouse` + `input/superscope` get boot+visual
validation only.
