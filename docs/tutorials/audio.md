# Audio & Music Tutorial {#tutorial_audio}

This tutorial covers SNES audio including the SPC700 sound chip, BRR samples, and SNESMOD tracker playback.

## SNES Audio Architecture

The SNES has a dedicated **SPC700** sound processor with:

- 8 audio channels
- 64KB dedicated audio RAM
- BRR (Bit Rate Reduction) sample compression
- Hardware ADSR envelopes
- Echo/reverb effects

## Audio Options in OpenSNES

| Method | Best For | Size | Complexity |
|--------|----------|------|------------|
| **Direct BRR samples** | One-shot SFX, voice clips | per sample | Low |
| **SNESMOD** | Music + tracker SFX | ~5.5KB | Medium |

Reach for **direct BRR samples** when you want a jump, a hit, a coin, or a
voice clip played on demand — you load a `.brr` into the SPC700 and trigger it
from C (see @ref examples_audio_soundboard). Reach for **SNESMOD** when you
want music, or SFX that share a tracker soundbank with it. The two can
coexist. To *make* a `.brr` from your own audio, see
[One-shot samples from WAV](#audio_wav2brr) below.

## SNESMOD (Recommended)

SNESMOD is a tracker-based audio engine supporting Impulse Tracker (.it) modules.

### Setup

1. Create music in a tracker (OpenMPT recommended)
2. Export as Impulse Tracker (.it) format
3. Convert with smconv tool
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

### One-shot samples from WAV (wav2brr) {#audio_wav2brr}

SNESMOD is built around tracker modules. For a plain one-shot effect — a jump,
a hit, a UI blip, a bit of speech — the lighter path is a **direct BRR
sample**: convert an audio file to the SNES's BRR format once, bake it into the
ROM, and trigger it from C. This is what @ref examples_audio_soundboard does.

**1. Convert your WAV to BRR.** `wav2brr` (built by `make tools`) turns a PCM
`.wav` into a `.brr` using the same encoder smconv uses on its own samples:

```sh
# one-shot effect
wav2brr res/jump.wav res/jump.brr

# a looping sample (loop between sample indices 2048 and 8192)
wav2brr --loop 2048 8192 res/cello.wav res/cello.brr
```

Input is PCM WAV (8- or 16-bit, mono or stereo — stereo is downmixed). Keep
the source at or below **32 kHz**; that is the DSP's ceiling, and a higher rate
just plays back sharp. Pass `-v` to see the block count, loop offset, and a
ready-to-paste `audioLoadSample()` line.

**2. Bake the `.brr` into the ROM.** Put it in a `data.asm` with a label and an
end label, exactly as the example does:

```asm
.section ".samples" superfree
brr_jump:     .incbin "res/jump.brr"
brr_jump_end:
.ends
```

**3. Load it once, play it on demand.** The label becomes a C symbol; the size
is the two labels subtracted, and the loop point is the byte offset `wav2brr`
reported (0 for a one-shot):

```c
extern u8 brr_jump[], brr_jump_end[];

audioLoadSample(0, brr_jump, (u16)(brr_jump_end - brr_jump), 0);
// ...later, when the player jumps:
audioPlaySample(0);
```

See @ref audio_samples in the API reference for the full sample API
(`audioLoadSample`, `audioPlaySample`, `audioUnloadSample`).

> A `.brr` is a build input, like a converted PNG — generate it with `wav2brr`
> and commit it next to your source WAV. There is no automatic `.wav` → `.brr`
> build step yet; run the tool when the source audio changes.

### Volume Control

```c
// Set master volume (0-127)
snesmodSetModuleVolume(127);

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

### smconv Tool

```bash
# Convert music for soundbank
smconv -s -o soundbank -b 1 music.it effects.it

# Output files:
# - soundbank.asm  (link with ROM)
# - soundbank.h    (module/effect IDs)
# - soundbank.bnk  (binary data)
```

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
| Echo Buffer | ~4KB (at $D000-$FFFF) |

Total audio RAM: 64KB

## Tips

1. **Keep samples small** - Use lower sample rates for less important sounds
2. **Reuse samples** - Same sample at different pitches for variety
3. **Test on hardware** - Emulator timing may differ
4. **Use echo sparingly** - Takes 4KB+ of audio RAM

## Examples

- `examples/audio/snesmod_music/` - Music playback demo
- `examples/audio/snesmod_sfx/` - Sound effects demo

## Next Steps

- @ref snesmod.h "SNESMOD API Reference"
- @ref tutorial_graphics "Back to Graphics"

## The audio module (v2) — samples and effects from pure C

For sound effects and sample playback, `LIB_MODULES += audio` gives
you the full engine with no SPC700 code of your own: the lib ships a
resident driver (built from source at lib build time) and `audio.h`'s
22 functions drive it — `audioInit()`, `audioLoadSample()` (BRR
streamed into APU RAM at runtime), `audioPlaySampleEx()` (volume/pan/
pitch, 8-voice round-robin polyphony), per-voice ADSR/GAIN, and a
configurable echo with FIR filter. Every call is bounded — the API
returns `AUDIO_ERR_TIMEOUT` rather than hanging. Worked example:
`audio/soundboard`. Main-thread only; one engine per ROM (don't link
`audio` and `snesmod` together).

Choosing a path: **snesmod** for tracker music (IT modules),
**audio** for C-driven samples and DSP effects, **apu** (below) for
writing your own SPC700 program.

## The raw APU path (no snesmod)

Since the SPC700 arc, the SDK has a second audio path: the `apu` module
uploads a wla-spc700-assembled program straight through the IPL boot-ROM
protocol — `apuWaitBoot()`, `apuUpload()`, `apuExecute()` — giving full
DSP control (voices, ADSR, echo, pitch) with no tracker involved. Worked
example: `audio/speech_synth` (phoneme-bank speech — upload, DSP
config, per-phoneme sequencing). The two paths are exclusive:
don't link `apu` and `snesmod` in one ROM.

APU-side memory layout matters: the flat binary is laid out by
`wlalink -b`, and an `.ORG` section that overlaps your growing code
OVERWRITES it silently (the SPC700-arc sequencer died exactly this way during development).
Budget the code page before placing the sample directory.

### Hot-swapping APU programs

`apuWaitBoot()` only works once — the IPL handshake is consumed at
boot. To replace the running program later, use the cooperative reset
protocol (worked example: `audio/apu_switch`):

1. Build the APU program with `APU_CHECK_RESET` (from
   `templates/memmap_spc700.inc`) inside its wait loops. The idiom
   polls CPUIO0 for `APU_RESET_MAGIC`; on match it silences the DSP,
   acks, re-enables the IPL ROM and jumps to `$FFC0` — the boot ROM is
   re-entrant.
2. On the 65816, call `apuReset()`, then `apuUpload()`/`apuExecute()`
   for the next program — but NOT `apuWaitBoot()` (the reset already
   consumed the ready signal).

Two contract points, both learned the hard way in `apu_switch`:
`apuReset()` blocks forever on a program that never polls for the
magic, and the next program receives the DSP *dirty* — a residual
`ADSR1` bit 7 from the previous program silently overrides `GAIN`, so
every register a voice depends on must be written explicitly.

## Debugging audio: luna's spc-dump

`luna spc-dump` runs a ROM and exports the complete APU state — 64 KB of
ARAM plus all 128 DSP registers — as a standard playable `.spc`. Diffing
two dumps (yours vs a reference, or two instants of your own ROM) answers
in seconds what ears cannot localize: upload integrity, directory/loop
addresses, per-voice ADSR/pitch/envelope state, phoneme/note schedules.
`--audio-out` (WAV capture) complements it for spectral verification.
