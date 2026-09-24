# OpenSNES → luna : the Super FX asks — 2026-09-24

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop`, post-v0.44.0) |
| **luna in use** | v1.24.0 (the 1.26.0 pin bump is queued behind your tag) |
| **Status** | sent 2026-09-24. Each item was checked against `luna <cmd> --help` and a run on the day it was written. Nothing is filed as an issue; every item is what we would run tomorrow. |

## 1. Since the 2026-09-21 reply

- The v0.44.0 release ran the whole corpus on both Linux legs of
  `release.yml` on the tagged binary — the first release whose artefacts
  had executed ROMs on their own arch before shipping.
- `luna diff` at ±0 is what let 94 asset sections move out of bank $00 in
  one commit with a pixel-identical corpus — and what caught the object
  engine reading the map from a hardcoded bank $00 the first time a map
  left it: two DIFFs, then none once the bank was honoured. That bug had
  been latent since the engine was written.
- No baseline moved on our side that luna did not explain.

## 2. Why these four, now

We are sizing a multi-week chantier to make real Super FX games possible:
a CPU that keeps running (input, music, sprites) while the GSU renders
every frame, with the interrupt vectors at `$0108` / `$010C` in WRAM the
way the commercial titles do it, the frame presented by a split-frame
double buffer, completion by the GSU's IRQ on STOP. luna already runs the
GSU natively and we lean on `--superfx-trace`, `--force-mapper superfx`
and the cart-RAM dump. What follows is what only the emulator can give
that work. Simplest first.

### R1 — a `gsu` block in `luna state --out -`

**Kind:** parity with the CPU block. **Cost:** small (the state exists).

`luna state --out -` on `examples/chips/superfx_3d/superfx_3d.sfc` has no
key naming the GSU (checked 2026-09-24: no `gsu` / `superfx` / `coproc`
top-level key). We can trace it but not *assert* on it.

**Ask:**
```json
"gsu": { "present": true, "version": 4, "running": false,
         "sfr": …, "pbr": …, "r": [16 values], "cbr": …,
         "scmr": { "ron": 1, "ran": 1, "ht": …, "md": … }, "scbr": …,
         "instructions_executed": N }
```
and the same keys under `[asserts.gsu]` / `[checkpoint.gsu]` in
`luna test`, like `[asserts.dsp]`. That turns the GSU from "it renders"
into something our fixtures can pin.

### R2 — a bus-ownership diagnostic

**Kind:** new diagnostic, for the one bug class nothing else can see.
**Cost:** medium.

While SCMR RON / RAN grant the cartridge to the GSU, a 65816 read of Game
Pak ROM returns a dummy byte keyed on the address's low nibble and a read
of Game Pak RAM returns open bus. A game that forgets it — an NMI handler
still in ROM, a C function touching `$70:xxxx` mid-frame — reads garbage
silently, on hardware and in every emulator. We are about to write a
runtime whose whole point is to never do that.

**Ask:** a trace of every CPU access to ROM or cart RAM while the GSU owns
it (`seq, frame, line, pc, addr, kind`), the way `--trace-writes` reports
writes, plus a one-line count in `--out -` so a manifest can assert
`gsu_bus_violations == 0`. This is the check that lets us build the
"CPU keeps running during GSU jobs" runtime with confidence.

### R3 — GSU cycles in `luna profile`

**Kind:** parity. **Cost:** medium.

`profile` is 65816-only (no GSU flag in `--help`). For a renderer the
numbers that matter are per job (a job = GO → STOP): GSU cycles, cache hit
ratio, cycles spent in WAIT on RON / RAN, and CPU cycles spent waiting for
the GSU.

**Ask:** a `gsu` section of the profile report with those four per job, and
`--pc-set` covering GSU PCs so our coverage ratchet can count `.sfx` code
(today it cannot see it at all).

### R4 — `run_until` GSU STOP / GO, and a disassembled view (MCP)

**Kind:** debugging ergonomics. **Cost:** small once R1 exists.

Stepping a GSU job today means diffing a 200 000-line CSV.

**Ask:** MCP `run_until_gsu_stop` (and `_go`), and the `gsu` block of R1
readable from the debugger with the current instruction disassembled.

## 3. Priority, from our side

| # | Request | Why |
|---|---|---|
| R1 | `gsu` block + manifest asserts | fixtures can pin GSU state |
| R2 | bus-ownership diagnostic | the silent bug class of a real GSU game |
| R3 | GSU cycles in profile | frame budget of a renderer; coverage of `.sfx` |
| R4 | run-until STOP, disassembly | ergonomics |

## 4. Small observations (no ask)

- `--superfx-trace` with a 200 000 cap is the right default for a frame; a
  `--superfx-trace-from <frame>` would pair with it the way
  `--dma-trace-from` does.
- `luna state --peek 70:0000:8` did not return JSON for us on 2026-09-24
  (parse error on our side). We have not chased whether the `bank:addr:n`
  form is accepted for cart RAM or only for symbols; we will send the exact
  output with the 1.26.0 bump.
- Still queued behind your tag, unchanged from the 2026-09-21 reply: the
  pin bump, the mouse / Super Scope replay in our coverage ratchet, the
  stack-floor gate as its own `profile` invocation, `padIsConnected` with
  `--port2 none` and the `& 1` mask on `$4017`.
