# Chantier — SNESMOD, the 65816 side: what `snesmod.asm` does to the rest of the machine

Status: points 1, 2, 3, 4, 5, 6 FIXED on 2026-10-03 (same day), Class B
(`lib/source/snesmod.asm`); NMITIMEN fixed the same day. One item left open at the end of this note, plus the owner's call on upstream.
Opened by snes-rag's reading of the upstream driver
(`partners/snes-rag/2026-10-03_from_snes-rag_snesmod-api.md`); each point was
then checked in our copy, which descends from PVSnesLib's port.

## Findings in our copy

| # | What | State | Evidence |
|---|---|---|---|
| 1 | `snesmodProcess` waits for the SPC by reading `REG_SLHV` then `REG_OPVCT` **once** per turn. OPVCT is a read-twice register: every other read returns the high byte (bit 0 = line bit 8, the rest PPU2 open bus). | **measured** | below |
| 2 | The same single read leaves OPVCT's read pointer mid-sequence for whoever reads it next (`profileGetScanline`, the Super Scope code in `crt0.asm`, a user's H-IRQ handler). `console.c` already documents this trap for its own seed read and resets with STAT78. | read; follows from 1 | `snesmod.asm:750-751`, `:821-822`; `console.c:72-78`; `profile.asm:139-147` |
| 3 | Reading `$2137` sets the latch flag (STAT78 bit 6; fullsnes `5f1c3420ba3a986c`: "in all three cases the latch flag in 213Fh.Bit6 is set"). `crt0.asm` treats that flag as "the Super Scope fired" (`:1894-1899`) and reads a position. A game with a Super Scope and SNESMOD would get shots at the beam position of `snesmodProcess`. | **read, not measured** — no example links both | to reproduce before fixing |
| 4 | `QueueMessage` ends with `cli`, then `plb` / `plp`. The caller's I flag is restored by the `plp`, so unlike upstream the caller is not left unmasked, but an IRQ the caller had masked can be taken in the two-instruction window. | read | `snesmod.asm:639-659` |
| 5 | The FIFO is 256 bytes, 3-byte messages, 8-bit indexes, no overflow check: the 86th message queued before a `snesmodProcess` overwrites the first. | read | `snesmod.asm:150-152`, `:643-652` |
| 6 | `snesmodGetPosition` reads `REG_APUIO3` once. Upstream reads its port five times until stable (but the wrong port, APUIO2); ours reads the right port with no stability check, so a read while the SPC writes the port can return a torn value. | read | `snesmod.asm:1019-1030`; upstream `snesmod_dev.asm:627-635` |

Points 1, 2, 3 only run when the queue still holds a message after one was
sent, or the SPC has not acknowledged yet: with one message per frame the
wait loop is never entered (measured: `snesmod_music`, stop at frame 60, no
`@next` executed).

## Measurement of point 1 (luna v1.30.4, 2026-10-03)

`examples/audio/snesmod_sfx`, A + B + X + Y pressed on frame 120 (four
effects queued), CPU trace, A after the `lda REG_OPVCT` at `$00:A5AC`:

```
luna state snesmod_sfx.sfc --until-frame 124 --input "120:0xC0C0,122:0" \
     --cpu-trace cpu.csv --cpu-trace-max 400000
frame 122, 13 reads, value/Y:  E6/5 E6/5 E6/5 E6/5 E6/5 E6/5 E6/5 E6/5
                               E7/5 E6/4 E7/3 E6/2 E8/1
```

While the line is even the low byte and the "high byte" (open bus = the low
byte with bit 0 cleared) are equal, so nothing is counted. As soon as the
line is odd every turn sees a change and `PROCESS_TIME` (5, "process for 5
scanlines") runs out in four turns: about two lines instead of five.

## What was done (2026-10-03)

- **1, 2, 3**: the wait counts rising edges of the H-blank flag (`$4212`
  bit 6: set at H=274, cleared at H=1, on every line, V-blank and forced
  blank included; anomie-timing `08a81c8c93552908`, fullsnes
  `ec4585ecbad65257`). No `$2137`, no OPVCT. A waiting turn is about 170
  master cycles against a 268-cycle flag, so no edge is missed; a turn that
  sends a message is longer and may hide one (the budget then runs slightly
  long, never short). Both copies of the loop (`spcProcessMessages`,
  `xspcProcessMessages`).
- **4**: the `cli` is gone from `QueueMessage` and from `snesmodInit`; the
  `plp` restores the caller's flag.
- **5**: `QueueMessage` drops a message when 253 bytes or more are in use.
  The measurement that decided it: 100 sends with no process left a depth of
  44 and the driver never answered again (the indexes were out of step, it
  received the middle of a message as a command).
- **6**: `snesmodGetPosition` reads until two reads agree.

Fixture `devtools/libtests_snesmod` + `manifests/libtest_snesmod.toml`:

| | before | after |
|---|---|---|
| STAT78 bit 6 after a waiting `snesmodProcess` | `$40` | 0 |
| lines spent waiting | 3 | 6 |
| frames to send 4 queued messages | 4 | 2 |
| queue depth after 100 sends | 44 (wrapped) | 255 |
| the full queue drains | never (1484 frames and counting) | 40 frames |

Audio hashes of `snesmod_music` and `snesmod_music_large` moved: commands
reach the driver a fraction of a millisecond apart from before. Old and new
captures (300 frames) compared: energy per half second within 1 %
(5448/5448, 4426/4427, 1218/1210, 227/225), onset +0 and +8 samples, no
offset makes them bit-identical (the driver's tick phase differs). fbhash
89/89 unchanged.

## Still open

- **Super Scope + SNESMOD** was never reproduced as a false shot: no example
  links both. The cause is removed (no latch any more); a probe would only
  document the past.
- ~~`snesmodInit` writes `$81` to NMITIMEN~~ fixed 2026-10-03: it restores
  the lib's `nmitimen_shadow`. Fixture: a V-timer IRQ armed before
  `snesmodInit()` fired 0 times in the 10 frames after it, 10 now
  (`r_irq` in `libtest_snesmod.toml`).
- Upstream has all of these; whether they go to Mukunda as issues is the
  owner's call (`partners/snesmod-upstream/README.md`).

## Fix, as first sketched (kept for the record)


- 1 and 2: read OPVCT twice per turn (`lda REG_OPVCT` then `bit REG_OPVCT`),
  which keeps the pointer where it was found. That restores the real
  five-line budget, so `snesmodProcess` may then spend up to five scanlines
  when several messages are queued: measure it (`nmi_budget.py`, the audio
  hashes) before deciding to keep 5.
- 3: the wait should not use the software latch at all, or the latch flag
  must be cleared in a way that cannot eat a real shot. Needs a Super Scope +
  SNESMOD probe first.
- 4: drop the `cli`; the `plp` restores the caller's flag.
- 5: decide between a documented limit and a checked queue (the cost is on
  every message).
- 6: read until two reads agree.

Upstream has 1, 2, 3, 5 as written, 4 worse (no `plp`: the caller is left
unmasked) and 6 differently (stable read of the wrong port). Whether these go
to Mukunda as issues is the owner's call; five are already open there
(`partners/snesmod-upstream/README.md`).
