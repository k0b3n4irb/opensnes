# SPC700 Audio Programming Guide

This document captures lessons learned from implementing SPC700 audio support in OpenSNES.

## Overview

The SNES has a separate audio subsystem consisting of:
- **SPC700**: 8-bit CPU running at ~2.048 MHz
- **DSP**: Digital Signal Processor with 8 voices
- **64KB Audio RAM**: Shared between SPC700 code and sample data

The SPC700 runs independently from the main 65816 CPU. Communication happens through 4 I/O ports.

## Memory Architecture

### SPC700 Address Space (64KB)
```
$0000-$00EF  - Zero Page (direct page addressing)
$00F0-$00FF  - Hardware registers (DSP, I/O ports, timers)
$0100-$01FF  - Stack
$0200-$FFBF  - Available for code and data
$FFC0-$FFFF  - IPL ROM (boot loader, can be disabled)
```

### Typical Memory Layout for Custom Driver
```
$0200  - Driver code start
$0300  - Sample directory (4 bytes per sample)
$0400+ - BRR sample data
$6000  - Echo buffer (if used)
```

## I/O Port Communication

### Port Registers
| 65816 Side | SPC700 Side | Direction |
|------------|-------------|-----------|
| $2140 | $F4 | Bidirectional |
| $2141 | $F5 | Bidirectional |
| $2142 | $F6 | Bidirectional |
| $2143 | $F7 | Bidirectional |

### Critical Insight: Separate Latches
Each port has TWO latches - one for each direction:
- When 65816 writes to $2140, SPC700 reads it from $F4
- When SPC700 writes to $F4, 65816 reads it from $2140
- **They do NOT see their own writes!**

### Working Communication Protocol (Echo-Based)
After extensive testing, this protocol proved reliable:

```
SPC700 Side:
@loop:
    mov A, $F4      ; Read what 65816 wrote
    mov $F4, A      ; Echo it back
    cmp A, #$55     ; Check for play command
    bne @loop
    ; ... trigger sound ...
    bra @loop

65816 Side:
    ; Send command
    REG_APUIO0 = 0x55;

    ; Wait for echo (confirms SPC received it)
    while (REG_APUIO0 != 0x55) {}

    ; Clear command
    REG_APUIO0 = 0x00;
    while (REG_APUIO0 != 0x00) {}
```

### Failed Approaches
1. **Change Detection**: Comparing port value against saved "last command" failed because the initial value after IPL boot was unpredictable.

2. **Ready Signal + Acknowledge**: Having SPC write 0x42 as "ready" and waiting for 65816 to acknowledge caused timing issues.

3. **Polling for Specific Value Without Echo**: Without echo confirmation, the 65816 couldn't know if SPC processed the command.

## DSP Registers

### Register Access
The DSP is accessed through two SPC700 registers:
- `$F2` - DSP register address
- `$F3` - DSP register data

```asm
; Write $7F to DSP register $0C (MVOLL)
mov $F2, #$0C
mov $F3, #$7F
```

### Key DSP Registers

#### Global Registers
| Addr | Name | Description |
|------|------|-------------|
| $0C | MVOLL | Master volume left |
| $1C | MVOLR | Master volume right |
| $5D | DIR | Sample directory address (high byte, page-aligned) |
| $6C | FLG | Flags (mute, echo, reset) |
| $4C | KON | Key on (trigger voices) |
| $5C | KOF | Key off (stop voices) |

#### Per-Voice Registers (Voice 0 shown, add $10 for each voice)
| Addr | Name | Description |
|------|------|-------------|
| $00 | V0VOLL | Voice 0 left volume |
| $01 | V0VOLR | Voice 0 right volume |
| $02 | V0PITCHL | Voice 0 pitch (low byte) |
| $03 | V0PITCHH | Voice 0 pitch (high byte) |
| $04 | V0SRCN | Voice 0 source number (sample index) |
| $05 | V0ADSR1 | Voice 0 ADSR settings 1 |
| $06 | V0ADSR2 | Voice 0 ADSR settings 2 |
| $07 | V0GAIN | Voice 0 gain (if not using ADSR) |

## BRR Sample Format

BRR (Bit Rate Reduction) is the SNES audio compression format:
- 9 bytes per block: 1 header + 8 data bytes
- Each block = 16 PCM samples
- ~4:1 compression ratio

### Sample Directory
Located at address specified by DIR register (multiplied by $100):
```
Entry 0: Start address (2 bytes), Loop address (2 bytes)
Entry 1: Start address (2 bytes), Loop address (2 bytes)
...
```

### Pitch Calculation
```
playback_rate = (pitch / $1000) * 32000 Hz
```

For a sample recorded at 32000 Hz:
- Pitch $1000 = normal speed
- Pitch $0800 = half speed (octave down)
- Pitch $2000 = double speed (octave up)

**Discovered:** For the tada.brr sample (32kHz), pitch $0440 produced correct playback.

## IPL Boot Sequence

The SPC700 boots from IPL ROM and waits for the 65816 to upload code.

### Boot Protocol
1. SPC signals ready: Port0=$AA, Port1=$BB
2. 65816 sends data in blocks
3. Final block triggers execution at specified address

### spc_upload() Function
```c
void spc_upload(u16 addr, const u8 *data, u16 size);
```
Uploads data to SPC memory using IPL protocol.

### spc_execute() Function
```c
void spc_execute(u16 addr);
```
Starts execution at specified address.

## Common Pitfalls

### 1. Forgetting to Initialize DSP
All DSP registers should be explicitly set. The DSP state after boot is undefined.

### 2. KON/KOF Timing
After writing to KOF, you need a small delay before the voice can be keyed on again:
```asm
mov $F2, #$5C   ; KOF
mov $F3, #$FF   ; All voices off

; Delay loop
mov X, #$10
mov Y, #$00
-   dec Y
    bne -
    dec X
    bne -

mov $F2, #$5C   ; KOF
mov $F3, #$00   ; Clear KOF
```

### 3. Echo Buffer Corruption
If echo is enabled, the echo buffer WILL be written to. Make sure ESA (echo start address) points to unused memory.

### 4. Port Synchronization After IPL
After `spc_execute()`, the ports contain leftover values from the IPL protocol. The SPC driver should immediately start echoing to establish synchronization.

## Minimal Working Driver

```c
static const u8 spc_driver[] = {
    /* Initialize DSP... (omitted for brevity) */

    /* Main loop: echo port0, play on 0x55 */
    0xE4, 0xF4,         /* mov A, $F4       ; read port0 */
    0xC4, 0xF4,         /* mov $F4, A       ; echo back */
    0x68, 0x55,         /* cmp A, #$55      ; play command? */
    0xD0, 0xF8,         /* bne @loop (-8)   ; no, loop */

    /* Key on voice 0 */
    0x8F, 0x4C, 0xF2,   /* mov $F2, #$4C */
    0x8F, 0x01, 0xF3,   /* mov $F3, #$01 */

    /* Delay to prevent re-trigger */
    0xCD, 0xFF,         /* mov X, #$FF */
    0x1D,               /* dec X */
    0xD0, 0xFD,         /* bne -3 */

    /* Back to loop */
    0x2F, 0xEB,         /* bra @loop (-21) */
};
```

## SPC700 Instruction Reference (Common)

| Opcode | Instruction | Description |
|--------|-------------|-------------|
| 8F xx yy | mov dp, #imm | Store immediate to direct page |
| E4 xx | mov A, dp | Load A from direct page |
| C4 xx | mov dp, A | Store A to direct page |
| 68 xx | cmp A, #imm | Compare A with immediate |
| CD xx | mov X, #imm | Load X with immediate |
| 8D xx | mov Y, #imm | Load Y with immediate |
| 1D | dec X | Decrement X |
| DC | dec Y | Decrement Y |
| D0 xx | bne rel | Branch if not equal |
| F0 xx | beq rel | Branch if equal |
| 2F xx | bra rel | Branch always |

### Branch Offset Calculation
Relative branches use signed 8-bit offsets from the address AFTER the instruction:
```
-1 = $FF
-2 = $FE
-3 = $FD
...
-16 = $F0
```

## Testing Tips

1. **Use Mesen2**: Excellent SPC700 debugger, DSP viewer, and audio debugging tools.

2. **Start Simple**: Get a working "UZI" test (continuous rapid playback) before adding command detection.

3. **Echo Everything**: The echo-based protocol is the most reliable for 65816-SPC communication.

4. **Check Pitch**: If audio sounds wrong (too fast/slow), adjust pitch register. Use known-working samples from PVSnesLib as reference.

---

## SNESMOD Reference Implementation

SNESMOD (https://github.com/mukunda-/snesmod) is a well-documented open-source audio library that provides excellent reference material for advanced SPC700 driver design.

### SNESMOD Architecture Overview

**Components:**
- **sm-spc**: SPC700 driver (~5KB) handling music playback and sound effects
- **smconv**: Tool converting Impulse Tracker (.it) files to soundbanks
- **65816 API**: SNES-side library for driver communication

### SNESMOD Memory Layout

```
$0000-$00EF  - Zero Page (channel state, module vars, temps)
$00F0-$00FF  - Hardware registers
$0100-$01FF  - Stack
$0200-$02FF  - Sample directory (64 entries × 4 bytes)
$0300-$033F  - Effect directory (16 SFX × 4 bytes)
$0380-$03FF  - Pattern command cache (16 × 8 bytes)
$0400+       - Driver code
$1A00+       - Module data (sequence, patterns, instruments, samples)
$D000+       - Echo buffer (EDL × 2KB)
```

### SNESMOD Command Protocol

Uses validation-based handshake (not simple echo):

| Command | ID | Port1 | Port2 | Port3 | Description |
|---------|-----|-------|-------|-------|-------------|
| LOAD | $00 | module_id | - | - | Load music module |
| PLAY | $01 | position | - | - | Start playback |
| STOP | $02 | - | - | - | Stop playback |
| VOLUME | $03 | volume | - | - | Set module volume |
| FADE | $04 | target | speed | - | Fade volume |
| EFFECT | $05 | sfx_id | vol+pan | pitch | Play sound effect |
| STREAM | $08+ | data... | - | - | Stream audio data |

**Handshake Pattern:**
```
65816:  Write command ID + validation byte to ports
SPC700: Echo validation byte through PORT1 when processed
65816:  Detect echo = command acknowledged
```

### SNESMOD 65816 API

```c
/* Initialization */
void spcBoot(void);                    /* Upload driver, init SPC */
void spcSetBank(u8 bank);              /* Set soundbank ROM bank */
void spcFlush(void);                   /* Sync message queue */

/* Module Playback */
void spcLoad(u8 module_id);            /* Load module to ARAM */
void spcPlay(u8 position);             /* Start at pattern */
void spcStop(void);                    /* Stop playback */

/* Volume */
void spcSetModuleVolume(u8 vol);       /* 0-255 immediate */
void spcFadeModuleVolume(u8 target, u8 speed);  /* Gradual fade */

/* Sound Effects */
void spcLoadEffect(u8 sfx_id);         /* Load SFX sample */
void spcEffect(u8 vol_pan, u8 idx, u8 pitch);  /* Play SFX */

/* Status */
u8 spcReadStatus(void);                /* Get status flags */
void spcProcess(void);                 /* Call once per frame */
```

**Status Flags:**
- `SPC_F ($80)`: Fade in progress
- `SPC_E ($40)`: End of module reached
- `SPC_P ($20)`: Module currently playing

### SNESMOD Pattern Sequencer

The driver implements a tracker-style sequencer:

1. **Tick System**: Speed value determines ticks per row
2. **Maskvar**: Bitfield indicating which elements present (note, instrument, volume, effect)
3. **Effect Memory**: Cached parameters for slides/vibrato continuation
4. **Envelope Processing**: 4-byte nodes with target + duration

### SNESMOD Supported Effects

| Effect | Hex | Description |
|--------|-----|-------------|
| Dxy | Volume slide | x=up, y=down |
| Exx/Fxx | Pitch slide | Down/Up |
| Gxx | Portamento to note |
| Hxy | Vibrato | x=speed, y=depth |
| Jxy | Arpeggio | +x, +y semitones |
| Mxx/Nxy | Channel volume |
| Pxy/Xxy | Panning |
| S0x | Echo control | 1=off, 2=on, 3=all off, 4=all on |

### SNESMOD Echo Configuration

Set via song message with `[[SNESMOD]]` tag:
```
[[SNESMOD]]
EDL 4        ; Delay: 4 × 16ms = 64ms
EFB 64       ; Feedback: -128 to 127
EVOL 40 40   ; Echo volume L/R
EFIR 127 0 0 0 0 0 0 0   ; FIR coefficients
EON 1 2 3    ; Enable echo on channels 1,2,3
```

**Memory Cost**: Each 16ms delay = 2KB ARAM

### Key Insights from SNESMOD

1. **Volume Chain**: Final volume = channel × instrument × sample × module × envelope × fadeout × master

2. **Pitch Calculation**: Uses lookup tables (`LUT_DIV3`, `LUT_FTAB`) for note-to-DSP conversion

3. **SFX Priority**: Sound effects use `sfx_mask` to claim voices, channel 8 often reserved

4. **Sample Constraints**:
   - Length and loop point must be divisible by 16
   - 8-bit → 9/16 size after BRR, 16-bit → 9/32 size
   - Stereo not supported
   - Max 64 samples, must fit in ~58KB with driver

5. **Streaming**: Supports SNES-to-SPC streaming for long samples (Star Ocean style)

### SNESMOD vs OpenSNES Audio

OpenSNES supports **both** audio modes:

| Feature | Legacy Driver | SNESMOD Mode |
|---------|---------------|--------------|
| Music format | BRR samples only | Impulse Tracker (.it) |
| Driver size | ~500 bytes | ~5.5KB |
| Voices | 8 | 8 |
| Effects | Vol/pan/pitch | Full tracker set |
| Echo/reverb | Manual setup | Automatic via IT |
| Use case | Simple SFX | Full game audio |

**Build Selection:**
- Default: Legacy driver (`lib/source/audio.asm`)
- SNESMOD: Set `USE_SNESMOD=1` in Makefile

**Design Choice**: Legacy driver is readable and educational. SNESMOD provides production-quality tracker audio for complete games.
