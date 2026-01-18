# Getting Started with OpenSNES {#getting_started}

This guide covers installing OpenSNES and building your first SNES ROM.

## Prerequisites

### macOS

```bash
# Install Xcode Command Line Tools
xcode-select --install
```

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential gcc cmake make git
```

### Windows (MSYS2)

1. Download and install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal
3. Install the toolchain:

```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake base-devel git
```

### SNES Emulator

For testing your ROMs, install one of:
- [Mesen](https://www.mesen.ca/) (recommended, has debugging)
- [bsnes](https://github.com/bsnes-emu/bsnes)
- [Snes9x](https://www.snes9x.com/)

## Installation

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/opensneskd/opensnes.git
cd opensnes

# Build everything (compiler, tools, library, examples)
make
```

## Your First Project

```bash
# Build and run the hello world example
cd examples/text/1_hello_world
make

# Open hello_world.sfc in your emulator
```

## Creating a New Project

```bash
# Create project directory
mkdir my-game
cd my-game

# Create Makefile
cat > Makefile << 'EOF'
TARGET     := my_game
ROM_NAME   := MY GAME
USE_LIB    := 1
LIB_MODULES := console

include $(OPENSNES_HOME)/make/common.mk
EOF

# Create main.c
cat > main.c << 'EOF'
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
EOF

# Build
export OPENSNES_HOME=/path/to/opensnes
make
```

## Project Structure

```
my-game/
├── Makefile        # Build configuration (includes common.mk)
├── main.c          # Your game code
├── data.asm        # Graphics/audio data (optional)
└── res/            # Asset files (optional)
    ├── tiles.png
    └── music.it
```

## Next Steps

- @ref tutorial_graphics "Graphics Tutorial"
- @ref tutorial_sprites "Sprites Tutorial"
- @ref tutorial_input "Input Tutorial"
- @ref tutorial_audio "Audio Tutorial"

## Getting Help

- GitHub Issues: [opensnes/issues](https://github.com/opensnes/opensnes/issues)
- SNES Development Wiki: [snes.nesdev.org](https://snes.nesdev.org/)
