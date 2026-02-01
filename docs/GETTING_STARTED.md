# Getting Started with OpenSNES

This guide will get you from zero to running your first SNES ROM in about 10 minutes.

## What You'll Need

- **C programming basics** - if you can write a for loop, you're ready
- **Curiosity** - the SNES is a fascinating machine
- **An emulator** - to test your ROMs

## Step 1: Install Prerequisites

### macOS

```bash
xcode-select --install
```

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential clang cmake make git python3
```

### Linux (Fedora)

```bash
sudo dnf install clang cmake make git python3
```

### Windows

1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal (not the regular MSYS2 terminal)
3. Run:

```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-cmake base-devel git python
```

## Step 2: Get an Emulator

Pick one (Mesen is recommended for debugging):

| Emulator | Best For | Download |
|----------|----------|----------|
| [Mesen](https://www.mesen.ca/) | Debugging, accuracy | mesen.ca |
| [bsnes](https://github.com/bsnes-emu/bsnes) | Cycle accuracy | GitHub releases |
| [Snes9x](https://www.snes9x.com/) | Performance | snes9x.com |

## Step 3: Clone and Build OpenSNES

```bash
# Clone with submodules (--recursive is required!)
git clone --recursive https://github.com/OpenSNES/opensnes.git
cd opensnes

# Build everything
make
```

This builds the compiler, tools, library, and all 30 example ROMs.

**Expected output:**
```
Building cc65816 compiler...
Building WLA-DX assembler...
Building OpenSNES library...
Building examples... (30 ROMs)
```

## Step 4: Run Your First ROM

```bash
# Navigate to hello world example
cd examples/text/1_hello_world

# Open the ROM in your emulator
# macOS:
open -a Mesen hello_world.sfc
# Linux:
mesen hello_world.sfc
# Windows:
start Mesen.exe hello_world.sfc
```

You should see "Hello World!" on screen.

## Step 5: Create Your Own Project

The easiest way is to copy an existing example:

```bash
# From the opensnes root directory
cp -r examples/text/1_hello_world ~/my-snes-game
cd ~/my-snes-game

# Edit the Makefile to change the ROM name
# Edit main.c to change the code
# Build
export OPENSNES_HOME=/path/to/opensnes
make
```

### Minimal Project Structure

```
my-snes-game/
├── Makefile        # Build configuration
├── main.c          # Your game code
└── res/            # Assets (optional)
    ├── tiles.png
    └── music.it
```

### Minimal Makefile

```makefile
# Point to your OpenSNES installation
OPENSNES := /path/to/opensnes

# ROM settings
TARGET   := my_game.sfc
ROM_NAME := "MY GAME              "   # Exactly 21 characters!

# Source files
CSRC     := main.c

# Use OpenSNES library
USE_LIB  := 1
LIB_MODULES := console

# Include the build system
include $(OPENSNES)/make/common.mk
```

### Minimal main.c

```c
#include <snes.h>

int main(void) {
    // Initialize the console (sets up PPU, clears screen)
    consoleInit();

    // Print text at position (8, 10)
    consoleDrawText(8, 10, "Hello SNES!");

    // Turn on the screen (it starts off by default)
    setScreenOn();

    // Main game loop
    while (1) {
        WaitForVBlank();  // Wait for vertical blank (60fps on NTSC)
    }

    return 0;
}
```

## What's Next?

Now that you have a working setup, explore by complexity:

| Level | Examples | What You'll Learn |
|-------|----------|-------------------|
| **Beginner** | `text/1_hello_world`, `basics/1_calculator` | Console output, basic setup |
| **Intermediate** | `graphics/2_animation`, `input/1_joypad` | Sprites, controller input |
| **Advanced** | `graphics/10_mode7`, `audio/6_snesmod_music` | Mode 7, tracker music |
| **Expert** | `game/1_breakout`, `game/2_entity_demo` | Complete game structure |

Browse all examples:
```bash
ls -la examples/*/
```

## Troubleshooting

### "command not found: make"

Install build tools (see Step 1).

### "fatal: repository not found" or empty compiler folder

You forgot `--recursive`. Fix it:
```bash
git submodule update --init --recursive
```

### "OPENSNES_HOME not set" or "No rule to make target"

Set the environment variable:
```bash
export OPENSNES_HOME=/path/to/opensnes
```

Add to your shell profile (`~/.bashrc` or `~/.zshrc`) to make it permanent.

### Black screen when running ROM

Your ROM built but doesn't display anything. Common causes:
1. Missing `setScreenOn()` call
2. VBlank loop issues
3. Memory overlap (run `python3 tools/symmap/symmap.py --check-overlap game.sym`)

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more solutions.

### Build fails with "unhandled op" or assembly errors

This is usually a compiler limitation. Check:
- Are you using `u32`/`s32`? Prefer `u16`/`s16` when possible
- Static variables with initializers? Use `static u8 x;` not `static u8 x = 0;`

## Getting Help

- **Issues**: [github.com/OpenSNES/opensnes/issues](https://github.com/OpenSNES/opensnes/issues)
- **SNES Dev Wiki**: [snes.nesdev.org](https://snes.nesdev.org/)
- **Discord**: Search for "SNES Development" communities
