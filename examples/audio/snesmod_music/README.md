# SNESMOD Music Example


Full music playback using SNESMOD, a tracker-based audio engine for the SNES.

## Learning Objectives

After this lesson, you will understand:
- How SNESMOD works (module-based music)
- Loading soundbanks and modules
- Music playback controls (play, stop, pause, fade)
- Volume control
- How the build turns an `.it` file into a soundbank (`opensnes-music`)

## Prerequisites

- Basic understanding of SNES audio concepts
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
| Font tiles | text module (`textLoadFont`) |
| Text rendering | text module (`textPrintAt`, `textFlush`) |

---

## SNESMOD Overview

SNESMOD is a tracker-based music playback system:

```
Impulse Tracker file (.it)
         |
         v
    [opensnes-music bank]
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

**Warning:** `snesmodProcess()` is what sends the commands (play, stop, volume, sound effects) to the SPC700. Without it they wait in a queue of 85 and the newest are dropped once it is full; the music already playing goes on.

### Playback Control

```c
snesmodPlay(0);      /* Play from start */
snesmodStop();       /* Stop playback */
snesmodPause();      /* Pause */
snesmodResume();     /* Resume from pause */
```

### Volume Control

```c
snesmodSetModuleVolume(255);    /* 0-255, 255 at load */
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

When the Makefile sets `SOUNDBANK_SRC` (with `USE_SNESMOD := 1`), the build:
1. Runs `opensnes-music bank` on your `.it` file(s)
2. Writes `soundbank.bnk`, `soundbank.asm` and `soundbank.h`
3. Links the soundbank into the ROM

```makefile
SOUNDBANK_SRC := music/mymusic.it
```

---

## Generated Files

After a build:

**soundbank.h** (excerpt):
```c
#define MOD_POLLEN8                     	0
#define MOD_POLLEN8_SIZE                	44137
#define SFX_SAW1                        	0
...
#define SOUNDBANK_BANK 1
```

**soundbank.asm:** the bank is given to the soundbank alone, at its start:
```asm
.BANK 1
.ORG 0
.SECTION "SOUNDBANK" FORCE ; need dedicated bank(s)

soundbank:
.incbin "./soundbank.bnk"
.ENDS
```

---

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/audio/snesmod_music
```

Then run `music.sfc` in luna (`opensnes run`) or any SNES emulator.

---

## Modules Used

`console`, `sprite`, `dma`, `input`, `background`, `text` (`LIB_MODULES` in the Makefile).

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
Bank 0:    Code
Bank 1:    Soundbank data (its own bank, at its start; more banks if it is larger)
Others:    Tiles, tilemaps and other const data (the asset banks)
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

### Build fails converting the module
- Check `.it` file is valid Impulse Tracker format
- `opensnes-music inspect music/song.it` says what the converter reads in it

### A press does nothing, or late
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

### Converting by hand

```bash
opensnes-music bank -o . --name soundbank music/song.it
```
- `--name`: base name of the files written
- `-o`: output directory
- `opensnes-music --help` lists the rest

---

## What's Next?

**Sound Effects:** [SNESMOD SFX](../snesmod_sfx/) - Sound effect playback with pitch control

**Graphics:** [Simple Sprite](../../sprites/simple_sprite/) - Display a sprite on screen

---

## Credits

SNESMOD driver by Mukunda Johnson.

Sample music "pollen8" - Public domain.

---

## License

Code: MIT
SNESMOD: See snesmod.org license
