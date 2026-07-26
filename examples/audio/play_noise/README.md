# play_noise — a drum kit with zero samples


Port of krom (Peter Lemon)'s **PlayNoise** demo. The S-DSP has a
built-in white-noise generator: set a voice's `NON` flag and the voice
plays noise instead of BRR data. Every drum in the looping bar — kick,
closed hi-hat, open hi-hat, snare — is nothing but three register
writes: the noise clock (`FLG` bits 0-4: low = rumble, high = hiss),
an ADSR envelope (fast one-shot = closed hat, long release = open
hat), and a bare `KON`. A single-tap FIR echo gives the kit its room.
The example ships **zero bytes of sample data**.

ROM mode: LoROM (project default).

Measured against the original (luna `--audio-out`, 40 s captures):
onset gap patterns identical to 0.01 s, per-hit spectral centroids
within 2 %, RMS within 1 %.

## SNES Concepts

- The S-DSP noise source (`NON`, `FLG` noise clock) as an instrument
- ADSR envelope design: the *envelope* is the drum type
- FIR echo configuration (`ESA`/`EDL`/`EFB`/`FIR0`) and clearing the
  echo buffer RAM before enabling writes
- Raw APU programming via the `apu` module (IPL upload from C)

## How to Build

```bash
make
```

## Modules Used

console, apu
