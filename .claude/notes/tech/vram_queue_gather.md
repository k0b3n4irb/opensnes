# A gather entry for the VRAM queue: measured, declined (2026-10-25)

A second game asked for one queue entry that takes several sources to
consecutive VRAM addresses (its sprite frames are streamed tile by tile).
We said we would measure before answering.

## What was measured

A scratch ROM (not kept) flushed the same 16 tiles of 32 bytes, read from
WRAM 64 bytes apart, to 16 consecutive tile slots ($4000, step one word),
in forced blank, 50 flushes per frame, 60 frames, `luna profile` summed
over the flush routine's labels and divided by a call counter read with
`luna state --peek` at the same frame. luna v1.38.0 pin.

| Variant | mclk per flush | vs today |
|---|---|---|
| A — `vramQueueFlush` today, 16 entries | 14 821 | — |
| B — gather: VMADD and VMAIN written once, then 16 DMAs | 12 166 | −18 % |
| C — one entry of 512 bytes (the sources contiguous) | 5 611 | −62 % |
| D — today's loop detecting by itself that an entry continues the last | 20 257 | +37 % |

VRAM after the run: A, B and D byte-identical (the 16 sources end to end,
checked against the expected pattern with `--dump-vram`); a scattered
layout run through D and through today's flush, identical too.

Per entry, today: about 670 mclk besides the 256 of the bytes themselves.
A gather sub-transfer still pays the DMA set-up (size, source, bank,
MDMAEN): 503. It saves 167 mclk per tile, 5 300 on 32 entries, about a
tenth of an NTSC VBlank.

## Why no

- The cost per entry is the DMA start, not the VRAM address: a gather
  entry keeps most of it. Contiguous sources remove it (C): the lever is
  the layout of the data, which the game controls (a sprite sheet whose
  frames' tiles are stored in streaming order becomes one entry per
  frame).
- Detecting continuity in the flush (D), so that every user gets B's gain
  without a new API, costs more per entry than it saves; even written
  tighter, it pays a load, a compare and an add to save one load and two
  stores.
- A new entry kind would put a branch in every user's flush, for a gain
  only streaming of scattered tiles sees (four checks of `exchanges.md`:
  the cost to every other user is not small enough for 18 %).

What would change the answer: a measured case where the sources cannot be
made contiguous (tiles shared between frames, chosen at run time) and the
VBlank is the bottleneck.
