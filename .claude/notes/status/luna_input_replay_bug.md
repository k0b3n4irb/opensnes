# luna v1.9.0 — `state --input` misapplies checkpoint frames

Status: found 2026-07-18 during the apu_switch example work. Filed
upstream as k0b3n4irb/luna#126. Affects
`luna state --input` (the probes' input path). `luna run` has no
`--input`, so coverage/visual baselines are NOT affected.

## Symptoms (both reproduced deterministically on apu_switch.sfc)

1. **First checkpoint's mask is latched at boot**, not at its frame.
   `--input "900:0x8000,903:0"` (B at frame 900 = 15 s) flips the
   example's `current_song` WRAM var by ~0.8 s — the earliest frame
   the main loop polls the pad:

   ```
   luna state -n 5000000 --input "900:0x8000,903:0" --out /dev/null \
       --peek current_song:1 examples/audio/apu_switch/apu_switch.sfc
   # -> current_song = 01   (no press should have happened yet)
   ```

   Control: the same command WITHOUT `--input` leaves it at 00.

2. **The checkpoint list re-fires periodically.** With a 4-checkpoint
   script (`"0:0,300:0x8000,303:0,660:0x80,663:0"`), the ROM receives
   alternating phantom B/A presses every ~1–2 s for the whole run —
   audible as endless drum/cello re-switches in `--audio-out`, and
   visible as the DSP flip-flopping between the two programs' register
   sets in `spc-dump` at different `-n`. A 2-checkpoint B-only script
   converges (extra B presses are no-ops for the ROM) which masked the
   bug until a two-button script was used.

Identical invocations are bit-identical (`--audio-out` WAVs compare
equal), so this is a deterministic replay-timing bug, not racy state.

## Harness impact

- `probes/*.py` asserts are directional / held-value, so they stay
  green — but any future probe asserting *when* or *how many times* an
  input lands will be wrong until this is fixed.
- `probes/apu_switch.py` deliberately asserts only convergent states
  and documents this bug in its docstring; its cello→drums (A after B)
  assert is omitted and should be added back once fixed.

## Where to look (luna side)

The frame-latch comparison in the scripted-input replay presumably
compares against a frame counter that (a) starts counting later than
the checkpoint parser assumes and (b) wraps/resets, re-arming the
checkpoint list. `state`'s path is affected; `spc-dump --input` uses
the same script format and shows the same behavior.
