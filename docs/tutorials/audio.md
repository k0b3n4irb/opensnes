# Music with SNESMOD {#tutorial_audio}

Choose this page for tracker music: SNESMOD plays Impulse Tracker modules,
and plays sound effects from the same soundbank. For samples driven from C
with DSP effects (ADSR, GAIN, echo), or for an SPC700 program of your own,
read @ref tutorial_audio_samples instead.

## SNES Audio Architecture

The SNES has a dedicated **SPC700** sound processor with:

- 8 audio channels
- 64KB dedicated audio RAM
- BRR (Bit Rate Reduction) sample compression
- Hardware ADSR envelopes
- Echo/reverb effects

## Two engines, one per ROM

| Module | Best for | Size in audio RAM |
|--------|----------|-------------------|
| `snesmod` (this page) | Music, and effects that share its soundbank | ~5.5KB driver |
| `audio` (@ref tutorial_audio_samples) | One-shot effects, voice clips, DSP effects from C | per sample |

Pick one per ROM: `audio` and `snesmod` both own the SPC700, so they are
never linked together. A one-shot `.brr` made from a WAV belongs to the
`audio` module: see @ref audio_wav2brr.

## SNESMOD

SNESMOD is a tracker-based audio engine supporting Impulse Tracker (.it) modules.

### Setup

1. Create music in a tracker (OpenMPT recommended)
2. Export as Impulse Tracker (.it) format
3. List the modules in `SOUNDBANK_SRC`: the build converts them
4. Link soundbank with your ROM

### Makefile Configuration

```makefile
# Enable SNESMOD
USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite input

# Soundbank source files
SOUNDBANK_SRC  := music/mymusic.it sfx/effects.it
```

### Basic Music Playback

```c
#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"

int main(void) {
    consoleInit();

    // Initialize SNESMOD (uploads SPC driver)
    snesmodInit();

    // Set soundbank location
    snesmodSetSoundbank(SOUNDBANK_BANK);

    // Load a module (defined in soundbank.h)
    snesmodLoadModule(MOD_MYMUSIC);

    // Start playback
    snesmodPlay(0);  // 0 = start from beginning

    setScreenOn();

    while (1) {
        WaitForVBlank();

        // MUST call every frame!
        snesmodProcess();
    }
    return 0;
}
```

### Sound Effects

```c
#include <snes/snesmod.h>
#include "soundbank.h"

// Load effects at startup
void init_audio(void) {
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);

    // Load sound effects (indices from soundbank.h)
    snesmodLoadEffect(0);  // Jump sound
    snesmodLoadEffect(1);  // Coin sound
    snesmodLoadEffect(2);  // Hit sound
}

// Play effect when needed
void play_jump_sound(void) {
    // snesmodPlayEffect(effectId, volume, pan, pitch)
    snesmodPlayEffect(0, 127, 128, SNESMOD_PITCH_NORMAL);
    // volume: 0-127
    // pan: 0=left, 128=center, 255=right
    // pitch: SNESMOD_PITCH_LOW/NORMAL/HIGH (4/8/12)
}

void play_coin_sound(void) {
    // Higher pitch for coins
    snesmodPlayEffect(1, 100, 128, SNESMOD_PITCH_HIGH);
}
```

### Volume Control

```c
// Set module volume (0-255; a module starts at 255 — 127 is half)
snesmodSetModuleVolume(255);

// Fade out over time
snesmodFadeVolume(0, 4);  // target=0, speed=4

// Pause/Resume
snesmodPause();
snesmodResume();

// Stop completely
snesmodStop();
```

### Creating Music with OpenMPT

1. Download [OpenMPT](https://openmpt.org/) (free)
2. Create new Impulse Tracker module
3. Keep within SNES limits:
   - Max 8 channels
   - Total samples < 58KB
   - 32kHz max sample rate
4. Export as .it file

### The converter

The build runs this for you; by hand it is:

```bash
opensnes-music bank music.it effects.it --name soundbank --bank 1

# Output files:
# - soundbank.asm  (link with ROM)
# - soundbank.h    (module/effect IDs)
# - soundbank.bnk  (binary data)
```

The build runs it from `SOUNDBANK_SRC` when `USE_SNESMOD := 1`
(@ref tools_opensnes_music), and `opensnes-music inspect`
tells you what each module costs in the SPC700's 64 KB.

## Pitch Constants

```c
#define SNESMOD_PITCH_LOW    4   // 16kHz - lower pitch
#define SNESMOD_PITCH_NORMAL 8   // 32kHz - normal pitch
#define SNESMOD_PITCH_HIGH   12  // 48kHz - higher pitch
```

## Important: Call snesmodProcess()!

**You MUST call `snesmodProcess()` every frame!** Failure to do so causes:
- Audio glitches and stuttering
- Command buffer overflow
- Desynchronization

```c
while (1) {
    WaitForVBlank();
    snesmodProcess();  // CRITICAL!

    // ... game logic
}
```

## Following the music and managing the queue

`snesmodGetPosition()` returns the playback position the driver reports
(the current row), for syncing game events to the music:

```c
if (snesmodGetPosition() == 32) {
    start_boss_flash();              /* on the beat of row 32 */
}
```

Every `snesmod*` command goes through a queue that `snesmodProcess()`
drains a little each frame. `snesmodFlush()` blocks until the queue is
empty, for the rare moment you need every command applied now, such as
before loading another module.

The queue holds 85 commands and the SPC700 takes about two per frame. With
several queued, `snesmodProcess()` waits on the audio CPU for up to five
scanlines (about 2 % of a frame) before leaving the rest to the next frame.
A command sent while the queue is full is dropped, so a burst of dozens of
effects in one frame loses the last ones rather than corrupting the queue.

## Example: Music + SFX

```c
#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"

int main(void) {
    u16 pad, pad_prev = 0, pad_pressed;

    consoleInit();

    // Initialize audio
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);

    // Load music module
    snesmodLoadModule(MOD_LEVEL1);

    // Load sound effects
    snesmodLoadEffect(0);  // Jump
    snesmodLoadEffect(1);  // Coin

    // Start music
    snesmodPlay(0);

    setScreenOn();

    while (1) {
        WaitForVBlank();
        snesmodProcess();

        // Read input
        while (REG_HVBJOY & 0x01) {}
        u16 pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_pressed = pad & ~pad_prev;
        pad_prev = pad;
        if (pad == 0xFFFF) pad_pressed = 0;

        // Play SFX on button press
        if (pad_pressed & KEY_A) {
            snesmodPlayEffect(0, 127, 128, SNESMOD_PITCH_NORMAL);
        }
        if (pad_pressed & KEY_B) {
            snesmodPlayEffect(1, 127, 128, SNESMOD_PITCH_HIGH);
        }
    }
    return 0;
}
```

## Memory Usage

| Component | Size |
|-----------|------|
| SPC700 Driver | ~5.5KB |
| Sample Data | Up to ~58KB |
| Echo Buffer | 2 KB per unit of delay, under $FF00 (none without an echo) |

Total audio RAM: 64KB

## Tips

1. **Keep samples small** - Use lower sample rates for less important sounds
2. **Reuse samples** - Same sample at different pitches for variety
3. **Test on hardware** - Emulator timing may differ
4. **Size the echo to what you hear** - its ring takes 2 KB of audio RAM per 16 ms of delay

## Example ROMs

- `examples/audio/snesmod_music/` - Music playback demo
- `examples/audio/snesmod_sfx/` - Sound effects demo

## Next Steps

- @ref tutorial_audio_samples "Samples, voices and the raw APU" — the `audio` and `apu` modules, and luna's `spc-dump` for debugging either engine
- @ref snesmod.h "SNESMOD API Reference"
- @ref tutorial_graphics "Back to Graphics"
