# Getting Started with OpenSNES {#getting_started}

This guide will get you from zero to running your first SNES ROM in about 10 minutes.

## Choose Your Path

| | **I want to make SNES games** | **I want to contribute to the SDK** |
|---|---|---|
| **What** | Download the pre-built SDK, write C code, build ROMs | Clone the repo, modify compiler/library/tools |
| **Prerequisites** | `make` + text editor | clang, cmake, git, python3 |
| **Time to start** | ~5 minutes | ~15 minutes |
| **Go to** | [Path A: Game Developer](#path-a-game-developer) | [Path B: SDK Developer](#path-b-sdk-developer) |

---

## Path A: Game Developer

You want to write SNES games in C. The SDK is already compiled — you just need
to download it, write code, and run `make`.

### A1. Install Prerequisites

You only need `make` (the build tool) and an emulator. No compiler installation
required — the SDK ships with its own cross-compiler.

**macOS:**
```bash
xcode-select --install
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install make
```

**Linux (Fedora):**
```bash
sudo dnf install make
```

**Windows:**
1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal
3. Run: `pacman -S make`

### A2. Get an Emulator

Pick one (Mesen is recommended for debugging):

| Emulator | Best For | Download |
|----------|----------|----------|
| [Mesen](https://www.mesen.ca/) | Debugging, accuracy | mesen.ca |
| [bsnes](https://github.com/bsnes-emu/bsnes) | Cycle accuracy | GitHub releases |
| [Snes9x](https://www.snes9x.com/) | Performance | snes9x.com |

### A3. Download OpenSNES SDK

Download the latest release for your platform from the
[GitHub Releases page](https://github.com/k0b3n4irb/opensnes/releases):

| Platform | File |
|----------|------|
| Linux x86_64 | `opensnes_<version>_linux_x86_64.zip` |
| Linux aarch64 | `opensnes_<version>_linux_aarch64.zip` |
| macOS arm64 | `opensnes_<version>_darwin_arm64.zip` |
| Windows x86_64 | `opensnes_<version>_windows_x86_64.zip` |

Extract the archive somewhere permanent (e.g., `~/opensnes` or `C:\opensnes`).

### A4. Run Your First ROM

The SDK comes with pre-built example ROMs:

```bash
cd opensnes/examples/text/hello_world

# Open in your emulator
mesen hello_world.sfc        # Linux
open -a Mesen hello_world.sfc  # macOS
start Mesen.exe hello_world.sfc  # Windows
```

You should see "Hello World!" on screen.

### A5. Create Your Own Project

Create a new directory anywhere on your machine:

```bash
mkdir ~/my-snes-game
cd ~/my-snes-game
```

Create two files:

**Makefile:**
```makefile
# Point to your OpenSNES installation
OPENSNES := /path/to/opensnes

# ROM settings
TARGET   := my_game.sfc
ROM_NAME := MY GAME

# Source files
CSRC     := main.c

# Use OpenSNES library
USE_LIB  := 1
LIB_MODULES := console sprite dma background input

# Include the build system
include $(OPENSNES)/make/common.mk
```

**main.c:**
```c
#include <snes.h>

int main(void) {
    consoleInit();
    consoleDrawText(8, 10, "Hello SNES!");
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
```

Build and run:
```bash
make
mesen my_game.sfc
```

That's it — you're making SNES games.

### Project Structure

```
my-snes-game/
├── Makefile        # Build configuration
├── main.c          # Your game code
└── res/            # Assets (optional)
    ├── tiles.png
    └── music.it
```

---

## Path B: SDK Developer

You want to modify the compiler, library, tools, or build system itself.
This requires building the entire SDK from source.

### B1. Install Prerequisites

You need a full C/C++ development environment.

**macOS:**
```bash
xcode-select --install
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install build-essential clang cmake make git python3
```

**Linux (Fedora):**
```bash
sudo dnf install clang cmake make git python3
```

**Windows:**
1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal
3. Run:
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-cmake base-devel git python
```

### B2. Clone and Build

```bash
# Clone with submodules (--recursive is required!)
git clone --recursive https://github.com/k0b3n4irb/opensnes.git
cd opensnes

# Build everything: compiler → tools → library → examples
make
```

This takes a few minutes. Expected output:
```
Building cc65816 compiler...
Building WLA-DX assembler...
Building OpenSNES library...
Building examples... (52 ROMs)
OpenSNES SDK build complete!
```

### B3. Run Tests

```bash
make tests   # luna: coverage + visual regression + probes
```

### B4. Development Workflow

```bash
make clean && make     # Full rebuild (required after compiler changes)
make lib               # Rebuild library only
make examples          # Rebuild examples only
make -C examples/text/hello_world  # Rebuild one example
```

See [CLAUDE.md](../CLAUDE.md) for architecture details and coding conventions.

---

## What's Next?

Explore by complexity:

| Level | Examples | What You'll Learn |
|-------|----------|-------------------|
| **Beginner** | `text/hello_world`, `text/text_test` | Console output, text formatting |
| **Intermediate** | `graphics/sprites/simple_sprite`, `input/two_players` | Sprites, controller input |
| **Advanced** | `graphics/backgrounds/mode7`, `audio/snesmod_music` | Mode 7, tracker music |
| **Expert** | `games/breakout`, `games/likemario` | Complete game structure |
| **SA-1 Coprocessor** | `memory/sa1_hello`, `memory/sa1_starfield` | 10.74 MHz second CPU ([tutorial](tutorials/sa1.md)) |
| **SuperFX (GSU)** | `memory/superfx_hello`, `memory/superfx_3d` | RISC coprocessor, 3D rendering ([tutorial](tutorials/superfx.md)) |

Browse all examples:
```bash
ls examples/*/
```

## Tutorials

| Topic | Guide |
|-------|-------|
| Graphics & Backgrounds | [tutorials/graphics.md](tutorials/graphics.md) |
| Sprites & Animation | [tutorials/sprites.md](tutorials/sprites.md), [tutorials/animation.md](tutorials/animation.md) |
| Scrolling & Parallax | [tutorials/scrolling.md](tutorials/scrolling.md) |
| Collision Detection | [tutorials/collision.md](tutorials/collision.md) |
| Input Handling | [tutorials/input.md](tutorials/input.md) |
| Audio & Music | [tutorials/audio.md](tutorials/audio.md) |
| Game States | [tutorials/game_states.md](tutorials/game_states.md) |
| SA-1 Coprocessor | [tutorials/sa1.md](tutorials/sa1.md) |

## Troubleshooting

### "command not found: make"

Install build tools (see prerequisites for your path above).

### "fatal: repository not found" or empty compiler folder

You forgot `--recursive` when cloning. Fix it:
```bash
git submodule update --init --recursive
```

### Build fails with "Library not built"

Run `make` from the SDK root first — the library must be compiled before examples:
```bash
cd /path/to/opensnes && make lib
```

### Black screen when running ROM

Your ROM built but doesn't display anything. Common causes:
1. Missing `setScreenOn()` call
2. Missing `WaitForVBlank()` in main loop
3. Wrong `LIB_MODULES` — check that you include all needed modules

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more solutions.

### Build fails with "unhandled op" or assembly errors

This is usually a compiler limitation. Check:
- Are you using `u32`/`s32`? Prefer `u16`/`s16` when possible
- Use `u8`, `u16`, `s16`, `u32` types from `snes.h` (not `int` or `long`)

## Getting Help

- **Issues**: [github.com/k0b3n4irb/opensnes/issues](https://github.com/k0b3n4irb/opensnes/issues)
- **SNES Dev Wiki**: [snes.nesdev.org](https://snes.nesdev.org/)
