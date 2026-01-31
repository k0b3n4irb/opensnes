# OpenSNES

A modern, open-source SDK for Super Nintendo (SNES) development.

## Overview

OpenSNES provides everything you need to create SNES games in C:

- **C Compiler (cc65816)**: QBE-based compiler targeting the 65816 CPU
- **Assembler (WLA-DX)**: Industry-standard assembler for 65816 and SPC700
- **Hardware Library**: Clean C API for PPU, input, DMA, sprites, text, and audio
- **Audio System**: Dual-mode audio with simple BRR driver and full SNESMOD tracker support
- **Asset Tools**: Graphics converter, font converter, soundbank converter
- **Build System**: Simple Makefile-based workflow

## Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/OpenSNES/opensnes.git
cd opensnes

# Build everything (compiler, tools, library)
make

# Build and run an example
cd examples/text/1_hello_world
make
# Open hello_world.sfc in your emulator (bsnes, Mesen, snes9x)
```

## Project Status

| Component | Status | Description |
|-----------|--------|-------------|
| Compiler (cc65816) | **Working** | cproc frontend + QBE w65816 backend |
| Assembler (WLA-DX) | **Working** | wla-65816, wla-spc700, wlalink |
| Library | **Working** | Console, input, sprites, text, DMA, audio |
| Audio (Legacy) | **Working** | Simple BRR sample playback, 8 voices |
| Audio (SNESMOD) | **Working** | Impulse Tracker modules, SFX, echo/reverb |
| Tools | **Working** | gfx4snes, font2snes, smconv |
| Test Suite | **Working** | Mesen2-based automated testing |

## Examples

| Category | Example | Description |
|----------|---------|-------------|
| Text | `1_hello_world` | Basic text rendering with console |
| Text | `2_custom_font` | Custom font loading and display |
| Basics | `1_calculator` | Arithmetic operations demo |
| Graphics | `2_animation` | Sprite animation |
| Audio | `1_tone` | Simple tone generation |
| Audio | `2_sfx` | Sound effects with BRR samples |
| Audio | `6_snesmod_music` | Tracker music with SNESMOD |

Build any example:
```bash
cd examples/audio/2_sfx
make
```

## Library API

```c
#include <snes.h>

int main(void) {
    // Initialize console for text output
    consoleInit();

    // Print text
    consolePrint("Hello SNES!");

    // Main loop
    while (1) {
        WaitForVBlank();

        // Read controller
        padUpdate();
        u16 pressed = padPressed(0);

        if (pressed & KEY_A) {
            // Play sound effect
            audioPlaySample(0, sample_data, sample_size, 0x1000);
        }
    }
}
```

### Available Modules

| Header | Functions |
|--------|-----------|
| `<snes/console.h>` | `consoleInit`, `consolePrint`, `consolePrintXY` |
| `<snes/input.h>` | `padUpdate`, `padPressed`, `padHeld`, `padReleased` |
| `<snes/sprite.h>` | `spriteInit`, `spriteSet`, `spriteUpdate` |
| `<snes/text.h>` | `textInit`, `textPrint`, `textPrintXY` |
| `<snes/dma.h>` | `dmaCopyVram`, `dmaCopyCgram`, `dmaFillVram` |
| `<snes/audio.h>` | `audioInit`, `audioPlaySample`, `audioStop` |
| `<snes/snesmod.h>` | `snesmodInit`, `snesmodPlay`, `snesmodPlayEffect` |

## Audio System

OpenSNES provides two audio modes, selectable at build time:

### Legacy Driver (default)
Simple BRR sample playback. Small footprint (~500 bytes).
```c
#include <snes/audio.h>
audioInit();
audioPlaySample(0, sample, size, pitch);
```

### SNESMOD (USE_SNESMOD=1)
Full tracker-based audio with Impulse Tracker support. 5.5KB driver.
```c
#include <snes/snesmod.h>
snesmodInit();
snesmodSetSoundbank(soundbank);
snesmodLoadModule(MOD_MUSIC);
snesmodPlay(0);
// Call every frame:
snesmodProcess();
```

## Build System

Examples use `make/common.mk` for standard build rules:

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := mygame.sfc
ROM_NAME := "MY GAME NAME         "  # 21 chars, pad with spaces

CSRC     := main.c
GFXSRC   := sprites.png             # Auto-converted to .h
USE_LIB  := 1                       # Link OpenSNES library

# For SNESMOD audio:
USE_SNESMOD   := 1
SOUNDBANK_SRC := music.it

include $(OPENSNES)/make/common.mk
```

## Tools

| Tool | Purpose |
|------|---------|
| `gfx4snes` | Convert PNG to SNES tiles, sprites, metasprites (from PVSnesLib) |
| `font2snes` | Convert font images to SNES format |
| `smconv` | Convert Impulse Tracker (.it) to SNESMOD soundbank |

## Testing

```bash
# Run all tests with Mesen2
./tests/run_tests.sh /path/to/Mesen

# Run specific test
cd /path/to/Mesen && ./Mesen /path/to/rom.sfc --testrunner --lua /path/to/test.lua
```

## Lineage

OpenSNES builds upon the SNES homebrew community's work:

- **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by Alekmaul - Original SNES C development kit
- **[QBE](https://c9x.me/compile/)** by Quentin Carbonneaux - Compiler backend
- **[WLA-DX](https://github.com/vhelin/wla-dx)** by Ville Helin - Assembler
- **[SNESMOD](https://github.com/mukunda-/snesmod)** by Mukunda Johnson - Tracker audio driver

See [ATTRIBUTION.md](ATTRIBUTION.md) for full credits.

## Contributing

1. Read [ATTRIBUTION.md](ATTRIBUTION.md) for guidelines
2. Check issues for "good first issue" labels
3. See [CLAUDE.md](CLAUDE.md) for development notes and debugging tips

## License

MIT License - See [LICENSE](LICENSE)
