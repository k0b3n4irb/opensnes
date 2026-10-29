# opensnes-sample — WAV to BRR samples {#tools_opensnes_sample}

The first tool of the `opensnes-*` family (@ref tools_conventions). It turns a PCM
`.wav` — a jump, a hit, a UI blip, a voice clip — into the `.brr` the SNES
DSP plays, plus a header with its sizes, and it tells you what a sample
costs before you commit to it. The encoder is the one `opensnes-music`
bakes into soundbanks.

## Encode

```sh
opensnes-sample encode res/jump.wav                 # → res/jump.brr, res/jump.h
opensnes-sample encode res/hum.wav --loop 40 200    # loop between two sample indices
opensnes-sample encode res/*.wav --out build/       # several at once, elsewhere
opensnes-sample encode res/hum.wav --loop 40 200 --save   # remember the loop beside the file
```

**In:** PCM WAV, 8- or 16-bit, mono or stereo (stereo is downmixed). Keep
the source at 32 kHz or below: the DSP plays BRR at 32 kHz whatever the
file said, so a 44.1 kHz source comes out lower and slower (the tool warns
and says by how much).

**Out:** `jump.brr` (raw BRR, one 9-byte block per 16 samples, loop and end
flags set), `jump_data.as` (the fragment the build assembles: the `.brr` in
an `ASSET_SECTION`, between `jump_brr` and `jump_brr_end`) and `jump.h`:

```c
extern u8 jump_brr[], jump_brr_end[];
#define jump_brr_size 1035   /* bytes: 115 BRR blocks of 16 samples */
#define jump_brr_loop 0      /* byte offset of the loop point; 0 with no loop */
#define jump_brr_loops 0     /* 1 = the sample loops */
#define jump_tuning 4096     /* x4096: 4096 = plays at the pitch it was encoded at */
#define jump_rate 32000      /* Hz of the source; the DSP plays at 32000 */
```

**A loop has to be a whole number of 16-sample blocks**, because BRR is.
When the one you give is not, the tool makes it so, in one of two ways:

- **it repeats the loop** until the total is a multiple of 16, when that
  stays under 2,000 samples: a waveform of 8 samples becomes one block
  holding it twice, one of 12 samples becomes three blocks holding it four
  times. Nothing changes in pitch, and `_tuning` is 4096;
- **it resamples the loop** to the next multiple of 16 otherwise. The
  sample then plays slightly lower, the tool warns, and `_tuning` (and
  `"tuning_x4096"` in `--json`) carries the factor: multiply the playback
  pitch by `_tuning / 4096` to get the original pitch back.

A loop that does not start on a block boundary is preceded by silence so
that it does. `_brr_loop` is the byte offset the loop comes back to; 0
means "from the start", not "no loop": `_brr_loops` says whether it loops.

**In a project**, put the settings beside the sound — `res/jump.wav.toml`
with `tool = "opensnes-sample"` and an `[encode]` table, which `--save`
writes for you — and the build converts it and links `jump_data.as`: there
is no assembly to write. Load the sample with the sizes from the header:

```c
#include "res/jump.h"
audioLoadSample(0, jump_brr, jump_brr_size, jump_brr_loop);
audioPlaySample(0);
```

Without a settings file, `.incbin` the `.brr` yourself in an
`ASSET_SECTION` under the label `jump_brr`.

`examples/audio/sfx_from_wav` is the worked example, @ref tutorial_audio_samples the
tutorial.

## Settings beside the asset

`--save` writes `res/hum.wav.toml` next to the file:

```toml
tool = "opensnes-sample"

[encode]
loop = [40, 200]
```

From then on `opensnes-sample encode res/hum.wav` loops without being told,
and a `--loop` on the command line overrides it for that run. The file is
the one the build system reads (the `tool` line says which tool converts
the asset); a key it does not know is refused, so a typo cannot fall back
to a default in silence.

## Inspect

```sh
opensnes-sample inspect res/jump.wav       # 32000 Hz, 16-bit, mono, 1840 samples (0.058 s) -> 115 BRR blocks, 1035 bytes of ARAM
opensnes-sample inspect res/hum.wav --loop 40 200   # what encode --loop 40 200 would write
opensnes-sample inspect res/*.brr --json   # one JSON object: bytes, blocks, loop, ARAM
```

Nothing is converted or written; this is how you size a sound set against
the 64 KB of audio RAM before recording more. A WAV is sized as `encode`
writes it: with a loop (from `--loop`, or from the `.wav.toml` beside it)
what follows the loop's end is left out, and a loop that is not a whole
number of 16-sample blocks is repeated or resampled, so the figure is the
size of the `.brr`, not of the file. The JSON of a WAV gives `loops`,
`loop_offset` and `tuning_x4096` next to `aram_bytes`, as `encode --json` does.

## Exit codes and messages

0 done; 1 the input was refused and the message names the limit (`--loop
100 50 is out of range — the file has 220 samples, START < END`); 2 usage;
3 the file system. A refused input writes nothing. `--json` puts the
result — sources, outputs, blocks, ARAM bytes, loop — on stdout as one
object; `-q` drops the summary line; `-v` says what is being done.

## From wav2brr

| wav2brr | opensnes-sample |
|---|---|
| `wav2brr in.wav [out.brr]` | `opensnes-sample encode in.wav [--out DIR]` (the name follows the input) |
| `--loop START END` | the same, and `--save` keeps it in `in.wav.toml` |
| `-v` prints the load line | `in.h` carries the sizes; `inspect` the figures |

Same bytes: the golden suite of `opensnes-sample` compares its `.brr`
against `wav2brr`'s goldens.
