# luna stress ROMs

Purpose-built micro-ROMs that push luna into hard hardware corners. Each one
writes its results into a WRAM array, and a `luna test` manifest beside it
asserts those slots byte for byte (`[asserts.values]`). They run with every
`make test-manifests`, next to the corpus manifests. The running log of the
campaign is `.claude/notes/status/luna_stress_campaign.md`.

| ROM | What it pins |
|---|---|
| `hwmath/` | CPU multiply/divide (`$4202`/`$4203` → `$4216`, `$4204`-`$4206` → `$4214`/`$4216`), including divide-by-zero (quotient `$FFFF`, remainder = dividend) |
| `ppumul/` | PPU Mode 7 signed multiply (`$211B` × `$211C` → signed 24-bit at `$2134`-`$2136`) |
| `openbus/` | open-bus / MDR reads of the `$2100` mirror through banks (`lda.l bb:2100` returns the bank byte) |
| `bcd/` | 8-bit decimal-mode `ADC`/`SBC`: each case stores the result and `P` |
| `sprite_overflow/` | 40 small sprites on one scanline: STAT77 (`$213E`) range-over sets, time-over stays clear because range caps the evaluation at 32 sprites |

The expected values were established against the references (fullsnes,
anomie) and, once, against a second emulator when the campaign started in
2026-08; luna is the only backend since. One open observation is not
asserted: `$213F` STAT78 reports PPU2 version 2 on luna, a chip-revision
modelling choice (real consoles ship 1/2/3) that belongs to the owner.

Build artifacts (`*.sfc`, `*.sym`, `*.o`, `build/`) are gitignored; only
the sources, the `Makefile` and the `.toml` are tracked. `make test-manifests`
rebuilds the ROMs on demand.
