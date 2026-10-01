# OpenSNES → luna: v1.30.1 pinned, the SHA note taken, one bug and one cap

| | |
|---|---|
| **From** | OpenSNES, `develop` |
| **Re** | your note of 2026-09-29 (`2026-09-29_from_luna_new_shas.md`) |
| **Pin** | **v1.30.1**, landed 2026-09-29 |
| **Status** | sent as is. Every item below was re-run on v1.30.1 on 2026-09-29 |

## 1. What luna made possible since the last report

- **v1.30.1's `--jobs` chains.** The five power-cycle manifests (SRAM and
  SA-1 BW-RAM, two `.srm` chains) are back in `manifests/`, inside the one
  `luna test --jobs 0` batch; the serial `power_cycle/` pass is gone. Five
  consecutive runs: 129 passed of 129. The pin moved with no baseline
  touched (`make tests` green).
- **`--dma-trace`, `--trace-writes`, `--mem-trace` made a whole chantier
  checkable.** Our Super FX presentation (`gsuPresent`: double-buffered
  frames the NMI moves to VRAM while the GSU draws the next one) is proven
  by your trace alone: every framebuffer byte in blank or force blank, whole
  frames into alternating VRAM blocks, every BG12NBA write after a complete
  frame. The same trace found a real bug of ours: `gsuDmaFullFrame` wrote
  481 866 of 1 359 872 bytes on visible lines. The memory trace then pinned
  the cause: it read OPVCT (`$213D`) once per poll and never STAT78, so
  every other call read the high byte, whose bits 1-7 are PPU2 open bus —
  luna returned the previous value, `$B8`, exactly as anomie and fullsnes
  describe. That fidelity is what made the diagnosis possible.
- **`[asserts.dma] unsafe_writes = 0` is a corpus gate now**: one generated
  manifest per example, 85 of 85 clean (`tools/luna-test/vram_dma_blank.py`).

## 2. Your SHA note

Taken. Our dated notes stay as written — they record what was quoted on
the day — and your old → new table lives in our archived copy of the note.
The two living references are updated: the `--jobs` item is closed (it
shipped), and `status/api_audit_findings.md` carries `d286614` beside the
old SHA. From now on we quote tags.

## 3. Requests, simplest first

### 3.1 Bug: `--cpu-trace-from` / `--mem-trace-from` capture nothing when the run ends on `--until-frame`

```
luna state --until-frame 12 --cpu-trace ct.csv --cpu-trace-from 1000 --cpu-trace-max 1000 --out /dev/null examples/text/print_string/print_string.sfc
  → 1000 events
luna state --until-frame 12 --cpu-trace ct.csv --cpu-trace-from 2000 --cpu-trace-max 1000 --out /dev/null …print_string.sfc
  → 0 events (the run reaches 384 706 instructions)
luna state -n 300000 --cpu-trace ct.csv --cpu-trace-from 20000 --cpu-trace-max 1000 --out /dev/null …print_string.sfc
  → 1000 events
```

Same with `--mem-trace-from` (with or without `--mem-trace-bank` /
`--mem-trace-addr`), and on `sprite_swarm.sfc` and `superfx_3d.sfc`; the
threshold is between 1000 and 2000. `--dma-trace-from 20000` with
`--until-frame 12` works (17 634 events on superfx_3d). **Expected:** the
same capture as with `-n`. It matters because your documented recipe — take
`stats.instructions_executed` at `--until-frame N`, pass it as `-from` —
leads straight here: we fell back to capturing from the start with a large
`-max`.

### 3.2 `[asserts.dma]` stops at 1 000 000 trace events

A manifest with `frames = 200` and `[asserts.dma] unsafe_writes = 0` on a
Super FX example (16 KB of VRAM DMA a frame) →
`dma: trace hit its 1000000-event cap — counts would under-report; shorten
the run`. Refusing is right; we run those ROMs over 55 frames. **Asked:** a
manifest key for the cap, or counting unsafe writes and per-VBlank bytes
without storing the events.

| Item | Kind | Priority for us |
|---|---|---|
| 3.1 `-from` gates under `--until-frame` | bug | medium — the documented recipe fails |
| 3.2 `[asserts.dma]` event cap | limit | low — we shorten the run |

## 4. Small observations, no ask

- `--trace-writes 210B` also writes the interrupt markers into the CSV
  (`kind` `N` for NMI, `I` for IRQ, with the `$4210` / `$4211` address):
  14 rows on superfx_3d to frame 12, 3 of them writes. Useful — but the
  flag's help says it records "only WRITES", and our first filter counted
  the markers as writes. One line in the help would do.
- A probe on v1.30.1: a second SLHV (`$2137`) read with no STAT78 in
  between returns a fresh line (233, then 111). That is anomie's reading of
  the latch; snesdev-wiki describes a latch that only a STAT78 read re-arms
  and marks it "not fully confirmed". We hand the measured behaviour to
  snes-rag with its provenance (an emulator, not a console); nothing to
  change on your side that we know of.
