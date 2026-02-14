# SFX Demo

> Press A to play a sound. That's it. But behind that one button press
> is a whole second CPU (the SPC700), a custom driver uploaded at boot,
> and a BRR-encoded audio sample. Welcome to SNES audio.

![Screenshot](screenshot.png)

## Controls

| Button | Action |
|--------|--------|
| A | Play sound effect |

## Build & Run

```bash
make -C examples/audio/sfx_demo
# Open sfx_demo.sfc in Mesen2
```

## What You'll Learn

- The SNES has two CPUs — the 65816 for game logic and the SPC700 for audio
- How to upload a driver program to the SPC700 via the IPL boot protocol
- BRR format: the SNES's compressed audio format (like ADPCM, but weirder)
- The port-based communication protocol between the 65816 and SPC700

---

## Walkthrough

### 1. Two Computers in One Console

The SNES audio system is essentially a separate computer. The SPC700 has its own
CPU, its own 64KB of RAM, its own DSP chip, and its own instruction set. It runs
independently from the main 65816 CPU.

The two CPUs communicate through 4 shared ports ($2140-$2143 from the 65816 side,
$F4-$F7 from the SPC700 side). That's it — four bytes. Everything else is done by
uploading code and data to the SPC700's RAM.

### 2. The Upload Protocol

When the SNES powers on, the SPC700 runs its IPL ROM — a tiny bootloader that waits
for data on the ports. The upload protocol is:

1. Wait for `$BBAA` on ports 0-1 (SPC ready signal)
2. Send destination address and `$CC` to start transfer
3. Send bytes one at a time, handshaking on port 0 each time
4. Send a final command with the execution address

```c
/* In C, we just call the assembly routine */
REG_NMITIMEN = 0x00;   /* Disable NMI — upload is timing-critical */
spc_init();
REG_NMITIMEN = 0x81;   /* Re-enable NMI + auto-joypad */
```

> **Why disable NMI?** The SPC upload protocol requires precise handshaking —
> the 65816 writes a byte, waits for the SPC700 to acknowledge, then writes the next.
> If an NMI fires mid-transfer and the handler takes too long, the timing breaks
> and the SPC700 gets confused. Disabling NMI during the upload prevents this.

### 3. What Gets Uploaded

The `spc.asm` file contains a "blob" — a contiguous block that gets uploaded
to SPC700 RAM starting at address `$0200`:

```
$0200-$02FF: SPC700 driver program (~120 bytes + padding)
$0300-$0303: Sample directory (start/loop addresses for sample 0)
$0304+:      BRR sample data (tada.brr)
```

The driver program does three things:
1. Clears SPC700 state (DSP registers, effects, etc.)
2. Configures voice 0 with volume, pitch, and ADSR envelope
3. Enters a loop waiting for a play command on port 0

### 4. The Driver's Main Loop

The SPC700 driver is hand-assembled as raw bytes (`.db` directives) because
it runs on a different CPU with a different instruction set:

```asm
; Main loop - wait for play command on port0
.db $E4, $F4             ; mov A, $F4 (read port0)
.db $F0, $FC             ; beq -4 (loop if 0)

; Got command - echo it back and key on voice 0
.db $C4, $F4             ; mov $F4, A (echo command)
.db $8F, $4C, $F2        ; mov $F2, #$4C (KON register)
.db $8F, $01, $F3        ; mov $F3, #$01 (voice 0 bit)

; Wait for SNES to clear port0, then loop
.db $E4, $F4             ; mov A, $F4
.db $D0, $FC             ; bne -4 (loop if not 0)
.db $C4, $F4             ; mov $F4, A
.db $2F, $EC             ; bra back to main loop
```

The handshake is simple: SNES writes `$01` to port 0, driver echoes it back (so the
SNES knows the command was received), keys on voice 0, then waits for the SNES to
write `$00` back.

### 5. Playing the Sound

From C, it's one function call:

```c
if (pad_pressed & KEY_A) {
    spc_play();
}
```

`spc_play()` in assembly: write `$01` to `$2140`, wait for echo, write `$00`, wait for
clear. The SPC700 driver sees the `$01`, triggers voice 0, and the DSP plays the BRR
sample through the DAC to the speakers.

### 6. BRR — The SNES Audio Format

BRR (Bit Rate Reduction) is a lossy compression format. Each 9-byte BRR block encodes
16 audio samples:
- 1 header byte (shift amount, filter mode, end/loop flags)
- 8 data bytes (4-bit deltas, two samples per byte)

The `tada.brr` file is a pre-converted BRR sample. To create your own:

```bash
# Convert a WAV file to BRR (16-bit mono, 32000 Hz recommended)
brr_encoder input.wav output.brr
```

---

## Tips & Tricks

- **No sound?** Check that NMI was re-enabled after `spc_init()`. Without NMI, the
  joypad auto-read doesn't work, so button presses aren't detected.

- **Sound is distorted or wrong pitch?** The pitch registers (`PITCHL`/`PITCHH`)
  control playback speed. The default `$0440` is roughly middle-C for a 32kHz sample.
  Higher values = higher pitch.

- **Want to change the sound?** Replace `tada.brr` with a different BRR file. Make
  sure the loop point in the sample directory matches (or set it to the start for
  one-shot playback).

- **Why raw `.db` bytes instead of an assembler?** The SPC700 uses a completely
  different instruction set from the 65816. WLA-DX can assemble SPC700 code, but
  for a tiny 120-byte driver, inline bytes work fine and avoid build complexity.

---

## Go Further

- **Multiple samples:** Expand the sample directory and add more BRR files. Use
  different port values ($01 = sample 0, $02 = sample 1) to select which one plays.

- **Volume control:** The DSP's VOLL/VOLR registers ($00/$01) control per-voice volume.
  Send a volume byte on port 1 before the play command.

- **Use SNESMOD instead:** For music + multiple SFX channels, the SNESMOD library
  handles all of this automatically. See [snesmod_music](../snesmod_music/) for
  the high-level approach.

- **Next example:** [SNESMOD Music](../snesmod_music/) — play Impulse Tracker music
  files with the SNESMOD audio driver.

---

## Under the Hood: The Build

### The Makefile

```makefile
TARGET      := sfx_demo.sfc
USE_LIB     := 1
LIB_MODULES := console input sprite dma
CSRC        := main.c
ASMSRC      := spc.asm
```

### The Assembly Link: spc.asm

This is where it gets interesting. `spc.asm` contains three things that can't exist in C:

1. **The SPC700 driver** — raw machine code bytes for a different CPU (`.db $E4, $F4...`)
2. **The BRR sample** — binary audio data (`.INCBIN "tada.brr"`)
3. **The upload routine** — 65816 assembly that bit-bangs the IPL protocol

The SPC700 is a completely separate processor with its own instruction set. You can't
write SPC700 code in C — it compiles to 65816. So the driver is hand-assembled as raw
bytes and included in the ROM alongside the main program.

```asm
.SECTION ".spc_data" SUPERFREE
spc_blob:
    ; SPC700 driver program (~120 bytes)
    .db $8F, $6C, $F2    ; mov $F2, #$6C (FLG register)
    .db $8F, $20, $F3    ; mov $F3, #$20 (mute + echo off)
    ; ... setup DSP registers, enter main loop ...
    .INCBIN "tada.brr"   ; BRR audio sample appended after driver
.ENDS
```

> **Why not use SNESMOD?** SNESMOD is a full audio engine — it handles multiple channels,
> music playback, and mixing. For a single sound effect, it's overkill. This example
> shows the bare-metal approach: upload your own tiny driver, trigger sounds manually.
> It's 200 lines of assembly vs. a multi-KB library.

### BRR: The SNES Audio Format

The `tada.brr` file is a BRR-encoded audio sample. BRR (Bit Rate Reduction) compresses
16 PCM samples into 9 bytes — roughly 4:1 compression. To create your own:

```bash
# From a WAV file (16-bit mono, 32000 Hz recommended):
brr_encoder input.wav output.brr
```

The sample rate matters because the SPC700 DSP's pitch registers assume 32 kHz.
A sample recorded at 44.1 kHz will play back sharp unless you adjust the pitch value.

### No Audio Tool in the Build

Unlike SNESMOD examples (which use `smconv` to convert Impulse Tracker `.it` files),
this example has no audio conversion step in the Makefile. The BRR file is pre-converted
and included directly. This is the simplest possible audio pipeline — one binary file,
one `.INCBIN`, done.

---

## Technical Reference

| Register | Address (65816) | Address (SPC700) | Role |
|----------|----------------|-------------------|------|
| APUIO0   | $2140 | $F4 | Port 0: command/handshake |
| APUIO1   | $2141 | $F5 | Port 1: transfer data |
| APUIO2-3 | $2142-43 | $F6-F7 | Ports 2-3: address for transfers |
| NMITIMEN | $4200 | — | Disable NMI during upload |

### SPC700 DSP Registers (written via $F2/$F3)

| Register | Address | Role in this example |
|----------|---------|---------------------|
| KON      | $4C     | Key on (trigger voice playback) |
| KOF      | $5C     | Key off |
| DIR      | $5D     | Sample directory base address (×$100) |
| FLG      | $6C     | Flags (mute, echo write disable) |
| V0VOLL   | $00     | Voice 0 left volume |
| V0VOLR   | $01     | Voice 0 right volume |
| V0PITCHL | $02     | Voice 0 pitch low byte |
| V0PITCHH | $03     | Voice 0 pitch high byte |
| V0SRCN   | $04     | Voice 0 sample number (index in DIR) |
| V0ADSR1  | $05     | Voice 0 ADSR envelope attack/decay |
| V0ADSR2  | $06     | Voice 0 ADSR envelope sustain/release |

## Files

| File | What's in it |
|------|-------------|
| `main.c` | Text display, input loop, calls `spc_init()`/`spc_play()` (~205 lines) |
| `spc.asm` | SPC700 driver + upload code + BRR sample include (~203 lines) |
| `tada.brr` | BRR audio sample (the "tada" sound effect) |
| `Makefile` | `LIB_MODULES := console input sprite dma` |
