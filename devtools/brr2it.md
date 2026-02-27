# brr2it.py — BRR to Impulse Tracker Converter

Converts SNES BRR (Bit Rate Reduction) audio samples to Impulse Tracker (.it)
files, which can then be processed by `smconv` to generate SNESMOD soundbanks.

## When to use

- **Porting PVSnesLib projects** that include raw `.brr` sound effects
- Converting existing BRR assets for use with OpenSNES's `snesmodPlayEffect()` API

For **new content**, prefer creating IT files directly with a tracker
(e.g., [OpenMPT](https://openmpt.org/)) or wait for WAV support in smconv.

## Dependencies

None (Python stdlib only: `struct`, `sys`).

## Usage

```bash
uv run brr2it.py <input.brr> <output.it> [sample_name] [sample_rate]
```

### Arguments

| Argument | Required | Default | Description |
|----------|----------|---------|-------------|
| `input.brr` | Yes | — | Source BRR file |
| `output.it` | Yes | — | Output Impulse Tracker file |
| `sample_name` | No | `"Sample"` | Name embedded in the IT file (max 25 chars) |
| `sample_rate` | No | `16000` | C5Speed in Hz (see Pitch Tuning below) |

### Example

```bash
# Convert a PVSnesLib jump sound effect
uv run brr2it.py mariojump.brr jump_sfx.it "Jump" 8363
```

## Pitch Tuning

The `sample_rate` parameter sets the IT file's **C5Speed**, which determines
the base pitch in the SNESMOD soundbank.

- **8363 Hz** = neutral (PitchBase = 0). Recommended starting point.
- Higher values = higher base pitch, lower values = lower base pitch.

The final playback pitch is controlled by the `pitch` parameter in
`snesmodPlayEffect()`. With C5Speed=8363 (neutral), the pitch scale is:

| Pitch value | Approximate rate |
|-------------|-----------------|
| 1 | ~4 kHz |
| 2 | ~8 kHz |
| 3 | ~12 kHz |
| 4 | ~16 kHz |
| 8 | ~32 kHz |

PVSnesLib's LikeMario plays `mariojump.brr` at ~12 kHz, so use
C5Speed=8363 with pitch=3 to match.

## BRR Format

BRR encodes audio in 9-byte blocks (1 header + 8 data bytes = 16 PCM samples):

```
Header byte: RRRR FF EL
  R = shift/range (0-12)
  F = filter (0-3, prediction coefficients)
  E = end flag
  L = loop flag
```

The decoder uses filter coefficients matching the bsnes/higan DSP implementation.

## Limitations

- The BRR is decoded to PCM then re-encoded to BRR by smconv (double conversion).
  Quality loss is minimal for short SFX but may be noticeable for complex samples.
- Loop points from the BRR are not preserved in the IT file.
