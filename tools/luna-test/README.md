# luna-test — Luna-backed test harness

The project's test harness, built on
[luna](https://github.com/k0b3n4irb/luna) — a cycle-accurate Rust SNES emulator
that runs headless, detects/executes SA-1 + Super FX (GSU) + DSP-1, and exposes
the full machine state via CLI / API / MCP.

Migration history: `.claude/notes/chantiers/luna_migration.md`.

## Status

**Sole test backend** (the snes9x-WASM + Mesen2 harness `tools/opensnes-emu`
was removed). `make tests` runs corpus liveness coverage + full-corpus visual
regression + functional probes (scripted input → WRAM asserts), and CI adds
the WRAM-state stream regression (`wram_regress.py`, cross-arch exclusions).
Compile-time cc65816 checks live in `devtools/compiler-tests/`.

## Requirements

- The pinned luna binary — version in `tools/luna-test/luna.version` (currently
  **`v1.1.0`**). Resolution order: `$LUNA_BIN` → `luna` on `PATH` →
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

For each example the runner calls `luna run -n <steps> --print-fbhash
--screenshot <png>` and keys the regression on **luna's `fbhash`** — a hash of
the pre-PNG pixels, byte-deterministic run-to-run and cross-arch-stable (see
the note below); the PNG is kept next to it for human diffing (hash gate **+**
PNG debug). luna also provides `--assert BANK:OFFSET=HEX` (+ `-aram`/`-vram`)
for direct WRAM assertions, used by the probes.

Baselines live in `baselines/`: `<label>.png` + a single `baselines.json`
manifest (`fbhash`, `steps`, `rom_sha256`, `luna_version`). Self-animating
examples opt into MULTIPLE capture points via `manifest.toml`
`steps = [a, b]` — `fbhash`/`steps` become lists, extra PNGs are
`<label>@<steps>.png`, and a partial mismatch is reported as "timing drift?".

## Cross-arch baseline key

The regression key is luna's **`--print-fbhash`** (v0.3.0) — a hash of the
pre-PNG pixels luna documents as **cross-architecture-stable**. So the baselines
committed here (captured on aarch64) are expected to match on the x86_64 CI
runner, and the CI visual step is a **hard gate** (no `continue-on-error`). The
PNG is kept only for human diffing, not hashed. If a future luna release ever
breaks fbhash cross-arch stability, that's a luna bug — regenerate baselines with
`luna_runner.py --update` on the CI arch as a stopgap and report it.

## Migration complete

Everything the chantier scoped has landed: runtime/WRAM probes (`probes/`,
incl. mouse + Super Scope via luna v1.1.0's scripted peripheral input), the
WRAM-stream regression (`wram_regress.py`, CI-gated with cross-arch
exclusions), input sequences (`--input`), the full-corpus manifest, and the
CI rewrite (both Linux arches). For interactive debugging, `luna mcp` /
luna's GUI are available alongside Mesen2.

## Hardening tests (luna v1.1.0 capabilities)

Beyond visual/coverage/probes, the harness exercises axes the old snes9x harness
never could (see `/tmp/luna_test_hardening_ideas.md` for the full list):

- **Coprocessor execution** (`probes/coproc.py`) — Super FX (`--superfx-trace`)
  and SA-1 (`--sa1-trace`) examples must execute ≥1 coprocessor instruction. The
  old harness could not even *detect* the GSU ("GSU: NOT DETECTED"); luna runs it
  natively and this turns that into a positive, regression-guarding assertion (a
  silent codegen/template break that stops the chip is invisible to the
  framebuffer gate).
- **SRAM persistence** (`probes/sram.py`) — battery save round-trip on
  `save_game`: drive a save, persist the battery with `--srm-out`, power-cycle by
  reloading it with `--srm-in`, assert the loaded struct matches, with a
  no-battery negative control proving the match came from the file (not ROM
  determinism). Exercises `snes/sram.h` end-to-end.
- **Mouse input** (`probes/mouse.py`) — luna v1.1.0 `--port1 mouse --mouse`
  injects SNES Mouse motion; the probe drives sustained motion and asserts the
  cursor saturates to the clamp ((255,223) right/down, (0,0) left/up) — a
  sensitivity-independent deterministic check that `input/mouse` decodes deltas.
- **Super Scope** (`probes/superscope.py`) — `--port2 superscope --superscope`
  fires the light gun at a known aim; the probe asserts the lib's decoded raw beam
  position (`input/superscope`). Closes the last two boot+visual-only examples.
- **Audio** (`probes/audio.py`, H5) — SNESMOD examples must have ≥1 active SPC
  voice + non-silent PCM (`--audio-out`); the SFX driver must be alive.
- **WRAM-state regression** (`wram_regress.py`, `make test-wram`, H7) — per-frame
  `wram-trace` hash stream vs a baseline; catches runtime-state regressions
  invisible to the framebuffer. **Local, same-arch tool — not a CI gate:** raw
  WRAM content (unlike the framebuffer) isn't a luna cross-arch guarantee
  (mapandobjects, slopemario diverge x86_64 ↔ aarch64), so `--update` on your own
  machine before `--compare`. Baseline entries carry `rom_sha256` provenance
  (#120): a mismatch reports whether the ROM itself changed vs the capture, and
  `--update` refuses a stale tree (corpus-fresh guard #105 + per-example
  source-mtime check) so stale-ROM rebaselines fail at capture time.
- **VRAM-DMA timing safety + budget** (`probes/dma_budget.py`, H2) — luna v1.1.0
  tags each `--dma-trace` write with `force_blank` (INIDISP), so a write is safe
  iff `blank || force_blank`. The probe asserts **zero unsafe writes** (active
  display, screen on — the #1 silent failure, now testable) and that the per-VBlank
  peak stays ≤ 4 KB (real bytes now: `dynamic_metasprite` peaks ~3.5 KB).
