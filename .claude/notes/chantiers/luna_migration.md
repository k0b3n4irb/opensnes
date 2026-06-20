# Chantier — Migration test harness → Luna

**Goal:** replace `tools/opensnes-emu` (snes9x compiled to WASM) **and** Mesen2
with [luna](https://github.com/k0b3n4irb/luna), a cycle-accurate Rust SNES
emulator (headless CLI + API + MCP) that natively runs SA-1 / Super FX / DSP-1.

Full pre-migration report (exhaustive): `/tmp/luna_migration_report_2026-06-20.md`.

## Why (value-add)

- **Unifies two emulators into one** headless Rust binary; kills the heavy CI
  stack (Emscripten + WASM build + Node-core + Mesen2 + xvfb + ubuntu-22.04 pin).
- **Runs the chips snes9x can't**: snes9x reports "GSU: NOT DETECTED" for our
  SuperFX ROMs → the dual-baseline hack (`.bin` "boots only" + `.mesen2.bin`
  "the real one") disappears. luna v0.2.0 detects `mapper = SuperFx` and reaches
  `sa1_status = $A5` (= Mesen2), verified in the spike.
- More accurate (CPU/SPC 100% Tom Harte), richer introspection, deterministic.
- Aside: luna already defaults SIWP/CIWP to `$FF` *specifically for our
  `sa1_starfield` demo* — same convention as our crt0 (ties into finding P1-3).

## Locked decisions (2026-06-20)

| # | Decision |
|---|---|
| 1 | Baseline format = **both**: SHA-256 framebuffer gate + PNG debug |
| 2 | **Drop Mouse/Super Scope** (nothing to test it with). `input/mouse` + `input/superscope` → boot+visual only, no input coverage. Document in KNOWN_LIMITATIONS + their READMEs |
| 3 | Pass/fail computed **in the runner** (luna has no `--assert` yet); upstream `--assert` optional |
| 4 | Pin **luna v0.2.0** |
| 5 | **Rewrite the runner off Node** — Python (consistent with `devtools/*.py`), shelling out to `luna` |
| 6 | **Temporary double-run** CI (luna alongside the old suite) to prove value-add, then decommission |

## Progress

- ✅ **Phase 0 (spike) — GO** (2026-06-20). v0.2.0 aarch64 binary, SHA-256 OK,
  ran headless. hello_world / mapscroll(B1 bank-$02 map) render; superfx_hello →
  `SuperFx`; sa1_hello → `$A5`; framebuffer **byte-identical** across two runs.
- ✅ **Phase 1 (prototype, in progress)** — `tools/luna-test/luna_runner.py`:
  deterministic visual-regression over a 6-example first batch (text, maps,
  sprites, audio, superfx, sa1). `--update`/compare/`--only`, runner-side
  pass/fail. 6/6 baseline + 6/6 compare PASS; negative control (tampered hash)
  → FAIL rc=1. Baselines in `tools/luna-test/baselines/`.

- ✅ **Corpus coverage (branch `wip/luna-migration-corpus`)** — `luna_runner.py
  --coverage` runs every built example ROM headless and writes
  `tools/luna-test/CORPUS_COVERAGE.md`. Result: **57/57 OK, 0 SUSPECT, 0 FAIL**
  — luna renders 100% of the OpenSNES example suite (all BG modes 0/1/3/5/7 +
  mode7-perspective, HDMA gradient/wave/helpers, both SuperFX demos incl. a 3D
  wireframe cube, both SA-1 demos, HiROM, all games/sprites/audio/input). The
  GSU examples snes9x could never run are fully rendered. Spot-checked visually:
  mapscroll (bank-$02 map), superfx_hello (ALL TESTS PASSED), superfx_3d (3D
  cube), mode7 (PVSnesLib logo). `input_mouse`/`input_superscope` render fine —
  only their device *input* is uncovered (decision #2).

- ✅ **J0 prerequisites (P1-P3)** — `tools/luna-test/luna.version` (=v0.2.0),
  `scripts/install-luna.sh` (download+SHA-256+install to `bin/luna`, honours
  `$LUNA_BIN` for local-luna co-dev), runner resolves `bin/luna`. **N_corpus
  reconciled = 56** (git-tracked `main.c`). Finding: `examples/graphics/effects/
  hdma_gradient/` is **stale build residue** (no tracked `main.c`/`Makefile`) that
  inflated the earlier `.sfc` count to 57 — runner now discovers via `main.c`, so
  the phantom is excluded (56/56). The dir should be `rm`'d at some point.
- **Plan of record:** `/tmp/luna_migration_FINAL_2026-06-20.md`. Architecture
  CLOSED → Option B (Python runner + pinned native binary, no submodule, no WASM).
  WS-L luna features (G1 `framebuffer_hash` CLI / G2 `--assert` / G3 `$21FC`/WDM)
  are **optional enhancements, not gates** — migration + cutover need zero luna
  code. Cross-arch resolved by baselining on the CI arch (= snes9x's existing
  single-build-pin model).

- ✅ **WS-R 2.1 (manifest + liveness)** — `manifest.toml` (default_steps +
  per-example overrides + `input_dependent` for mouse/superscope) ; `liveness()`
  from `luna state` (NMI/VBlank advancing + CPU not stopped — the boot-offset
  frames−nmis is NOT gated, a first naive ratio gate false-flagged 7 healthy
  examples). Coverage now **54 OK / 2 INPUT-DEP / 0 DEAD / 0 FAIL** (honest, vs
  the earlier "57/57 renders").
- ✅ **WS-R 2.3 (functional probes)** — `tools/luna-test/probes/`: `lib.py`
  (symbol resolution via `.sym`, scripted `--input`, WRAM via `--peek`),
  `run_all.py`, and 2 ported probes (`controller` — input→pad_keys==mask;
  `mapscroll` — D-pad→xloc directional). Both PASS headless on luna. Pattern is
  portable; remaining ~20 `test/functional/*.test.mjs` are mechanical follow-up
  (mouse/superscope excluded — gap G4).

## Open / next

- ⚠️ **Cross-arch byte-stability of PNG baselines is UNVERIFIED** (baselines
  generated on aarch64). The #1 thing the double-run period must confirm. If
  x86_64 ≠ aarch64, fall back to: canonical-arch baseline, or hash raw RGBA /
  `framebuffer_hash()` instead of the encoded PNG. (See tool README.)
- Migrate remaining phases: runtime/WRAM (`state --peek`), lag (`state.stats`),
  input (`--input`), corpus (`bench`), MCP swap (`luna mcp`).
- Expand manifest to all 56 examples.
- CI: add a non-blocking luna job next to `functional-tests`; compare verdicts
  1-2 weeks; then rewrite the job and decommission `tools/opensnes-emu` (incl.
  the Node/TS runner, WASM core, Mesen2 install) + the `.bin`/`.mesen2.bin`
  baselines. Update `CLAUDE.md`, `.claude/rules/testing.md`, `KNOWN_LIMITATIONS.md`
  (the "snes9x can't detect GSU" entry becomes obsolete).

## Branch

`wip/luna-migration` (off develop). Reversible until the Phase-4
decommissioning; the old suite stays in place during the double-run.
