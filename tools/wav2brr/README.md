# wav2brr — WAV → SNES BRR sample converter

Turns a PCM `.wav` (a jump, a hit, a UI blip, a voice clip) into a `.brr`
sample you load at runtime with `audioLoadSample()`. `smconv` covers music
(Impulse Tracker modules); `wav2brr` fills the one-shot / sound-effect gap.

## Usage

```sh
wav2brr [options] input.wav [output.brr]

  --loop START END   loop between sample indices START..END (default: one-shot)
  -v, --verbose      print the input/output breakdown + a ready-to-paste load line
  -h, --help         show help
```

With no output path, `input.wav` is written alongside as `input.brr`.

- **Input:** PCM WAV, 8- or 16-bit, mono or stereo (stereo is downmixed).
- **Rate:** keep the source at ≤ 32 kHz — the DSP's ceiling. A higher rate is
  accepted but plays back sharp (`-v` warns).
- **Output:** raw BRR, size always a multiple of 9 bytes (one 9-byte block per
  16 samples), with the loop/end flags set correctly.

## Using the result

Bake the `.brr` into the ROM with `.incbin`, then load and trigger it:

```asm
.section ".samples" superfree
brr_jump:     .incbin "res/jump.brr"
brr_jump_end:
.ends
```

```c
extern u8 brr_jump[], brr_jump_end[];
audioLoadSample(0, brr_jump, (u16)(brr_jump_end - brr_jump), 0); // 0 = no loop
audioPlaySample(0);
```

`examples/audio/soundboard` is a complete worked example, and
`docs/tutorials/audio.md` walks the whole flow.

## How it's built

`wav2brr` reuses smconv's BRR encoder (`../smconv/src/brr.c`) directly, so a
`.brr` from here is byte-for-byte the same format as one baked into a soundbank.
The tool itself only parses the WAV and hands PCM to that encoder. The encoder
is (C) 2009 Mukunda Johnson (smconv); see `../smconv/README.md`.

## Tests

`python3 tools/wav2brr/tests/run_golden.py` (also run by `make test-tools`)
encodes a committed fixture WAV in one-shot and looping modes and byte-compares
against goldens. The encoder is deterministic, so any diff is a real change.
