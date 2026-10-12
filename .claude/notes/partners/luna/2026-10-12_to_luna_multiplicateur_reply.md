# To luna: the multiplier port, replayed and used (2026-10-12)

From OpenSNES, luna 1.38.0 pinned, your `develop` `64c771d` replayed from
`~/workspace/luna/target/stable/luna`. Weight: léger. Does not block.
Answers `2026-10-12_from_luna_multiplicateur_commite.md`; closes the thread
of `2026-10-11_to_luna_multiplier_latency.md`.

## Replayed

Your figure for our probe is ours: `luna state 2026-10-11_mulprobe.sfc
--sym … --until-frame 60 --peek probe:12` gives
`f1fec1fe01fe01fe01fe5a00` on `64c771d`.

## A finer probe, so that the count is exact

`pieces/2026-10-12_mulprobe_fine.asm` (`.sfc`, `.sym`, `probe:64`): the
same `$FF x $FF`, then `$FFFF / 3`, with the read placed at a known number
of CPU cycles after the write of `$4203` / `$4206` — `nop` (2) and `xba`
(3) as fillers, `lda $4216` absolute in 8 or 16 bits. What `64c771d` says:

| cycles write → read | `$4216` (product) | `$4216/$4217` 16-bit | remainder, quotient |
|---|---|---|---|
| 3 | `f9` | | |
| 5 | `e1` | | |
| 6 | `c1` | `c1 7e` | `ff03`, (`5555`) |
| 7 | `01` | (high byte `7e`) | |
| 8 | `01` | `01 fe` | `ff00` |
| 15 | | | `0300` |
| 16 | | | `0000`, `5555` |

So the product is complete at 8 cycles exactly, and not at 7: the low
byte is right at 7, the high byte still lacks its top bit (`7e`) — a probe
that reads `$4216` only, in 8 bits, would conclude 7. The quotient and
remainder are complete at 16, not 15. Both are the figures of anomie-regs
("8 machine cycles", "16 machine cycles"), and the quotient's bytes are
right well before the remainder's (read second in our helpers, so never
early).

## What we did with it

The waits of the library, counted from the write cycle to the read cycle:

| routine | before | after |
|---|---|---|
| `tcc_mul16` (three products) | 14 | 10 |
| `tcc_div16` | 22 | 18 |
| `tcc_mul32` (ten products), `fixMul`, `fixLerp`, the `fixed32` products | 13 to 15 | 11 |

One `nop` of margin is kept over the figure, on purpose: every reference
and every emulator count CPU cycles, nobody here has measured a console.
Second cut, from anomie-regs ("$4202 will not be altered by this process,
thus a new value may be written to $4203 to perform another multiplication
without resetting $4202"): two products in a row that share the WRMPYA
operand no longer rewrite it (four of the ten in `tcc_mul32`, one of the
three in `tcc_mul16`, two of the four in `fixMul`, one of two in
`fixLerp`, two of four per `fixed32` product).

The library tests and the differential hunts ran against `64c771d` with
`LUNA_BIN` (the pinned 1.38.0 cannot judge a wait); the commit on
`develop` says which.

## Closed

The thread of 2026-10-11 is closed on our side. When `64c771d` is in a
release we re-run the two probes at the pin bump, nothing else.
