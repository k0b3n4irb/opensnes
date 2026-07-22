# pitch_mod — hardware vibrato via pitch modulation

![Screenshot](screenshot.png)

Port of krom (Peter Lemon)'s **PitchMod** demo. The S-DSP's `PMON`
feature multiplies a voice's pitch by the *output* of the previous
voice, every sample, in hardware. Voice 1 holds a looping cello note
at C5; voice 0 plays a 9-byte square-wave BRR block at pitch `$0003` —
far below audibility, it is a slow LFO whose tiny amplitude
(`GAIN = 2`) sets the vibrato depth. The cello enters dry; one second
in, the LFO keys on and the note starts to sing. Each note rings out
like a bow stroke (instant attack, ~7 s fade — the ADSR the sample was
shipped with) and is re-bowed every 7 s. A maximum-length FIR echo
(`EDL 15`, feedback 100) does the concert hall.

ROM mode: LoROM (project default).

Measured against the original (luna `--audio-out`, same window):
pitch 174.4 Hz vs 174.3 Hz, vibrato depth ±0.82 % on both, LFO rate
~5 Hz on both.

## Why the envelope differs from the original (a true story)

krom's demo sets a flat `GAIN 127` — which would sustain the note
forever — yet his demo audibly decays like a bow stroke. The reason:
his SPC700 program has **no final idle loop**. After the last `KON`,
execution falls off the end of the code, through the sample directory,
and starts executing the cello's BRR bytes *as opcodes* (verified with
luna's `spc-dump`: his SPC700 PC ends up at `$0548`, inside the sample
data). The decay in the original is that runaway code eventually
scribbling on the DSP registers — an accident, not a design.

This port keeps the bow-stroke *sound* but produces it on purpose: the
cello's filename in krom's repo encodes its intended envelope
(`ADSR $FF/$E8` — instant attack, sustain level 7/8, gentle rate-8
fade), so we program that ADSR, park the CPU in a clean idle loop, and
re-key the voice every 7 s for the next stroke.

## SNES Concepts

- Pitch modulation (`PMON`): voice N−1's output scales voice N's pitch
- A silent voice as a hardware LFO — `GAIN` is the vibrato depth,
  `PITCH` is the vibrato rate
- ADSR envelope as articulation — instant attack + slow sustain-rate
  fade = a bow stroke; re-`KON` = the next stroke
- Sample directory layout (`[start][loop]` pairs) with a hand-written
  single-block BRR wave next to an `.incbin` sample
- Raw APU programming via the `apu` module (IPL upload from C)

## Sample provenance

`res/cello.brr` is the cello sample shipped with krom's PitchMod demo
(loop point 4167, played at his exact C5 pitch `$08BB` = `$8BB0 >> 4`).
See `ATTRIBUTION.md` at the repo root for the provenance caveat.

## How to Build

```bash
make
```

## Modules Used

console, apu
