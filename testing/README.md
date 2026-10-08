# testing/ — the luna-backed test harness (contributor-only, not shipped)

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
  `testing/luna.version` (the single source of truth; this README
  deliberately does not repeat the number). Resolution order: `$LUNA_BIN` →
  `testing/bin/luna` (where the installer puts it) → `luna` on `PATH`. Install with
  `scripts/install-luna.sh` (downloads the pinned tag's zip and checks it against `luna.sha256`, the sums pinned here).
- Python 3 (stdlib only — consistent with `devtools/*.py`). **No Node, no
  Emscripten, no WASM, no Mesen2, no xvfb.**

## Usage

```bash
export LUNA_BIN=/path/to/luna             # or put `luna` on PATH
python3 testing/luna_runner.py --list      # show the manifest
python3 testing/luna_runner.py --update    # (re)write baselines
python3 testing/luna_runner.py             # compare → exit 0/1
python3 testing/luna_runner.py --only sa1  # one label substring
```

## How it works

For each example the runner calls `luna run --until-frame <N> --print-fbhash
--screenshot <png>` and keys the regression on **luna's `fbhash`** — a hash of
the pre-PNG pixels, byte-deterministic run-to-run and cross-arch-stable (see
the note below); the PNG is kept next to it for human diffing (hash gate **+**
PNG debug). luna also provides `--assert BANK:OFFSET=HEX` (+ `-aram`/`-vram`)
for direct WRAM assertions, used by the probes.

Baselines live in `baselines/`: `<label>.png` + a single `baselines.json`
manifest (`fbhash`, `frames`, `rom_sha256`, `luna_version`). Self-animating
examples opt into MULTIPLE capture points via `manifest.toml`
`frames = [a, b]` — `fbhash`/`frames` become lists, extra PNGs are
`<label>@<frame>.png`, and a partial mismatch is reported as "phase drift?".

## Power-on state and the A/B protocol

Every runner pass accepts `--power-on zero|ones|random[=seed]`, handed to
luna as is. `--power-on random=1` boots each ROM from pseudo-random
WRAM/VRAM/CGRAM/OAM/ARAM with a fixed seed: a ROM that reads memory it
never initialised fails deterministically instead of passing on luna's
default zero-fill (the class of the v0.40.0 / v0.41.1 reset-vector fixes,
which luna could not see before). `make tests` runs the liveness pass
that way in addition to the default one.

`hardware_preflight.py` (`make hardware-preflight`, 2026-10-03) is the
same idea aimed at a console session: the ROMs of
`docs/HARDWARE_VERIFICATION.md` — read from the protocol's table, like
`scripts/hardware-kit.sh` — each replayed from three random seeds and
under `--force-region pal`, plus the VRAM-DMA-in-blank check of
`vram_dma_blank.py`. It reports ready / FAIL per row and exits 1 on any
failure; `--rows 1-7` restricts it to the gate rows.

`diff_corpus.py --ref <dir>` is the Class A proof for a compiler or
library change: for every example it runs `luna diff <ref rom> <new rom>
--frames <manifest frames> --tolerance N` and prints MATCH (with the
boot-length offset luna found) or DIFF (PNG pairs under `/tmp/luna-diff/`).
The reference tree is the `examples/` ROMs built before the change; a
DIFF is a rendering change to explain, never something to re-baseline
over.

## Measured ROM coverage (the never-executed ratchet)

`rom_coverage.py` asks luna for the set of executed PCs of every ROM
(`luna profile --pc-set`), folds them onto the `.sym` labels (FastROM/HiROM
mirrors folded like `symmap.py`) and unions the hits. Each example ROM is
profiled once input-free to its first capture frame and once per `luna
test` manifest that names it, replaying the manifest's joypad-1 script (the
checkpoints merged into one timeline, as `luna test` does) to its last
checkpoint or `frames` / `steps` bound; the library fixture
(`testing/fixtures/libtests/libtest.sfc`), its sibling for the modules it has no
RAM for (`testing/fixtures/libtests_fx/`: hdma, mode7, SNESMOD) and the compiler's
five runtime ROMs are profiled as well. The public
functions of `lib/include/snes/*.h` that nothing executes are written to
`baselines/never_executed.txt`; `make tests` fails if that set gains a
name (a function shipped with no example, no manifest leg and no libtest)
and reports names that became executed so `--update` can shrink the list.
`ROM_COVERAGE.md` is the human report. Mouse and Super Scope scripts are
not replayed — `luna profile` has `--input` only — so those examples'
peripheral paths are still under-counted (2026-09-19: 167 → 98 never
executed when the manifest legs and the fixture were added).

## Cross-arch baseline key

The regression key is luna's **`--print-fbhash`** (since v1.21.0 "fbhash v2":
FNV-1a 64 over the raw RGBA bytes of the displayed frame, pinned by
construction — the v1 key was Rust's `DefaultHasher`, not guaranteed stable
across toolchains; every baseline was re-keyed once at the switch) — a hash of the
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

## Writing a manifest: the vocabulary, and what it cannot express

Everything below was established by probing the pinned luna binary while
writing the R7 manifests (2026-09-15/16) — `docs/tools/luna.md` is generated
from `--help` and does not carry the schema.

**Top-level assert namespaces**, exactly these (luna's own error text lists
them): `wdm_empty`, `nocash_contains`, `fbhash`, `audio_rms_min`, `values`,
`blocks`, `trace`, `dsp`, `footprint`, `dma`, `oam`. They are evaluated at the
run bound, not per checkpoint.

**Checkpoints** accept `at_frame`, `input`, `input2`, `mouse`, `superscope`,
`values` and `delta` — and nothing else. So OAM, VRAM/CGRAM blocks and DSP
registers can only be asserted once, at the end of the run: an example whose
DSP state must be compared before and after an event needs **two manifests**,
not two checkpoints.

- `values` comparisons: `eq`, `ne`, `lt`, `le`, `gt`, `ge` (combinable, e.g.
  `{ ge = 100, le = 200 }`), with `width` 1, 2 or 4. A `width = 4` read works,
  but the expected value must still fit in 16 bits.
- `delta` directions: `increased`, `decreased`, `changed`, `unchanged`.
- Addresses are a `.sym` symbol or a raw `BANK:OFFSET` string; the run bound is
  `frames = N` (or `steps = N` for instruction-count tests).
- `blocks` keys must themselves be a symbol or `BANK:OFFSET`, even when
  `space` and `offset` are given explicitly.

**Input scripts are merged into one timeline.** Every checkpoint's `input` /
`mouse` / `superscope` entries are frame-stamped and combined for the whole
run, so a checkpoint observes every event scheduled before its frame, not only
the ones written beside it. Reading a checkpoint as if it replayed its own
script in isolation is the easiest way to write a wrong expectation.

**PPU registers are assertable since luna v1.24.0**: `[asserts.ppu]` at the
run bound and `[checkpoint.ppu]` per checkpoint compare against the `ppu`
block of `luna state --out -`, keyed by its JSON field names (`w12sel`,
`tmw`, `inidisp`, `bgmode`, `m7a`…; `.` steps into arrays, the Mode 7
fields are signed, there is no `width`). Reading an MMIO address through
`values` still returns 0 (it resolves WRAM). The library's WRAM shadows —
`hdma_enabled_state` in `hdma.asm`, `hdma_wave_amplitude`, `m7_sin` /
`m7_cos` / `m7_scale` in `mode7.c` — remain useful for what the PPU block
does not show (which HDMA channels the lib believes are on), and a shadow
is only trustworthy for an example that goes through the module.

## Hardening tests (luna scripted-input & trace capabilities)

Beyond visual/coverage, the harness exercises axes the old snes9x harness
never could. **These checks now live as native luna manifests under
`manifests/*.toml`** (run by `luna test` via `make test-manifests`); the
Python probes that pioneered them were deleted after the migration —
`lib/probes.py` is the `luna state --assert` / `--peek`
helper every runtime ROM checker imports (`from lib import assert_mem`) (`testing/fixtures/compiler/*`,
`testing/fixtures/libtests`; `project_test.py` too, until a project's `make test` became `luna test` on 2026-10-05); the `run_all.py` runner that globbed
the emptied directory was deleted on 2026-09-14. Same coverage, declarative form:

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
- **Compiler differential test** (`difftest.py`, `make test-difftest`, in
  `make tests` since 2026-10-08) — random C integer expressions compiled by
  cc65816 and run on luna in four shapes (operands as globals, as literals,
  as parameters, as locals), compared with the value C gives them on this
  target (int 16 bits, long 32). The expected value comes from a model in
  the script that clang checks expression by expression (`_Static_assert`
  under `--target=avr`), so a mistake in the model is reported as such. The
  gate is fixed: ten pinned expressions and fifteen seeds (370 expressions);
  `make test-difftest SEEDS=1000-1999` hunts, a failing expression is
  reduced to its smallest failing sub-expression, and `--cc <path>` runs
  another compiler (bisecting). Its first day found six compiler defects, three
  of them within the first 192 expressions (`.claude/notes/status/silent_defects_log.md`).
  It covers integer expressions only; the programs are the next entry.
- **Compiler differential test, programs** (`difftest_stmt.py`, same target,
  in `make tests` since 2026-10-08) — small generated functions: locals,
  parameters and global scalars, arrays in one and two dimensions, a struct
  reached by name, through a pointer and in an array, bit-fields, counted
  loops in four spellings with `break` and `continue`, a pointer walking an
  array, `if` / `else`, `switch` with fall-through, compound assignments,
  `++` / `--`, stores through a pointer to a pointer, and calls to helper
  functions generated with the program (pure, recursive, taking an array, a
  struct pointer or a pointer to write through, two behind a table of
  function pointers), a union read through its other members, array members
  of the struct, static locals, loops and skips written with `goto`. The ROM
  returns a checksum
  of everything the function left behind; the expected one comes from an
  interpreter in the script that evaluates each expression with the model
  above. A program that would meet undefined behaviour is thrown away. A
  failing program is reduced statement by statement (a program that never
  returns counts as failing), then run again writing each variable out, so
  the report names the wrong one. Gate: eleven pinned functions and twenty-one
  seeds (221 programs). Its first day: a loop-carried copy
  (`prev = cur; cur += d;`), a swap in a loop and Fibonacci miscompiled, a
  `do … while` with a `break`, initialisers starting with implicit zeros, a
  parameter read from an unwritten slot in a function of more than 256
  temporaries, an internal error on `a[x & 7]` with a long `x`, and a 32-bit
  compare reading a neighbour's stack slot. Not covered: floats, `long long` and structs
  passed by value (all three refused by the compiler), nested function
  pointer types, `setjmp`.
- **WRAM-state regression** (`wram_regress.py`, `make test-wram`, H7) — per-frame
  `wram-trace` hash stream vs a baseline; catches runtime-state regressions
  invisible to the framebuffer. **A CI gate since 2026-09-11**, inside `make tests`
  on both luna legs; raw WRAM content isn't a luna cross-arch guarantee for two
  examples (mapandobjects, slope_collision diverge x86_64 ↔ aarch64), which the
  oracle skips per arch. Baseline entries carry `rom_sha256` provenance
  (#120): a mismatch reports whether the ROM itself changed vs the capture, and
  `--update` refuses a stale tree (corpus-fresh guard #105 + per-example
  source-mtime check) so stale-ROM rebaselines fail at capture time.
- **VRAM-DMA timing safety + budget** (`manifests/dma_*.toml`, H2) — luna
  tags each `--dma-trace` write with `force_blank` (INIDISP), so a write is safe
  iff `blank || force_blank`. The probe asserts **zero unsafe writes** (active
  display, screen on — the #1 silent failure, now testable) and that the per-VBlank
  peak stays ≤ 4 KB (real bytes now: `dynamic_metasprite` peaks ~3.5 KB).
