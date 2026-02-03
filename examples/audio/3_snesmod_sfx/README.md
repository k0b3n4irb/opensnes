# SNESMOD Sound Effects Example

Sound effect playback with pitch control using SNESMOD.

## Learning Objectives

After this lesson, you will understand:
- SNESMOD sound effect playback
- Triggering different SFX on button presses
- Real-time pitch modification
- Using SNESMOD without background music

## Prerequisites

- Completed basic audio examples
- Understanding of SNESMOD initialization

---

## What This Example Does

Demonstrates SNESMOD sound effect system:
- Multiple sound effects mapped to buttons
- Three pitch modes (Low, Normal, High)
- Visual feedback showing current pitch
- No background music (SFX only)

```
+----------------------------------------+
|                                        |
|         SNESMOD SFX DEMO               |
|                                        |
|         Pitch: NORMAL                  |
|                                        |
|   A: Tada     B: Strings               |
|   X: Piano    Y: Marimba               |
|   L/R: Cowbell                         |
|   Left/Right: Change Pitch             |
|                                        |
+----------------------------------------+
```

**Controls:**
- A button: Play "Tada" sound
- B button: Play "Strings" sound
- X button: Play "Piano" sound
- Y button: Play "Marimba" sound
- L/R buttons: Play "Cowbell" sound
- D-Pad Left/Right: Cycle pitch (Low/Normal/High)

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| SNESMOD init | Library (`snesmodInit`) |
| SFX playback | Library (`snesmodPlaySfx`) |
| Soundbank | Binary data from SNESMOD tools |
| Display | Direct VRAM writes |
| Input | Direct register access |

---

## SNESMOD Sound Effects

### Initialization

```c
/* Initialize SNESMOD with soundbank */
snesmodInit(soundbank);

/* No music needed - just SFX */
```

### Playing Effects

```c
/* Play sound effect with pitch parameter */
snesmodPlaySfx(SFX_TADA, pitch);

/* Pitch values:
   - 0: Low pitch
   - 1: Normal pitch
   - 2: High pitch
*/
```

---

## Pitch Control

### Pitch Modes

```c
enum PitchMode {
    PITCH_LOW = 0,
    PITCH_NORMAL = 1,
    PITCH_HIGH = 2
};

static u8 current_pitch = PITCH_NORMAL;
```

### Cycling Pitch

```c
if (pressed & KEY_RIGHT) {
    current_pitch++;
    if (current_pitch > PITCH_HIGH) {
        current_pitch = PITCH_LOW;
    }
    updatePitchDisplay();
}
```

---

## Soundbank Structure

SNESMOD soundbanks contain:
- BRR sample data
- Instrument definitions
- Optional music patterns
- Sound effect triggers

### Creating a Soundbank

```bash
# Convert samples and create soundbank
smconv -s soundbank.bnk sample1.wav sample2.wav ...
```

---

## Build and Run

```bash
cd examples/audio/7_snesmod_sfx
make clean && make

# Run in emulator
/path/to/Mesen snesmod_sfx.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | SFX triggering and pitch control |
| `data.asm` | Font tiles for display |
| `soundbank/` | SNESMOD soundbank data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Volume Control

Add volume adjustment:
```c
/* If SNESMOD supports volume parameter */
snesmodPlaySfxVol(SFX_TADA, pitch, volume);
```

### Exercise 2: Random Pitch

Randomize pitch on each play:
```c
u8 random_pitch = (frame_counter % 3);
snesmodPlaySfx(SFX_TADA, random_pitch);
```

### Exercise 3: Rapid Fire

Allow holding button for repeated sounds:
```c
if (pad & KEY_A) {
    if (sfx_cooldown == 0) {
        snesmodPlaySfx(SFX_TADA, pitch);
        sfx_cooldown = 10;  /* Frames between plays */
    }
}
if (sfx_cooldown > 0) sfx_cooldown--;
```

### Exercise 4: Sound Sequence

Play a sequence of sounds:
```c
static const u8 sequence[] = {SFX_TADA, SFX_PIANO, SFX_STRINGS};
/* Play each with delay between */
```

---

## Technical Notes

### SFX Channels

SNESMOD reserves channels for SFX:
- Music uses channels 0-5 (configurable)
- SFX typically use channels 6-7
- Check soundbank configuration

### Polyphony

Playing a new SFX may cut off a previous one depending on channel allocation. Plan your sound design accordingly.

### Sample Rate

SNESMOD samples play at their original rate. Pitch parameter shifts playback speed, affecting both pitch and duration.

---

## What's Next?

**SNESMOD Music:** [SNESMOD Music](../6_snesmod_music/) - Tracker music playback

**Graphics:** Return to [graphics examples](../../graphics/)

---

## License

Code: MIT
Samples: See soundbank source
