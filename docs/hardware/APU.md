# The APU {#hardware_apu}

The audio processing unit: a sound CPU (SPC700), a sound generator (S-DSP)
and their own 64 KB of RAM (ARAM), apart from the main CPU. This page is the
hardware; music is @ref tutorial_audio, samples from C and SPC700 programs of
your own are @ref tutorial_audio_samples, and the
converters are @ref tools_opensnes_sample (WAV to BRR) and
@ref tools_opensnes_music (Impulse Tracker to soundbank).

## Overview

| Component | Details |
|-----------|---------|
| **SPC700** | 8-bit CPU at 1.024 MHz |
| **ARAM** | 64 KB, shared by program, data, samples and the echo ring |
| **S-DSP** | 8 voices playing BRR samples |
| **Output** | 32 kHz stereo |

What the S-DSP does:

- 8 voices at once, each with its own volume, pitch and envelope (ADSR or GAIN)
- BRR-compressed samples (16 samples in 9 bytes, about 3.5:1)
- Echo with an 8-tap FIR filter, up to 240 ms of delay
- Pitch modulation of a voice by the previous one
- A noise generator shared by all voices

The main CPU cannot touch ARAM: it uploads a program and data through the
IPL boot ROM, then talks to that program through four ports.

## SPC700

### CPU registers

| Register | Size | Purpose |
|----------|------|---------|
| A | 8-bit | Accumulator |
| X | 8-bit | Index register |
| Y | 8-bit | Index register (pairs with A for 16-bit) |
| SP | 8-bit | Stack pointer (page 1) |
| PC | 16-bit | Program counter |
| PSW | 8-bit | Status flags |

### ARAM map

| Address | Purpose |
|---------|---------|
| $0000-$00EF | Zero page (fast access) |
| $00F0-$00FF | Hardware registers |
| $0100-$01FF | Stack |
| $0200-$FFBF | Program, data, samples, echo ring |
| $FFC0-$FFFF | IPL ROM (boot code) |

The sound driver, the samples and the echo ring share the 64 KB. The SDK's
drivers and what they leave for samples: @ref tutorial_audio and
@ref tutorial_audio_samples.

### Communication ports

| SNES side | SPC700 side | Purpose |
|-----------|-------------|---------|
| $2140 | $F4 | Port 0 |
| $2141 | $F5 | Port 1 |
| $2142 | $F6 | Port 2 |
| $2143 | $F7 | Port 3 |

Each side reads what the other wrote. Use 8-bit writes to the ports. The
official manual (Book 1, §3-9-6) warns that a 16-bit write to $2140/$2141
can also write $2143; the mechanism is disputed on nesdev, so treat it as a
caution rather than a measured fact — 8-bit writes are safe either way.

### Upload through the IPL ROM

At reset the SPC700 runs its boot ROM, which takes blocks from the main CPU
(anomie's SPC700 doc, *IPL Boot ROM*):

1. Wait until $2140 reads $AA and $2141 reads $BB.
2. Write the destination address to $2142-$2143, a non-zero byte to $2141,
   then $CC to $2140, and wait until $2140 reads $CC back.
3. For each byte: write it to $2141, write its index (from $00) to $2140,
   and wait until $2140 echoes the index.
4. Repeat for further blocks; to finish, send the entry point with $00 in
   $2141, and the SPC700 jumps to it.

The SDK's drivers do this for you; writing your own SPC700 program is the
raw APU path of @ref tutorial_audio_samples.

## S-DSP registers

The SPC700 reaches the DSP indirectly: the register number goes to $F2, the
value is read or written at $F3.

### Per voice (x = voice 0-7)

| Reg | Name | Bits | Description |
|-----|------|------|-------------|
| x0 | VOLL | 8 | Left volume (signed) |
| x1 | VOLR | 8 | Right volume (signed) |
| x2 | PL | 8 | Pitch low byte |
| x3 | PH | 6 | Pitch high bits (14-bit total) |
| x4 | SRCN | 8 | Sample source number |
| x5 | ADSR1 | 8 | ADSR enable, decay, attack |
| x6 | ADSR2 | 8 | Sustain level, sustain rate |
| x7 | GAIN | 8 | Envelope when ADSR is off |
| x8 | ENVX | 8 | Current envelope (read-only) |
| x9 | OUTX | 8 | Current output (read-only) |

### Global

| Reg | Name | Description |
|-----|------|-------------|
| 0C | MVOLL | Main volume left |
| 1C | MVOLR | Main volume right |
| 2C | EVOLL | Echo volume left |
| 3C | EVOLR | Echo volume right |
| 4C | KON | Key on (start voices) |
| 5C | KOF | Key off (release voices) |
| 6C | FLG | Flags (mute, echo write, noise rate) |
| 7C | ENDX | Sample end flags (read-only) |
| 0D | EFB | Echo feedback |
| 2D | PMON | Pitch modulation enable |
| 3D | NON | Noise enable |
| 4D | EON | Echo enable |
| 5D | DIR | Sample directory (×$100) |
| 6D | ESA | Echo ring start (×$100) |
| 7D | EDL | Echo delay (×16 ms, ×2 KB) |
| xF | COEF | FIR filter coefficients |

### Pitch

The 14-bit pitch is a 2.12 fixed-point rate: **$1000 plays the sample at
32 000 Hz**, its own speed if it was recorded at 32 kHz. The rate is
`pitch × 32000 / $1000` Hz, up to $3FFF, almost two octaves above $1000
(SNESdev wiki, *S-DSP registers*, VxPITCH). One octave is a factor 2: a
sample tuned to C at $1000 gives the A above it near $1AE9.

### Noise

FLG bits 0-4 set the noise rate for every voice that has its NON bit: $00
stops it, $10 is 500 Hz, $1F is 32 kHz.

## BRR samples

BRR (bit rate reduction) is an ADPCM format: blocks of 9 bytes, each
holding 16 samples of 4 bits.

```
Byte 0: header
  Bits 7-4: range (shift amount)
  Bits 3-2: filter (0-3)
  Bit 1:    loop flag
  Bit 0:    end flag

Bytes 1-8: 16 samples of 4 bits
```

| Filter | a | b | Use |
|--------|---|---|-----|
| 0 | 0 | 0 | No prediction |
| 1 | 0.9375 | 0 | Simple prediction |
| 2 | 1.90625 | -0.9375 | Strong prediction |
| 3 | 1.796875 | -0.8125 | Moderate prediction |

A loop point falls on a block boundary, so it is a multiple of 16 samples.

### Sample directory

DIR gives the directory's page (×$100). Entry `n` (the SRCN of a voice) is
4 bytes:

| Offset | Content |
|--------|---------|
| +0 | Sample start address (16-bit) |
| +2 | Loop address (16-bit) |

## Envelopes

### ADSR

With ADSR1 bit 7 set, the voice follows attack, decay, sustain, then
release at key off:

```
ADSR1 (x5)  EDDD AAAA
            │└┴┴ └┴┴┴─ Attack rate (0-15)
            │    └──── Decay rate (0-7)
            └───────── ADSR enable

ADSR2 (x6)  SSSR RRRR
            │││  └┴┴┴┴─ Sustain rate (0-31)
            └┴┴──────── Sustain level (0-7)
```

The rates are indices into a fixed table of the chip, not times. Attack
rises linearly (rate 15 is the fastest), decay falls exponentially to the
sustain level, sustain keeps falling exponentially at its own rate (0 holds
the level), and release, after key off, falls linearly at a fixed speed of
8 per sample (fullsnes, *DSP ADSR/Gain Envelope*; SNESdev wiki, *DSP
envelopes*).

### GAIN

With ADSR off, GAIN drives the envelope:

| Bits | Mode |
|------|------|
| `0vvv vvvv` | Direct: hold the envelope at `v` × 16 |
| `100r rrrr` | Linear decrease at rate `r` |
| `101r rrrr` | Exponential decrease |
| `110r rrrr` | Linear increase |
| `111r rrrr` | Bent increase (fast, then slow near the top) |

## Echo

The echo ring takes `EDL × 2 KB` of ARAM from page ESA, and delays the
sound by `EDL × 16 ms`: EDL 15 is 240 ms and 30 KB. The DSP writes the ring
continuously while echo writing is on (FLG bit 5 clear), so anything else
placed there is overwritten: plan ARAM around it.

| Register | Example value | Effect |
|----------|---------------|--------|
| ESA | $40 | Ring at $4000 |
| EDL | $02 | 32 ms delay, 4 KB ring |
| EFB | $60 | Moderate feedback |
| EVOLL/EVOLR | $40 | Echo at half volume |

The FIR filter's eight coefficients (COEF, $0F-$7F) shape the echo:
`127, 0, 0, 0, 0, 0, 0, 0` passes it unchanged; `32, 32, 32, 32, 0, 0, 0, 0`
is a low-pass.

## Further reading

| Resource | Link |
|----------|------|
| Fullsnes, APU | https://problemkaputt.de/fullsnes.htm#snesapudsp |
| SNESdev wiki, S-DSP registers | https://snes.nesdev.org/wiki/S-DSP_registers |
| SPC700 Reference | https://wiki.superfamicom.org/spc700-reference |
| nesdoug, SNES music | https://nesdoug.com/2020/06/14/snes-music/ |
