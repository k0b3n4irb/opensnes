# luna → OpenSNES: the SA-1 speed, checked against a console

| | |
|---|---|
| **From** | luna (`k0b3n4irb/luna`, v1.30.0) |
| **Re** | your reply of 2026-09-27, §1 (R5, the SA-1 counters, now in your SA-1 tutorial) |
| **Status** | the SA-1 timing model is fixed and released as `v1.30.0`; your tutorial's figure needs one check |

## In one sentence

The SA-1 model behind `luna profile` in v1.28 / v1.29 was missing three
timing mechanisms and dropped part of its time around HDMA. All four are
fixed in **v1.30.0**, and luna now matches a real console on the SA-1
Speed Test wherever the reference emulators do.

## How we found out, and what changed

After v1.28.0 we ran the *SNES-SA1 Speed Test v5.1* (VitorVilela7,
`speed_test_v51.sfc`) on luna and compared its four pages with the photos
of a real console (board 1L8B-10) published in the same repository, with
Mesen2 as second source. Values in MHz:

| Case | Console | Mesen2 | luna 1.29 | luna 1.30 |
|---|---|---|---|---|
| S-CPU in WRAM, SA-1 in ROM | 10.068 | 10.068 | 10.738 | **10.068** |
| both in ROM | 5.043 | 5.042 | 5.378 | **5.043** |
| SA-1 `RTI` loop in ROM | 15.586 | 15.588 | 16.106 | **15.586** |
| both in I-RAM | 3.722 | 3.591 | 3.590 | **3.679** |
| S-CPU HDMA from WRAM, SA-1 in ROM | 10.054 | 10.068 | 8.401 | **10.067** |
| S-CPU DMA from ROM, SA-1 in ROM | 5.075 | 5.093 | 10.457 | **5.059** |

What was fixed, all ported from ares:

- **A jump, call or return into ROM costs the SA-1 one more cycle**
  (`idleJump` / `idleBranch`). This is the one that matters most for
  ROM-resident code.
- **During an S-CPU DMA, the SA-1 sees the DMA's address** when it checks
  for a bus conflict.
- **No I-RAM conflict during the S-CPU's DRAM refresh.**
- **An invented 120-clock cap was removed.** It silently threw away most
  of the SA-1's time during HDMA.

## What it means for your tutorial

**Your current `sa1_starfield` is not affected.** The build now in your
repository runs 100 % from I-RAM, and `luna profile --from-frame 60
--until-frame 180` reads `~10.70 MHz while running` on v1.29 and v1.30
alike, as it should: I-RAM code only conflicts when the S-CPU is in I-RAM
too.

**The 8.56 MHz you quoted came from an earlier build** that ran from ROM.
On v1.30 a ROM-resident loop reads lower than v1.29 did, by how often it
jumps. If the tutorial still shows the 8.56 figure or the "~2.4x" derived
from it, re-run the command on the ROM it describes and quote that. If it
shows the I-RAM build, the figure stands.

The point about scenes also still holds, now with a second cause: the
speed depends on what the S-CPU is doing **and** on how often the SA-1's
code jumps.

## What luna still does not match

These are gaps shared with the reference emulators, so we are not chasing
them:

- **A `JMP` loop in ROM:** 8.48 MHz in luna and Mesen2, 7.67 on the
  console.
- **The SA-1 core during its own I-RAM↔BW-RAM DMA:** 0.14 MHz in luna and
  Mesen2, ~10.3 on the console.
- **An S-CPU DMA from I-RAM while the SA-1 runs there:** 3.71 in luna, the
  figure ares' model gives; 5.5 on the console, and Mesen2 is close to it.

None of these is likely in your examples. If your runtime ever DMAs from
I-RAM while the SA-1 works in I-RAM, profile it on a console too.

## Sorry

We gave you 8.56 MHz to the hundredth without having checked the model
against a console first. That check now exists, and it found four faults.
