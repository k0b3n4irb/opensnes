# Audio Example: Tone / TADA

Play a BRR sound effect when pressing the A button using the SPC700 audio processor.

## Learning Objectives

After this lesson, you will understand:
- How the SNES audio system works (SPC700 + DSP)
- BRR sample format basics
- SPC700 initialization and driver upload
- Triggering sounds from the main CPU
- Direct SPC700 communication protocol

## Prerequisites

- Basic understanding of SNES architecture
- Completed text examples (for graphics basics)

---

## What This Example Does

- Displays "TADA" text on screen
- Waits for A button press
- Plays a sound effect when A is pressed
- Pure bare-metal implementation (no library)

**Controls:**
- A button: Play "TADA" sound effect

---

## Code Type

**Pure C + Assembly (No Library)**

| Component | Type |
|-----------|------|
| Hardware init | Direct register access (`main.c`) |
| Video setup | Direct register access |
| Input reading | Direct register access |
| SPC700 driver | Hand-written assembly (`spc.asm`) |
| Sample data | Embedded BRR (`tada.brr`) |
| Text display | Assembly helper (`data.asm`) |

This example demonstrates the lowest-level approach to SNES audio.

---

## SNES Audio Architecture

The SNES has a separate audio processor called the **SPC700**:

```
+------------------+     +------------------+
|   Main CPU       |     |    SPC700        |
|   (65816)        |<--->|  Audio CPU       |
|                  | I/O |                  |
|   Game Logic     |Ports|   Sound Driver   |
+------------------+     +------------------+
                              |
                              v
                        +------------------+
                        |   DSP            |
                        | (8 channels)     |
                        +------------------+
                              |
                              v
                        +------------------+
                        |   Audio Output   |
                        +------------------+
```

### Key Points

- SPC700 runs independently with its own RAM (64KB)
- Communication via 4 I/O ports ($2140-$2143)
- Samples are in BRR format (compressed)
- DSP handles mixing, effects, envelope

---

## BRR Sample Format

BRR (Bit Rate Reduction) is the SNES native audio format:

- 9 bytes = 16 samples (compressed)
- ~3.5:1 compression ratio
- Tools like `wav2brr` or `brr_encoder` convert from WAV

```
BRR Block (9 bytes):
+--------+--------------------------------+
| Header | 8 bytes of sample data        |
| (1B)   | (16 4-bit samples)            |
+--------+--------------------------------+
```

---

## How It Works

### 1. SPC700 Initialization

The driver upload uses the IPL ROM protocol:

```asm
spc_init:
    ; Wait for SPC ready ($BBAA in port0-1)
    ldx $2140
    cpx #$BBAA
    bne -

    ; Start transfer to $0200
    ldx #$0200
    stx $2142
    lda #$CC
    sta $2140

    ; Upload byte by byte, incrementing counter
    ; ... (see spc.asm for full code)

    ; Execute driver at $0200
    ldx #$0200
    stx $2142
    sta $2140
```

### 2. The SPC Driver

A minimal driver that:
1. Initializes DSP registers
2. Sets up sample directory
3. Waits for play command on port0
4. Keys on voice 0 when triggered

```asm
; Main loop - wait for play command
@main_loop:
    mov A, $F4     ; Read port0
    beq -          ; Loop if 0

    ; Got command - echo it back
    mov $F4, A

    ; Key on voice 0
    mov $F2, #$4C  ; KON register
    mov $F3, #$01  ; Voice 0

    ; Wait for port0 to return to 0
    ; ...
    bra @main_loop
```

### 3. Playing a Sound

From C, send a command to the SPC:

```c
void spc_play(void) {
    // Send play command
    REG_APUIO0 = 0x01;

    // Wait for acknowledgment
    while (REG_APUIO0 != 0x01);

    // Reset command
    REG_APUIO0 = 0x00;
}
```

### 4. Main Loop

```c
while (1) {
    /* Wait for auto-read complete */
    while (REG_HVBJOY & 0x01) {}
    joy = REG_JOY1H;

    /* A button pressed (new press only)? */
    if ((joy & 0x80) && !prev_a) {
        spc_play();
    }
    prev_a = joy & 0x80;

    /* VBlank wait */
    while (!(REG_HVBJOY & 0x80)) {}
    while (REG_HVBJOY & 0x80) {}
}
```

---

## SPC Memory Map

```
$0200-$02FF: Driver code (~100 bytes)
$0300-$0303: Sample directory (4 bytes)
$0304+:      BRR sample data (~8KB)
```

The sample directory tells the DSP where to find samples:
```asm
sample_dir:
    .db $04, $03, $04, $03   ; Sample 0: start=$0304, loop=$0304
```

---

## Build and Run

```bash
cd examples/audio/1_tone
make clean && make

# Run in emulator
/path/to/Mesen tone.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Main program (input, hardware init) |
| `spc.asm` | SPC700 driver and upload code |
| `data.asm` | Font tiles and text for display |
| `tada.brr` | BRR-encoded sound effect |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Multiple Sounds
Add a second sound effect triggered by the B button.

**Hint:** Expand the sample directory and use port0 value to select sample.

### Exercise 2: Volume Control
Modify the SPC driver to accept volume in port1.

**Hint:** Write to DSP registers $00/$01 (VOLL/VOLR).

### Exercise 3: Create Your Own Sound
1. Find a short WAV file (8-16kHz, mono)
2. Convert with `brr_encoder` or `wav2brr` tool
3. Replace `tada.brr`
4. Rebuild and test

---

## Common Issues

### No sound
- Check SPC700 initialization completed (port0 should echo)
- Verify emulator audio is enabled
- Check sample directory addresses match BRR location

### Sound plays immediately on boot
- Ensure button edge detection (track previous state)

### Crackling or distorted audio
- Sample rate mismatch (32kHz native)
- BRR encoding quality

### Sound cuts off early
- Check ADSR envelope settings
- Verify loop point in sample directory

---

## DSP Register Summary

Key DSP registers used by the driver:

| Register | Purpose |
|----------|---------|
| $00-$01 | Voice 0 volume L/R |
| $02-$03 | Voice 0 pitch |
| $04 | Voice 0 sample source |
| $05-$06 | Voice 0 ADSR |
| $0C-$1C | Master volume L/R |
| $4C | KON (key on) |
| $5C | KOFF (key off) |
| $5D | DIR (sample directory page) |

---

## What's Next?

**More Audio:** Check `2_sfx` for multiple sound effects

**Music:** See `6_snesmod_music` for tracker-based music playback

**Graphics:** Return to [Animation Example](../../graphics/2_animation/)

---

## License

Code: MIT
Sample: Public Domain
