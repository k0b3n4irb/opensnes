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
the WRAM-state stream regression (`wram_regress.py`, whole corpus on both arches).
Compile-time cc65816 checks live in `devtools/compiler-tests/`.

## Requirements

- The pinned luna binary — the exact version lives in
  `tools/luna-test/luna.version` (the single source of truth; **`v1.14.0`** at
  time of writing). Resolution order: `$LUNA_BIN` → `luna` on `PATH` →
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

Everything the chantier scoped has landed — and the runtime probes have since
been migrated from Python (`probes/*.py`, now deleted) to native luna
manifests (`manifests/*.toml`, see the next section). Still here: the
WRAM-stream regression (`wram_regress.py`), input sequences (`--input`),
the full-corpus manifest, and the CI rewrite (both Linux arches). For
interactive debugging, use `luna mcp` / luna's GUI.

## Hardening tests (luna scripted-input & trace capabilities)

Beyond visual/coverage, the harness exercises axes the old snes9x harness
never could. **These checks now live as native luna manifests under
`manifests/*.toml`** (run by `luna test` via `make test-manifests`); the
Python probes that pioneered them were deleted after the migration —
`probes/` retains only `lib.py` (helper API, used by `project_test.py`)
and `run_all.py`. Same coverage, declarative form:

- **Coprocessor execution** (`manifests/coproc_*.toml`) — SA-1, Super FX
  and DSP-1 examples must execute ≥1 coprocessor instruction
  (`[asserts.trace]`); sa1_hello additionally asserts `sa1_status = 0xA5`.
  The DSP-1 manifest is firmware-gated (`firmware = "dsp1b.rom"`) and
  SKIPs cleanly when the dump is absent.
- **SRAM persistence** (`manifests/a_sram_write.toml` & friends) —
  battery save round-trip via `srm_out`/`srm_in` with block asserts.
- **Mouse / Super Scope** (`manifests/mouse.toml`,
  `manifests/superscope.toml`) — scripted peripheral input with
  checkpointed WRAM value asserts.
- **Audio** (`manifests/audio_v2.toml`) — `[asserts.dsp]` voice + PCM
  liveness on the raw-APU driver fixture.
- **WRAM-state regression** (`wram_regress.py`, `make test-wram`, H7) — per-frame
  `wram-trace` hash stream vs a baseline; catches runtime-state regressions
  invisible to the framebuffer. **Local, same-arch tool — not a CI gate:** raw
  WRAM content (unlike the framebuffer) isn't a luna cross-arch guarantee
  (mapandobjects, slope_collision diverge x86_64 ↔ aarch64), so `--update` on your own
  machine before `--compare`. Baseline entries carry `rom_sha256` provenance
  (#120): a mismatch reports whether the ROM itself changed vs the capture, and
  `--update` refuses a stale tree (corpus-fresh guard #105 + per-example
  source-mtime check) so stale-ROM rebaselines fail at capture time.
- **VRAM-DMA timing safety + budget** (`manifests/dma_*.toml`, H2) — luna
  tags each `--dma-trace` write with `force_blank` (INIDISP), so a write is safe
  iff `blank || force_blank`. The probe asserts **zero unsafe writes** (active
  display, screen on — the #1 silent failure, now testable) and that the per-VBlank
  peak stays ≤ 4 KB (real bytes now: `dynamic_metasprite` peaks ~3.5 KB).
