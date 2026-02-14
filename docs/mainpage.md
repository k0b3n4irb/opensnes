# OpenSNES Documentation {#mainpage}

Welcome to the **OpenSNES** documentation - a modern, open-source SDK for Super Nintendo (SNES) development.

## Acknowledgments

**OpenSNES would not exist without [PVSnesLib](https://github.com/alekmaul/pvsneslib).**

This project is deeply built upon the incredible work of **Alekmaul** and all the PVSnesLib contributors who have spent years creating a comprehensive SNES development kit. We owe them an enormous debt of gratitude for:

- The complete hardware abstraction library
- SNESMOD audio engine integration
- Asset conversion tools (gfx4snes, smconv)
- Decades of SNES development knowledge
- 80+ working examples that taught us how SNES development works

**Thank you to the entire PVSnesLib community.**

## How OpenSNES Differs from PVSnesLib

OpenSNES is a fork that focuses on **modernizing the compiler toolchain** while keeping the proven library and tools:

| Component | PVSnesLib | OpenSNES |
|-----------|-----------|----------|
| **C Compiler** | 816-tcc (based on TinyCC) | cc65816 (cproc + QBE backend) |
| **Assembler** | WLA-DX | WLA-DX (same) |
| **Library** | PVSnesLib | Based on PVSnesLib |
| **Tools** | gfx4snes, smconv, etc. | Same tools (gfx4snes from PVSnesLib) |

### Why a New Compiler?

The 816-tcc compiler in PVSnesLib, while functional, has limitations. OpenSNES uses **cc65816**, a new compiler built on:

- **cproc** - A clean, modern C11 compiler frontend
- **QBE** - An optimizing backend, with a custom 65816 target

This provides:
- Better C11 standard compliance
- Cleaner codebase for maintenance
- Foundation for future optimizations
- Direct WLA-DX assembly output (no post-processing)

### What Stays the Same?

- The excellent PVSnesLib library API
- Proven asset tools
- SNESMOD audio system
- WLA-DX assembler

OpenSNES aims to be a **companion project** that explores compiler improvements while respecting and building upon PVSnesLib's solid foundation.

## What is OpenSNES?

OpenSNES provides everything you need to create SNES games in C:

- **C Compiler** (cc65816) - Write games in C, not just assembly
- **Assembler** (WLA-DX) - Full 65816 and SPC700 support
- **Library** - Hardware abstraction for graphics, audio, input
- **Tools** - Asset converters for graphics and audio
- **Examples** - 11 working example ROMs to learn from

## Quick Start

### Prerequisites

- **macOS**: Xcode Command Line Tools
- **Linux**: GCC, Make, CMake
- **Windows**: MSYS2 with MinGW-w64 toolchain (native, no WSL required)
- Git
- A SNES emulator (Mesen, bsnes, Snes9x)

### Build the SDK

```bash
git clone --recursive https://github.com/user/opensnes.git
cd opensnes
make
```

### Create Your First ROM

```bash
cd examples/text/hello_world
make
# Open hello_world.sfc in your emulator
```

## Documentation Sections

### Getting Started
- @ref getting_started "Installation & Setup"
- @ref example_walkthroughs "Example Walkthroughs"

### API Reference
- @ref snes.h "Main Header (snes.h)"
- @ref console.h "Console Functions"
- @ref sprite.h "Sprite/OAM Functions"
- @ref input.h "Input Handling"
- @ref audio.h "Legacy Audio"
- @ref snesmod.h "SNESMOD Tracker Audio"
- @ref mode7.h "Mode 7 Graphics"
- @ref dma.h "DMA Transfers"
- @ref registers.h "Hardware Registers"

### Tutorials
- @ref tutorial_graphics "Graphics & Backgrounds"
- @ref tutorial_sprites "Sprites & Animation"
- @ref tutorial_input "Controller Input"
- @ref tutorial_audio "Audio & Music"

### Contributing
- @ref contributing "Contributing Guide"
- @ref code_style "Code Style Guide"

## Examples Overview

| Category | Example | Description |
|----------|---------|-------------|
| Text | hello_world | Basic text output |
| Text | custom_font | Custom font loading |
| Basics | calculator | Arithmetic operations |
| Graphics | animation | Sprite animation |
| Graphics | mode7 | Rotation & scaling |
| Graphics | scrolling | Parallax scrolling |
| Audio | tone | Simple tone generation |
| Audio | sfx | BRR sample playback |
| Audio | snesmod_music | Tracker music |
| Audio | snesmod_sfx | Sound effects |
| Input | two_players | Two-player controls |

## Architecture

```
C Code (.c)
    ↓
cc65816 (cproc + QBE w65816)
    ↓
65816 Assembly (.asm)
    ↓
wla-65816 (assembler)
    ↓
wlalink (linker)
    ↓
SNES ROM (.sfc)
```

## License

OpenSNES is released under the MIT License. See LICENSE file for details.

## Links

- [GitHub Repository](https://github.com/user/opensnes)
- [Issue Tracker](https://github.com/user/opensnes/issues)
- [SNES Development Wiki](https://snes.nesdev.org/)
