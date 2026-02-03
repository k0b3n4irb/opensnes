# SNESMOD Music Example

Full music playback using SNESMOD, a tracker-based audio engine for the SNES.

## Learning Objectives

After this lesson, you will understand:
- How SNESMOD works (module-based music)
- Loading soundbanks and modules
- Music playback controls (play, stop, pause, fade)
- Volume control
- The smconv tool workflow

## Prerequisites

- Completed [1_tone](../1_tone/) example (audio basics)
- Understanding of BG text rendering
- Basic tracker music concepts (optional)

---

## What This Example Does

- Displays a menu showing controls
- Plays an Impulse Tracker module on startup
- Full playback controls via joypad
- Volume adjustment with L/R triggers
- Fade out effect

**Controls:**
| Button | Action |
|--------|--------|
| A | Play music |
| B | Stop music |
| X | Pause/Resume toggle |
| L/R | Volume down/up |
| START | Fade out |

---

## Code Type

**Mixed C + Library + SNESMOD**

| Component | Type |
|-----------|------|
| Hardware init | Library (`consoleInit()`) |
| Video mode | Library (`setMode()`) |
| Screen enable | Library (`setScreenOn()`) |
| VBlank sync | Library (`WaitForVBlank()`) |
| Audio | SNESMOD library |
| Font tiles | Embedded C array |
| Text rendering | Direct VRAM writes |

---

## SNESMOD Overview

SNESMOD is a tracker-based music playback system:

```
Impulse Tracker file (.it)
         |
         v
    [smconv tool]
         |
         v
soundbank.asm + soundbank.h
         |
         v
   Linked into ROM
         |
         v
    [SNESMOD driver]
         |
         v
    SPC700 playback
```

### Key Features

- 8 simultaneous channels
- Impulse Tracker module support
- Automatic BRR sample compression
- Volume control and fading
- Echo/reverb effects
- ~5.5KB driver footprint

---

## SNESMOD API

### Initialization

```c
#include <snes/snesmod.h>
#include "soundbank.h"

snesmodInit();                    /* Initialize SPC driver */
snesmodSetSoundbank(SOUNDBANK_BANK);  /* Set soundbank bank */
snesmodLoadModule(MOD_POLLEN8);   /* Load module by ID */
snesmodPlay(0);                   /* Start from position 0 */
```

### Main Loop (CRITICAL!)

```c
while (1) {
    WaitForVBlank();
    snesmodProcess();  /* MUST call every frame! */
    // ... game logic ...
}
```

**Warning:** Failing to call `snesmodProcess()` every frame causes audio glitches, buffer underruns, and potential crashes.

### Playback Control

```c
snesmodPlay(0);      /* Play from start */
snesmodStop();       /* Stop playback */
snesmodPause();      /* Pause */
snesmodResume();     /* Resume from pause */
```

### Volume Control

```c
snesmodSetModuleVolume(127);    /* 0-127 */
snesmodFadeVolume(0, 4);        /* Target volume, speed */
```

---

## Creating Music

### Using OpenMPT (Recommended)

1. Download [OpenMPT](https://openmpt.org/) (free for Windows/Wine)
2. Create new Impulse Tracker (.it) module
3. Follow SNES constraints:
   - Maximum 8 channels
   - 32kHz sample rate
   - Total samples under 58KB
4. Export as `.it` file
5. Place in `music/` directory

### Build Process

The Makefile automatically:
1. Runs `smconv` on your `.it` file
2. Generates `soundbank.asm` and `soundbank.h`
3. Links the soundbank into the ROM

```makefile
SOUNDBANK_SRC := music/mymusic.it
```

---

## Generated Files

After building with smconv:

**soundbank.h:**
```c
#define MOD_POLLEN8     0
#define SOUNDBANK_BANK  0x02   /* ROM bank where soundbank lives */
```

**soundbank.asm:**
```asm
.SECTION ".soundbank" SUPERFREE
soundbank:
    .INCBIN "soundbank.bnk"
.ENDS
```

---

## Build and Run

```bash
cd examples/audio/6_snesmod_music
make clean && make

# Run in emulator
/path/to/Mesen music.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | UI and SNESMOD API calls |
| `Makefile` | Build config with SNESMOD rules |
| `music/pollen8.it` | Sample Impulse Tracker module |
| `soundbank.h` | Generated: module IDs, bank number |
| `soundbank.asm` | Generated: soundbank data |
| `soundbank.bnk` | Generated: compiled soundbank |

---

## Memory Layout

### ROM
```
Bank 0-1:  Code, tiles, tilemaps
Bank 2+:   Soundbank data (smconv output)
```

### SPC RAM
```
$0000-$00FF: Driver variables
$0200-$CFFF: Module data, samples
$D000-$FFFF: Echo buffer (if enabled)
```

---

## Exercises

### Exercise 1: Add Sound Effects
Use `snesmodPlayEffect()` to trigger sounds on button presses.

### Exercise 2: Multiple Modules
Add a second `.it` file and switch between them with SELECT button.

### Exercise 3: Echo Effects
Enable echo in your tracker module and hear the reverb.

---

## Common Issues

### No audio output
- Check `snesmodProcess()` is called every frame
- Verify `snesmodInit()` before `snesmodPlay()`
- Check emulator audio is enabled

### Music sounds wrong
- Verify module exports correctly from OpenMPT
- Check channel count (8 max)
- Verify sample rate (32kHz max)

### Build fails with smconv error
- Check `.it` file is valid Impulse Tracker format
- Verify smconv is built: `make -C $OPENSNES/bin smconv`

### Audio glitches
- Ensure `snesmodProcess()` is called EVERY frame
- Check for infinite loops blocking the main loop

---

## Technical Notes

### Why Tracker-Based Music?

Tracker modules are efficient for the SNES:
- Compact pattern data (repeatable)
- Samples shared across patterns
- Volume, panning, effects per-channel
- No streaming required

### smconv Options

```bash
smconv -s -o soundbank music/song.it
```
- `-s`: Generate assembly and header
- `-o`: Output basename
- Input: Impulse Tracker file(s)

---

## What's Next?

**Return to basics:** [Calculator](../../basics/1_calculator/) - Input handling

**Graphics:** [Animation](../../graphics/2_animation/) - Sprite animation

---

## Credits

SNESMOD driver by Mukunda Johnson.

Sample music "pollen8" - Public domain.

---

## License

Code: MIT
SNESMOD: See snesmod.org license
