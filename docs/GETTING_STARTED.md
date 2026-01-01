# Getting Started with OpenSNES

> **Note**: OpenSNES is in early development. This guide will expand as features are added.

## Prerequisites

- GCC or Clang (for building tools)
- Make
- Git
- Mesen2 emulator (for testing)

## Installation

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/opensnes.git
cd opensnes

# Set environment variable
export OPENSNES_HOME=$(pwd)

# Build the SDK (when ready)
make
```

## Your First Project

```bash
# Copy a template
cp -r templates/platformer my-game
cd my-game

# Build
make

# Run in emulator
mesen my-game.sfc
```

## Project Structure

```
my-game/
├── Makefile        # Build configuration
├── hdr.asm         # ROM header
├── data.asm        # Data includes
└── src/
    └── main.c      # Your game code
```

## Next Steps

- Read the [API Reference](API.md)
- Explore the [Templates](../templates/)
- Check out [SNES Hardware Overview](HARDWARE.md)

## Getting Help

- GitHub Issues: [opensnes/issues](https://github.com/YOUR_USERNAME/opensnes/issues)
- SNESdev Discord: [discord.gg/snesdev](https://discord.gg/snesdev)
