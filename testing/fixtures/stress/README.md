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
| `rt32_nmi/` | the 32-bit runtime against interrupts: 32-bit multiplies, divides and remainders in the main loop, interrupted by an `nmiSetLag` callback that does 32-bit arithmetic too and by a per-scanline H-timer IRQ; every result against the compiler's folded constant, 0 wrong in 10 000 iterations |
| `wait_race/` | `WaitForVBlank` against an NMI that lands between its store of `vblank_flag` and its `wai`: a sweep of the call over every phase of the frame, each call classified by the order of its work frame and its lag frame; 0 lost wake-ups |
| `race_sweep/` | the subject of the interrupt gate (`testing/race_gate.py`): `probe()` does the 32-bit multiply, divide and remainder once, the NMI lag callback does others; `luna race-sweep` forces a NMI before each instruction of the helpers and compares the stored results with the run that was not interrupted (353 addresses, 0 racy; 135/141 and 193/206 racy on the tree before `76adcdb0`) |
| `timing_probe/` | a ROM whose screen is its own timing: the NMI callback reads the scanline it runs on (OPVCT) and paints the backdrop with a colour computed from it; luna reads 226 (the NMI is taken on 225, the handler's prologue comes first). In the corpus of the second-emulator comparison, where a NMI one line late changes the colour of the whole screen and a stabilised game screen shows nothing |
| `koff_hold/` | the audio driver's key-off against the S-DSP, which reads KOFF once every two samples (64 SPC700 cycles): 1024 `audioStopVoices()` of pseudo-random masks written at every phase of that period, then every voice's envelope read back; 0 voices of the mask left playing, 0 outside it silenced. It shows it can fail: built against a library whose hold is too short, `make -C lib AUDIO_KOFF_HOLD_TURNS=8` (54 cycles) then a clean build of the fixture, `luna test` reports `values.r_not_cut: 0x28D violates eq 0x0` (653 voices left playing of 4181); 9 turns (60 cycles) leave 326, 10 turns (66) none, the default 21 (132) none. Rebuild the library without the variable afterwards. Also the cost of a stop in scanlines (`r_lines_*`) |
| `sprite_overflow/` | 40 small sprites on one scanline: STAT77 (`$213E`) range-over sets, time-over stays clear because range caps the evaluation at 32 sprites |

The expected values were established against the references (fullsnes,
anomie) and, once, against a second emulator when the campaign started in
2026-08; luna is the only backend since. One open observation is not
asserted: `$213F` STAT78 reports PPU2 version 2 on luna, a chip-revision
modelling choice (real consoles ship 1/2/3) that belongs to the owner.

Build artifacts (`*.sfc`, `*.sym`, `*.o`, `build/`) are gitignored; only
the sources, the `Makefile` and the `.toml` are tracked. `make test-manifests`
rebuilds the ROMs on demand.
