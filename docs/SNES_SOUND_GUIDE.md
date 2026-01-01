# SNES Sound Programming Guide

This guide covers the SNES audio system for OpenSNES game development.

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [SPC700 CPU](#spc700-cpu)
3. [DSP Registers](#dsp-registers)
4. [BRR Sample Format](#brr-sample-format)
5. [ADSR Envelopes](#adsr-envelopes)
6. [Echo System](#echo-system)
7. [Communication Protocol](#communication-protocol)
8. [Asset Requirements](#asset-requirements)
9. [Tools & Resources](#tools--resources)

---

## Architecture Overview

The SNES audio system is completely **separate** from the main CPU:

| Component | Details |
|-----------|---------|
| **APU** | Audio Processing Unit (Sony SPC700) |
| **CPU** | 8-bit @ 1.024 MHz |
| **RAM** | 64 KB (shared program/data/samples) |
| **DSP** | 8 channels, BRR compression |
| **Sample Rate** | 32 kHz output |

### Key Capabilities

- 8 simultaneous channels
- 32 kHz sample playback
- BRR compression (~3.5:1 ratio)
- Echo with 8-tap FIR filter (up to 240ms delay)
- ADSR envelopes per channel
- Pitch modulation between channels
- Noise generator

---

## SPC700 CPU

### Registers

| Register | Size | Purpose |
|----------|------|---------|
| A | 8-bit | Accumulator |
| X | 8-bit | Index register |
| Y | 8-bit | Index register (pairs with A for 16-bit) |
| SP | 8-bit | Stack pointer (page 1) |
| PC | 16-bit | Program counter |
| PSW | 8-bit | Status flags |

### Memory Map

| Address | Purpose |
|---------|---------|
| $0000-$00EF | Zero page (fast access) |
| $00F0-$00FF | Hardware registers |
| $0100-$01FF | Stack |
| $0200-$FFBF | Program/data/samples |
| $FFC0-$FFFF | IPL ROM (boot code) |

### Communication Ports

| SNES Side | SPC Side | Purpose |
|-----------|----------|---------|
| $2140 | $F4 | Port 0 |
| $2141 | $F5 | Port 1 |
| $2142 | $F6 | Port 2 |
| $2143 | $F7 | Port 3 |

**Important**: Use 8-bit writes only! 16-bit writes can corrupt port 3.

---

## DSP Registers

Access via indirect addressing: write address to `$F2`, read/write data at `$F3`.

### Per-Voice Registers (x = voice 0-7)

| Reg | Name | Bits | Description |
|-----|------|------|-------------|
| x0 | VOLL | 8 | Left volume (signed) |
| x1 | VOLR | 8 | Right volume (signed) |
| x2 | PL | 8 | Pitch low byte |
| x3 | PH | 6 | Pitch high byte (14-bit total) |
| x4 | SRCN | 8 | Sample source number |
| x5 | ADSR1 | 8 | ADSR enable, decay, attack |
| x6 | ADSR2 | 8 | Sustain level, release |
| x7 | GAIN | 8 | Envelope (if ADSR disabled) |
| x8 | ENVX | 8 | Current envelope (read-only) |
| x9 | OUTX | 8 | Current output (read-only) |

### Global Registers

| Reg | Name | Description |
|-----|------|-------------|
| 0C | MVOLL | Main volume left |
| 1C | MVOLR | Main volume right |
| 2C | EVOLL | Echo volume left |
| 3C | EVOLR | Echo volume right |
| 4C | KON | Key on (start voices) |
| 5C | KOF | Key off (release voices) |
| 6C | FLG | Flags (mute, echo, noise rate) |
| 7C | ENDX | Sample end flags (read-only) |
| 0D | EFB | Echo feedback |
| 2D | PMON | Pitch modulation enable |
| 3D | NON | Noise enable |
| 4D | EON | Echo enable |
| 5D | DIR | Sample directory (×$100) |
| 6D | ESA | Echo buffer start (×$100) |
| 7D | EDL | Echo delay (×16ms, ×2KB) |
| xF | COEF | FIR filter coefficients |

---

## BRR Sample Format

BRR (Bit Rate Reduction) is ADPCM compression with ~3.5:1 ratio.

### Block Structure (9 bytes)

```
Byte 0: Header
  Bits 7-4: Range (shift amount)
  Bits 3-2: Filter (0-3)
  Bit 1:    Loop flag
  Bit 0:    End flag

Bytes 1-8: Data (16 × 4-bit samples)
```

### Filters

| Filter | a | b | Use |
|--------|---|---|-----|
| 0 | 0 | 0 | No prediction |
| 1 | 0.9375 | 0 | Simple prediction |
| 2 | 1.90625 | -0.9375 | Strong prediction |
| 3 | 1.796875 | -0.8125 | Moderate prediction |

### Sample Directory

Location set by `DIR` register (×$100). Each entry is 4 bytes:

| Offset | Content |
|--------|---------|
| +0 | Sample start address (16-bit) |
| +2 | Loop address (16-bit) |

---

## ADSR Envelopes

When ADSR enabled (bit 7 of x5 = 1):

### ADSR1 Register (x5)
```
EDDD AAAA
│└┴┴ └┴┴┴─ Attack rate (0-15)
│    └───── Decay rate (0-7)
└────────── ADSR enable
```

### ADSR2 Register (x6)
```
SSSR RRRR
│││  └┴┴┴┴─ Sustain release rate (0-31)
└┴┴──────── Sustain level (0-7)
```

### Timing Values

| Attack | Time | Decay | Time | Release | Time |
|--------|------|-------|------|---------|------|
| 0 | 4.1s | 0 | 1.2s | 0 | ∞ |
| 15 | 0ms | 7 | 37ms | 31 | 18ms |

### GAIN Modes (when ADSR disabled)

| Mode | Bits | Description |
|------|------|-------------|
| Direct | 0xxx xxxx | Set envelope directly |
| Inc Linear | 10xx xxxx | Rise by 1/64 |
| Inc Bent | 11xx xxxx | Rise, slow near top |
| Dec Linear | 100x xxxx | Fall by 1/64 |
| Dec Exp | 101x xxxx | Fall × 255/256 |

---

## Echo System

### Configuration

| Register | Value | Effect |
|----------|-------|--------|
| ESA | $40 | Echo buffer at $4000 |
| EDL | $02 | 32ms delay, 4KB buffer |
| EFB | $60 | Moderate feedback |
| EVOL | $40 | 50% echo volume |

### Memory Usage

```
Echo memory = EDL × 2KB
Delay time = EDL × 16ms
```

**Warning**: Echo buffer overwrites memory! Plan ARAM layout carefully.

### FIR Filter Coefficients (COEF 0F-7F)

Default (no filtering): `127, 0, 0, 0, 0, 0, 0, 0`

Example low-pass: `32, 32, 32, 32, 0, 0, 0, 0`

---

## Communication Protocol

### Upload Protocol (Boot)

1. SNES waits for `$AA` at $2140, `$BB` at $2141
2. SNES sends destination address to $2142-$2143
3. SNES sends `$CC` to $2140
4. SNES uploads data byte-by-byte via $2141
5. Repeat for additional blocks
6. Send entry point and `$00` to execute

### Runtime Communication

```c
// Send command to SPC
void spc_send_command(u8 cmd, u8 data) {
    while (REG_APUIO0 != last_ack);  // Wait for SPC
    REG_APUIO1 = data;
    REG_APUIO0 = cmd;
    last_ack = cmd;
}
```

---

## Asset Requirements

### Sample Preparation

| Parameter | Recommendation |
|-----------|----------------|
| Source rate | 22-32 kHz |
| Bit depth | 16-bit |
| Duration | < 1 second per sample |
| Loop point | Multiple of 16 samples |

### Memory Budget (64KB total)

| Usage | Typical Size |
|-------|--------------|
| Sound driver | 2-4 KB |
| Sequence data | 2-8 KB |
| Echo buffer | 0-30 KB |
| **Samples** | **~30-50 KB** |

### Typical Sample Sizes

| Instrument | Raw Size | BRR Size |
|------------|----------|----------|
| Drum kit | 32 KB | ~9 KB |
| Bass | 8 KB | ~2.3 KB |
| Lead | 4 KB | ~1.1 KB |
| SFX | 2 KB | ~0.6 KB |

---

## Tools & Resources

### Sound Creation Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| **OpenMPT** | IT/XM tracker | Export to SNESMOD format |
| **SNESGSS** | Native tracker | By Shiru |
| **C700** | VST plugin | SPC700 emulation |
| **Audacity** | Sample editing | Free |

### Conversion Tools

| Tool | Purpose |
|------|---------|
| **SNESMOD** | IT → SPC conversion |
| **snesbrr** | WAV → BRR conversion |
| **brr2wav** | BRR → WAV (preview) |
| **Split700** | Extract samples from SPC |

### Sample Sources

| Source | License |
|--------|---------|
| [Freesound.org](https://freesound.org) | Various (check each) |
| [OpenGameArt](https://opengameart.org) | CC0/CC-BY |
| [Kenney.nl](https://kenney.nl) | CC0 |

### Documentation

| Resource | Link |
|----------|------|
| SPC700 Reference | https://wiki.superfamicom.org/spc700-reference |
| nesdoug Sound Tutorial | https://nesdoug.com/2020/06/14/snes-music/ |
| Wikibooks SPC700 | https://en.wikibooks.org/wiki/Super_NES_Programming/SPC700_reference |

---

## Quick Reference

### Pitch Calculation

```
Pitch register = Hz × 128 / 1000
Hz = Pitch register × 1000 / 128
```

| Note | Hz | Pitch |
|------|-----|-------|
| C4 | 262 | $1000 (if sampled at 32kHz) |
| A4 | 440 | $1B00 |

### Common Operations

```
// Start voice 0
DSP[$4C] = 0x01;  // KON

// Stop voice 0
DSP[$5C] = 0x01;  // KOF

// Set voice 0 pitch
DSP[$02] = pitch_low;
DSP[$03] = pitch_high;

// Set voice 0 volume
DSP[$00] = left_vol;
DSP[$01] = right_vol;
```

### Noise Frequencies

| FLG bits 0-4 | Frequency |
|--------------|-----------|
| $00 | 0 Hz |
| $10 | 500 Hz |
| $1F | 32 kHz |
