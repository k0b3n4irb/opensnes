# SFX voices

Effects driven **voice by voice**, the way a game's own sound player does it,
and the cost of each step counted in scanlines. There is nothing to press: the
phases run on their own and the backdrop colour names the phase (blue loading,
green waveform sweep, yellow restarts, red seven voices, grey done).

## What it shows

1. **A looped one-cycle waveform whose pitch changes every frame**: the pitch
   you send with `audioSetVoiceVolumePitch()` is the note.
2. **Stop, then play, on the same voice inside one frame**: `audioStopVoice()`
   then `audioPlaySampleOn()`, the "last sound to arrive cuts the one in
   progress" rule.
3. **A sample sent in pieces**: `audioLoadSampleBegin()` and one
   `audioLoadSampleStep()` per frame, while the game runs.
4. **Seven voices updated every frame**, and what that costs.

The oracles (`load_lines_max`, `restart_lines_max`, `seven_lines_max`, …) are
globals; `testing/manifests/audio_sfx_voices.toml` reads them. Measured: a
192-byte step 115 scanlines at most, a stop+play pair 12, a pitch update 3,
seven voices 29 (1 scanline = 1 364 master cycles).

## SNES Concepts

- A looped one-cycle waveform: pitch is the note
- The game picks the voice; the module obeys
- Piecewise upload over the APU ports: a bounded cost per frame
- Counting a cost in scanlines from the code itself

## How to Build

```bash
cd examples/audio/sfx_voices && make
```

`res/wave.wav` is written by `gen_wave.py`; the build converts it with
`opensnes-sample` (no `.brr` is committed).

## Modules Used

`console`, `dma`, `audio`, `profile`

See also `docs/tutorials/audio_samples.md` ("Driving effects voice by voice").
