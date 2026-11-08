# Sample swap

Samples at **addresses the game chooses**: a resident set, and a swap area
whose samples are replaced during play. `audioLoadSampleAt()` and
`audioLoadSampleBeginAt()` place a sample, whole or in steps, where the
game says, refuse a place that is not free, and leave `audioLoadSample()`
working beside them. There is nothing to press: the phases run on their
own and the backdrop colour names the phase (blue placing, red refusals,
green replacement, yellow silence cases, purple timing, grey done).

## What it shows

1. **Placing**: two samples at $2000 and $3000; an ordinary
   `audioLoadSample()` after them goes above them ($3048); one placed just
   below the first (touching it) leaves the allocator where it was, and
   unloading it gives nothing back.
2. **Refusals**, each with its code and nothing changed: slot 64, below
   $0B00, at the end of the area, past $FFFF, one byte on another sample,
   size 0, size under 3 in steps, on the load in steps, the slot being loaded,
   a second load in steps.
3. **Replacing during play**: voice 0 holds the sample at $2000 while the one
   at $3000 is replaced in three steps of 24 bytes, then played.
4. **Silence before overwriting**: the voice playing the replaced sample is
   left on, stopped just before the first step, stopped late in a frame,
   stopped one or two frames before. The ROM times each gap and the
   release.
5. **A note on another voice** during a load in steps: twelve notes, one a
   frame, alone, after a 192-byte step and before one.

The samples are BRR written by hand in `data.asm`, block by block, so the
manifests can compare their bytes in the sound CPU's memory.
`testing/manifests/audio_sample_swap.toml` checks the places, the refusals,
the bytes at each boundary and the notes' timing;
`audio_sample_swap_silence.toml` checks the gaps against the release.
Measured: the release takes 133 scanlines from `audioStopVoice()` (8 ms);
a key-off just before the first step is not enough, two VBlanks are (see
the tutorial).

## SNES Concepts

- The sample directory: two 16-bit addresses per sample, on any byte
- A swap area of fixed places in the sound CPU's memory, filled while voices play
- Key-off is a release, not a cut: the voice reads its sample until the envelope is down
- BRR blocks: a header and sixteen 4-bit samples in 9 bytes

## How to Build

```bash
cd examples/audio/sample_swap && make
```

## Modules Used

`console`, `dma`, `audio`, `profile`

See also `docs/tutorials/audio_samples.md` ("A swap area").
