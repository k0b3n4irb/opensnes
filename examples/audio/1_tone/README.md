# Lesson 1: Tone (Audio)

Play a sound effect when pressing a button using the SPC700 audio processor.

## Learning Objectives

After this lesson, you will understand:
- How the SNES audio system works (SPC700)
- BRR sample format basics
- SPC700 initialization and communication
- Triggering sounds from the main CPU

## Prerequisites

- Basic understanding of SNES architecture
- Completed text or graphics examples

---

## SNES Audio Architecture

The SNES has a separate audio processor called the **SPC700**:

```
┌─────────────────┐     ┌─────────────────┐
│   Main CPU      │     │    SPC700       │
│   (65816)       │◄───►│  Audio CPU      │
│                 │ I/O │                 │
│   Game Logic    │Ports│   Sound Driver  │
└─────────────────┘     └─────────────────┘
                              │
                              ▼
                        ┌─────────────────┐
                        │   DSP           │
                        │ (8 channels)    │
                        └─────────────────┘
                              │
                              ▼
                        ┌─────────────────┐
                        │   Audio Output  │
                        └─────────────────┘
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
- Tools like `wav2brr` convert from WAV

```
BRR Block (9 bytes):
┌────────┬────────────────────────────────┐
│ Header │ 8 bytes of sample data         │
│ (1B)   │ (16 4-bit samples)             │
└────────┴────────────────────────────────┘
```

---

## Files in This Example

| File | Purpose |
|------|---------|
| `main.c` | Main program (input, SPC communication) |
| `spc.asm` | SPC700 driver code |
| `data.asm` | Sample data and SPC binary |
| `tada.brr` | Sound effect sample |
| `Makefile` | Build configuration |

---

## How It Works

### 1. SPC700 Initialization

The main CPU uploads the sound driver to SPC700 RAM:

```c
void spc_init(void) {
    // Wait for SPC700 ready signal
    while (REG_APUIO0 != 0xAA);
    while (REG_APUIO1 != 0xBB);

    // Upload driver code and samples
    // ... (see spc.asm for details)

    // Start the driver
    REG_APUIO0 = start_address_low;
    REG_APUIO1 = start_address_high;
}
```

### 2. Playing a Sound

Once initialized, trigger sounds via I/O ports:

```c
void spc_play(void) {
    // Send play command to SPC700
    REG_APUIO0 = 0x01;  // Play command

    // Wait for acknowledgment
    while (REG_APUIO0 == 0x01);
}
```

### 3. Main Loop

```c
int main(void) {
    // Initialize hardware
    init_hardware();

    // Initialize SPC700 and upload driver
    spc_init();

    while (1) {
        wait_vblank();

        // Read joypad
        u16 joy = read_joypad();

        // Play sound on A button press
        if (joy & KEY_A) {
            spc_play();
            // Wait for button release to avoid repeat
            while (read_joypad() & KEY_A) {
                wait_vblank();
            }
        }
    }
}
```

---

## Build and Run

```bash
cd examples/audio/1_tone
make clean && make

# Test with emulator
/path/to/Mesen tone.sfc
```

**Controls:**
- A button: Play sound effect

---

## Testing

Automated tests verify this example works correctly:

```bash
# Run from project root
cd tests
./run_tests.sh examples
```

Or run the specific test in Mesen2:
```
tests/examples/tone/test_tone.lua
```

### Test Coverage

- ROM boots and reaches `main()`
- Hardware initialization (`InitHardware`)
- SPC700 initialization (`spc_init`)
- `spc_play` function exists
- VBlank handler is operational
- No WRAM mirror overlaps

See [snesdbg](../../../tools/snesdbg/) for the debug library used in tests.

---

## Common Issues

### No sound
- Check SPC700 initialization completed
- Verify sample data is uploaded correctly
- Make sure emulator audio is enabled

### Sound plays immediately on boot
- Check button detection logic
- Ensure proper debouncing

### Crackling or distorted audio
- Sample rate mismatch
- BRR encoding issues

---

## Exercises

### Exercise 1: Multiple Sounds
Add a second sound effect triggered by the B button.

### Exercise 2: Volume Control
Modify the SPC driver to support volume levels.

### Exercise 3: Create Your Own Sound
1. Find a short WAV file
2. Convert with `wav2brr` tool
3. Replace `tada.brr`
4. Rebuild and test

---

## See Also

- [SPC700 Audio Guide](../../../.claude/skills/spc700-audio.md) - Detailed SPC700 programming
- [KNOWLEDGE.md](../../../.claude/KNOWLEDGE.md) - SNES development knowledge base
