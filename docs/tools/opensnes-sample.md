# opensnes-sample — WAV to BRR samples {#tools_opensnes_sample}

The first tool of the 1.x family (@ref tools_conventions). It turns a PCM
`.wav` — a jump, a hit, a UI blip, a voice clip — into the `.brr` the SNES
DSP plays, plus a header with its sizes, and it tells you what a sample
costs before you commit to it. The encoder is the one `smconv` bakes into
soundbanks and the one `wav2brr` used: a `.brr` from here is byte for byte
what `wav2brr` wrote.

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
flags set) and `jump.h`:

```c
#define jump_brr_size 1035   /* bytes: 115 BRR blocks of 16 samples */
#define jump_brr_loop 0      /* byte offset of the loop point; 0 with no loop */
#define jump_brr_loops 0     /* 1 = the sample loops */
#define jump_rate 32000      /* Hz of the source; the DSP plays at 32000 */
```

Bake the `.brr` into the ROM and load it with the sizes from the header:

```asm
ASSET_SECTION "samples"
jump_brr:  .incbin "res/jump.brr"
.ENDS
```

```c
#include "res/jump.h"
extern u8 jump_brr[];
audioLoadSample(0, jump_brr, jump_brr_size, jump_brr_loop);
audioPlaySample(0);
```

`examples/audio/soundboard` is the worked example, @ref tutorial_audio the
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
opensnes-sample inspect res/*.brr --json   # one JSON object: bytes, blocks, loop, ARAM
```

Nothing is converted or written; this is how you size a sound set against
the 64 KB of audio RAM before recording more.

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
