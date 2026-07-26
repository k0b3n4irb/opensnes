# soundboard — the audio v2 engine, driven entirely from C


The first audio example with **zero SPC700 assembly**. The lib's
`audio` module ships its own resident SPC700 driver (built from
source at lib build time) and exposes the whole engine as plain C:
this example streams four BRR samples into APU RAM at runtime,
plays them polyphonically with per-play volume/pan/pitch, shapes
every voice's envelope, and switches a concert-hall echo on and off
— all through `snes/audio.h` calls.

| Input | Action |
|---|---|
| A | cello note, center |
| B | "AU" vowel, left |
| X | "SS" hiss, right |
| Y | "PP" pop, center |
| L / R | cello a fifth down / up (pitch demo) |
| Up / Down | master volume |
| START | toggle the echo hall |

The backdrop tints with each action. Up to 8 simultaneous voices,
round-robin allocated by the engine.

ROM mode: LoROM (project default).

## SNES Concepts

- Dynamic BRR loading: `audioLoadSample()` streams sample data over
  the APU I/O ports — no fixed APU memory image, no `.spc700.asm`
- `audioPlaySampleEx()`: volume/pan/pitch per play; the DSP's 8
  hardware voices ARE the polyphony
- ADSR as note articulation: one envelope per voice set at init —
  looping samples ring out and die instead of sustaining forever
- Echo: `audioSetEcho()` sizes and clears the ring, `audioSetEchoFilter()`
  sets the FIR, `audioEnableEcho()` routes voices in

## Sample provenance

`res/cello.brr` is krom (Peter Lemon)'s PitchMod cello (see
`ATTRIBUTION.md` for the caveat). The `ss`/`pp` phonemes are
OpenSNES-original formant-synthesized BRRs from the speech_synth
example. `au.brr` is an RMS-boosted regeneration of speech_synth's AU
(`gen_au_boost.py` — the speech bank is peak-normalized, and a vowel's
high crest factor made the original ~3x quieter than the recorded
cello next to it).

## How to Build

```bash
make
```

## Modules Used

console, audio, input
