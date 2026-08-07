# wav2brr — WAV to BRR sound samples {#tools_wav2brr}

The SNES sound chip plays samples in its own compressed format, **BRR** (Bit
Rate Reduction — a 9-byte block per 16 samples). `wav2brr` encodes an ordinary
PCM `.wav` into a `.brr` you load at runtime with `audioLoadSample()`. It is the
zero-config path for one-shot sound effects.

## What goes in, what comes out

**In:** a PCM `.wav` — 8- or 16-bit, mono or stereo (stereo is downmixed).

**Out:** a raw `.brr` (its size is always a multiple of 9 bytes). Default name is
`input.brr` alongside the input.

## The flags you will actually use

| Flag | Meaning |
|------|---------|
| `--loop START END` | loop between sample indices START..END (default: one-shot, no loop) |
| `-v` | verbose — prints the size breakdown, a sample-rate warning, and a ready-to-paste load line |

## How you actually use it

**One-shot SFX — automatic.** Drop a `.wav` in the example's `res/` and
`.incbin` the matching `.brr` in an ASM file; the build's `%.brr: %.wav` rule
runs `wav2brr` for you. No command to type:

```
res/blip.wav   ──(make)──►   res/blip.brr   ──.incbin──►   audioLoadSample()
```

**Looping samples — by hand.** A sustained instrument or a looping ambience needs
loop points, which only you know, so build it once and commit the `.brr`:

```sh
wav2brr --loop 512 2048 instrument.wav instrument.brr
```

Run with `-v` to see the block count and the exact `audioLoadSample()` call to
paste.

## Gotchas worth knowing up front

- **Keep the source at or below 32 kHz.** That is the DSP's output ceiling; a
  higher-rate WAV is accepted but plays sharp (a note higher than authored). `-v`
  warns when it happens. The pitch you hear is set at *playback* by the DSP pitch
  value, relative to the sample's rate — see @ref snes_sound_guide.
- **Looping bypasses the auto-rule.** The zero-config path makes one-shots only;
  anything with `--loop` is a hand-built, committed asset.
- **Deterministic and shared.** wav2brr uses the same BRR encoder as
  @ref tools_smconv, so a sample sounds identical whether it is a standalone SFX
  or baked into a soundbank (golden-tested via `make test-tools`).

## See it in practice

- @ref examples_audio_sfx_from_wav — one-shots auto-generated from `res/*.wav`.
- @ref examples_audio_soundboard — committed `.brr` blobs, `.incbin`'d and played
  with per-voice volume/pan/pitch.

For music (as opposed to individual samples), you want @ref tools_smconv and its
tracker-module workflow instead.
