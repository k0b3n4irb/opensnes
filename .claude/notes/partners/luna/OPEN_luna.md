# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.34.0 (2026-10-06).

Sent on 2026-10-06 in `2026-10-06_to_luna_rapport-v1.33.1.md`; answered
the same day (`2026-10-06_from_luna_reponse-rapport-v1.33.1.md`) and closed by
v1.34.0, pinned that day: the bare static name (D1), the cartridge RAM of
`--power-on random` (D3); the archive sums (D2) are GitHub's `digest` field,
which our pin recipe now reads (`testing/luna.sha256`). The two below were
held back: not re-run, they go out with a reproduction ROM or not at all.

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| 2026-10-05 | No diagnostic when CFGR bit 5 (MS0, fast multiply) and CLSR bit 0 (21 MHz) are both set: « MS0 must be zero in 21MHz mode » (fullsnes `1adef8e33ff3c4e9`; ares `06c6d2324e3c6d01`: products « may sometimes be invalid » in that mode). Our launchers mask the bit, but a program writing CFGR itself runs green on luna and may multiply wrong on a console. Needed: a counter or a note in `state.gsu` (like `bus_violations`) when a MUL/FMULT executes with both bits set | v1.32.0 | a ROM writing `$A0` to `$3037` then `$01` to `$3039` and running `fmult`; `luna state … --out - \| jq .gsu` shows `cfgr: 160, clsr: true` and nothing else |
| 2026-10-05 | A DSP-1 LoROM of 2 MB (`ROM_BANKS=64`, `USE_DSP1=1`, before our build refused it) loads and runs on luna while the only 2 MB DSP-1 LoROM board (SHVC-2B3B-01) maps the DSP registers elsewhere (chips audit S5). Low priority: `make` refuses the combination since 2026-10-04 (`ROM_BANKS_MAX` 32 for DSP-1); an emulator warning on header-vs-board inconsistencies would still be a service | v1.32.0 | build with `ROM_BANKS=64 USE_DSP1=1` on a tree before `a8113dd8`, `luna state` boots it without a word |

- 2026-10-08 — `luna test`, a failing `[asserts.blocks]` entry: the report
  (text and `--report json`) gives the first mismatching byte only
  (`first mismatch at +0x0 (expected 46, got 44)`). To re-measure a 64-byte
  VRAM block after an intended change we ran the manifest once per byte and
  patched the hex each time (64 runs for `dma_mode2_opt_table`). Wanted: the
  actual bytes of the block in the report (`actual_hex` beside the failure
  in JSON would do), or `--update` extended to blocks as it is to
  `asserts.fbhash`. Also tried: `luna assets-dump` has `-n` (instructions)
  but no `--until-frame`, so its `vram.bin` cannot be taken at the
  manifest's frame. Kind: convenience. Cost for us today: a 20-line script.
