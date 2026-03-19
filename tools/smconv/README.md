# smconv

Converts Impulse Tracker (.it) music files to SNESMOD soundbank format for the SPC700 audio processor.

SNESMOD is the audio engine used by OpenSNES. It plays music and sound effects on the SNES's dedicated Sony SPC700 sound chip. smconv handles the conversion from standard tracker format to the SPC700-compatible binary.

## Usage

### Create a soundbank (for use in games)

```bash
smconv -s -o soundbank -b 1 -n -p soundbank music.it
```

This generates:
- `soundbank.asm` — Assembly include for the soundbank data
- `soundbank.h` — C header with module/effect constants
- `soundbank.brr` — BRR-encoded sample data

### Multiple music + SFX files

```bash
smconv -s -o soundbank -b 1 -n -p soundbank overworld.it battle.it jump_sfx.it
```

Each `.it` file becomes a separate module. Music files are played with `spcLoad(N)` + `spcPlay(0)`, effects with `spcPlaySound(N)`.

### Create standalone SPC file (for testing)

```bash
smconv music.it
```

Outputs an `.spc` file playable in SPC players (no SNES ROM needed).

## Options

| Flag | Description |
|------|-------------|
| `-s` | Soundbank creation mode (default: SPC mode) |
| `-o FILE` | Output file or base name (required for soundbank mode) |
| `-b N` | ROM bank number for soundbank data (default: 5, OpenSNES uses 1) |
| `-n` | Skip `.include "hdr.asm"` in output (use for OpenSNES) |
| `-p NAME` | Symbol prefix (default: `SOUNDBANK__`) |
| `-i` | Use HiROM mapping mode |
| `-f` | Check IT file sizes against first file (for SFX banks) |
| `-V` | Verbose output |
| `-h` | Show help |
| `-v` | Show version |

## Makefile Integration

```makefile
USE_SNESMOD    := 1
SOUNDBANK_SRC  := res/music.it res/sfx.it
```

The `common.mk` build system handles the smconv invocation automatically when `USE_SNESMOD` is set.

## In Your Code

```c
#include <snes/snesmod.h>
#include "soundbank.h"

// Initialize audio
spcBoot();
spcSetBank(&SOUNDBANK__);

// Play music
spcLoad(0);          // Load first module
spcPlay(0);          // Start playback

// In main loop
while (1) {
    WaitForVBlank();
    spcProcess();    // Feed SPC700 with data
}

// Play sound effect
spcPlaySound(1);     // Play effect from second .it file
```

## Input Format

- Impulse Tracker (.it) files
- Sample rate: auto-converted to SPC700 BRR format (32000 Hz max)
- Channels: up to 8 (SPC700 limit)
- Effects: most standard IT effects supported (volume, panning, portamento, vibrato)

## Attribution

Based on SNESMOD by Mukunda Johnson. License: MIT-compatible.
