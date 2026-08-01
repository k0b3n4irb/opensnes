# Echo / reverb

**Family 8 — Audio · rung 8.9**

The S-DSP has a hardware **echo** (reverb) unit. This example isolates it — one
short "pop" on a timer, turned into a decaying tail by the echo — with **START
to toggle it off and on**, because you only hear reverb next to the dry sound.

*(No screenshot — the lesson is audible. Run it with sound and listen.)*

## What you'll learn

- `audioSetEcho(delay, feedback, volL, volR)` sizes the echo ring and sets how
  long the tail rings (feedback) and how loud the wet signal is. Call it
  **before** enabling.
- `audioSetEchoFilter(fir[8])` — the 8-tap FIR on the echo path; tap 0 = 127
  passes the echo straight through.
- `audioEnableEcho(voiceMask)` routes voices into the echo; `audioDisableEcho()`
  turns it off.
- Echo costs **ARAM** — the ring buffer is real memory, sized by the delay, so
  a bigger hall leaves less room for samples.

## SNES concepts

Echo is a DSP feature, not a CPU one: the S-DSP keeps a delay ring in APU RAM,
feeds a fraction of the output back into it (the feedback), and mixes the
delayed signal (the wet volume) back over the dry. `audioSetEcho` configures
all of that in one call; `audioEnableEcho` chooses which of the 8 voices are
sent in.

## How to build

```bash
make -C examples/audio/echo
```

Run `echo.sfc` in [luna](https://github.com/k0b3n4irb/luna) with audio and
press **START** to hear dry vs wet.

## Modules used

`console`, `audio`, `input`

## Ladder

The focused counterpart to [`soundboard`](../soundboard/) (which uses echo as
one of many features). It builds on the audio v2 engine — see `soundboard`
first for the sample-loading basics.
