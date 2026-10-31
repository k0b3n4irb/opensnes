# Sprites at fixed VRAM slots: measured, no module (2026-10-31)

Asked by a game; the case was restated in SDK terms and written here as
`devtools/slotbench` (nothing of the game in it). Replay: `make bench-slots`.
luna v1.40.0, master cycles per frame over 1024 frames; a frame is about
357,400, a VBlank about 49,000. Scenario `mix`, with the bench's own work
(`scenario`) taken out.

| | sprites | decide + queue | OAM | sends | total | unsent run | lost |
|---|---|---:|---:|---:|---:|---:|---|
| dynamic | 16 | (in draw) | 40,928 (with decide) | 19,547 | 60,475 | 3 | 22,016 VRAM, 76,160 OAM bytes, 185 frames |
| dynamic | 18 | (in draw) | 45,750 (with decide) | 19,611 | 65,361 | 3 | 22,272 VRAM, 77,248 OAM bytes, 187 frames, and 8 strips wrong at the end |
| fixed, C | 16 | 23,281 | 35,843 | 15,905 | 75,029 | 5 | 0 |
| fixed, C | 18 | 24,820 | 40,284 | 16,019 | 81,123 | 5 | 0 |
| fixed, asm loop | 18 | 15,331 | 40,283 | 16,020 | 71,634 | 5 | 0 |
| fixed, C, FlushUntil(258) | 18 | 25,656 | 40,281 | 21,410 | 87,347 | 7 | 0 |

Sends are `dyn_nmi` + `flush`; on the dynamic rows `dyn_nmi` also holds the
end-of-frame work. Burst scenario, all change at once: unsent run 2 (dynamic), 3 (fixed,
at 5 per VBlank and with FlushUntil), nothing lost on the fixed rows.

## What it shows

- **The decision is not the cost.** 1,379 cycles a sprite a frame in C, 852
  in assembly: a module's loop would save 9,489 cycles a frame, 2.7 % of a
  frame. The 20 lines of C in `slotsUpdate` lose nothing. So no module: it
  would carry API and tests for that. The OAM writes through `oamSet` cost
  more (2,238 a sprite), and they are not specific to slots.
- **The dynamic engine is faster on the main CPU and unsafe at its limit.**
  Its NMI hook sends up to 7 frames with no budget, before the OAM DMA.
  Writes grouped by VBlank (bytes, fixed-slot run alongside):
  7 frames in a VBlank: 140 VBlanks, all lose the row and column pushed
  behind them, 127 lose OAM bytes (71,264 bytes, about the whole 544-byte transfer each
  time; 4,896 more fall in the 0-frame row, whose sends were lost and so not counted).
  6 frames: 24 of 24 lose the row and column. 5 frames: 7 of 54. Fixed slots
  at 5 frames, nothing queued in the NMI: last write on line 255.
- **Past 16 sprites the dynamic engine is silently wrong**: `lkup32idT` and
  `lkup32idB` have 16 entries and `oamnumberspr0` has no bound, so slots 17
  and 18 read past the tables (8 of 8 strips wrong at the end of `dyn18`).
- `vramQueueFlushUntil(258)` with the NMI ending near line 232 sends about 4
  frames of 512 bytes and a row and column: it protects, it does not widen
  the VBlank. Reading the line right after it is misleading: moving the
  entries left takes time after the blank, which the first version of this
  bench counted as lateness. The trace is the oracle.

## Open

The two defects of the dynamic engine are in
`.claude/notes/status/silent_defects_log.md`. Neither is fixed by this lot.
