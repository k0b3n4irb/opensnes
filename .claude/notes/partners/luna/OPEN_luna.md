# OpenSNES → luna : open report (accumulating since 2026-09-24)

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop` @ `c9ddef1d`, post-v0.44.0) |
| **luna in use** | v1.24.0 (1.26.0 pin bump queued behind the tag) |
| **Status** | accumulating; each item re-checked against `luna <cmd> --help` and a run on the day it was written |

## 1. What luna made possible since the 2026-09-21 reply

- The v0.44.0 release ran the whole corpus on both Linux legs of
  `release.yml` on the tagged binary — the first release whose artefacts
  had executed ROMs on their own arch before shipping.
- `luna diff` at ±0 is what let 94 asset sections move out of bank $00 in
  one commit with a pixel-identical corpus, and it is what caught the
  object engine reading the map from the wrong bank the first time a map
  left bank $00 (two DIFFs, then none once the bank was honoured).

## 2. Requests, simplest first — all about the Super FX

Context: `.claude/notes/reviews/2026-09-24_superfx_game_gaps.md`. We are
sizing what a real GSU game needs from the SDK; these are the parts only
the emulator can give us. Nothing is filed; every item is what we would
run tomorrow.

### L1 — a `gsu` block in `luna state --out -`

**Kind:** parity with the CPU block. **Cost:** small (the state exists).

`luna state --out -` on `examples/chips/superfx_3d/superfx_3d.sfc` has no
key naming the GSU (checked 2026-09-24: no `gsu` / `superfx` / `coproc`
top-level key). We can trace it (`--superfx-trace`) but not *assert* on
it. Ask: `"gsu": { "present": true, "version": 4, "running": false, "sfr":
…, "pbr": …, "r": [16 values], "cbr": …, "scmr": {"ron":1,"ran":1,"ht":…,
"md":…}, "scbr": …, "instructions_executed": N }` — and the same keys under
`[asserts.gsu]` / `[checkpoint.gsu]` in `luna test`, like `[asserts.dsp]`.

### L2 — a bus-ownership diagnostic

**Kind:** new diagnostic, the class of bug nothing else can see.
**Cost:** medium.

While SCMR RON/RAN grant the cartridge to the GSU, a 65816 read of Game
Pak ROM returns a dummy byte keyed on the address's low nibble and a read
of cart RAM returns open bus (sneslab "Bus Conflicts"; the dummy-byte
table is how the `$0108` NMI vector trick works). A game that forgets
this — an NMI handler still in ROM, a C function that touches `$70:xxxx`
mid-frame — reads garbage silently. Ask: a trace / counter of every CPU
access to ROM or cart RAM while the GSU owns it (`seq, frame, line, pc,
addr, kind`), the way `--trace-writes` reports writes, plus a one-line
count in `--out -`. This is the check that would let us build the
"CPU keeps running during GSU jobs" runtime with confidence.

### L3 — GSU cycles in `luna profile`

**Kind:** parity. **Cost:** medium.

`profile` is 65816-only (no GSU flag in `--help`). For a renderer, the
numbers that matter are: GSU cycles per job (between GO and STOP), cache
hit ratio, cycles spent in WAIT on RON/RAN, and CPU cycles spent waiting
for the GSU. Ask: a `gsu` section in the profile report with those four,
per job (a job = GO → STOP), and `--pc-set` covering GSU PCs so our
coverage ratchet can count `.sfx` code.

### L4 — `run_until` GSU STOP / GO (MCP) and a GSU disassembly view

**Kind:** debugging ergonomics. **Cost:** small if L1 exists.

Stepping a GSU job today means diffing a 200 000-line CSV. Ask: an MCP
`run_until_gsu_stop` (and `_go`), and the `gsu` block of L1 readable from
the debugger, with the current instruction disassembled.

## 3. Priority, from our side

| # | Request | Why |
|---|---|---|
| L1 | `gsu` block + manifest asserts | turns the GSU from "it renders" into something the fixtures can pin |
| L2 | bus-ownership diagnostic | the one bug class of a real GSU game that is silent everywhere else |
| L3 | GSU cycles in profile | frame budget of a renderer, and coverage of `.sfx` code |
| L4 | run-until STOP, disassembly | ergonomics |

## 4. Small observations (no ask)

- `--superfx-trace` with a 200 000 cap is the right default for a frame;
  a `--superfx-trace-from <frame>` would pair with it the way
  `--dma-trace-from` does.
- `luna state --peek 70:0000:8` did not return JSON on 2026-09-24
  (parse error on our side); we have not chased whether the bank:addr
  form is accepted for cart RAM or only for symbols. We will report the
  exact output with the 1.26.0 bump.
