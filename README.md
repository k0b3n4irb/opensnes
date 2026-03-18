<div align="center">

# OpenSNES

**Modern, open-source SDK for Super Nintendo development**

*Write SNES games in C. Build on Linux, macOS, and Windows.*

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/license/mit/)
[![Build Status](https://github.com/k0b3n4irb/opensnes/actions/workflows/opensnes_build.yml/badge.svg)](https://github.com/k0b3n4irb/opensnes/actions)
[![Beta](https://img.shields.io/badge/status-beta-yellow.svg)]()
[![C11](https://img.shields.io/badge/language-C11-blue.svg)]()
[![65816](https://img.shields.io/badge/target-65816-purple.svg)]()

**[Documentation](https://k0b3n4irb.github.io/opensnes/)** · **[Getting Started](https://k0b3n4irb.github.io/opensnes/getting_started.html)** · **[Contributing](https://k0b3n4irb.github.io/opensnes/contributing.html)**

</div>

---

## Introduction

OpenSNES lets you write Super Nintendo games in **standard C11** — no proprietary toolchain, no assembly required to get started. One `make` command builds the compiler, tools, library, and all 41 example ROMs.

This project builds on **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by [Alekmaul](https://github.com/alekmaul) and its community. OpenSNES is a fork focused on a modern C11 compiler, comprehensive testing, and developer experience.

## A Fair Warning

SNES development is hard. Not "takes a weekend to figure out" hard — fundamentally, structurally hard.

The SNES was designed in 1989 by hardware engineers, for assembly programmers, with no concessions to convenience. There is no operating system. There is no standard library. There is no debugger that pauses the world while you inspect variables. The CPU runs at 3.58 MHz, has 128 KB of RAM, and doesn't know what a float is.

You will need to understand how a PPU renders tiles scanline by scanline. You will need to know why writing to VRAM outside of VBlank silently fails. You will need to care about individual clock cycles, because on this hardware, every single one matters.

OpenSNES lets you write game logic in C. That's a real advantage — you get if/else, functions, structs, and all the abstraction C provides. The SDK handles initialization, DMA transfers, joypad reading, sprite management, and audio playback through a clean API. For many things, you'll never touch a register directly.

But C alone won't get you to a finished game.

The PPU has quirks that no C abstraction can fully hide. Performance-critical inner loops, custom HDMA effects, and advanced hardware tricks eventually require reading — and writing — 65816 assembly. The SDK handles audio through SNESMOD (music and sound effects from C, no assembly needed), and it handles sprites, backgrounds, and DMA through its library. But the moment you push past what the library provides, you're on the hardware's terms.

This is not a flaw in the SDK. It's the nature of the machine. OpenSNES will keep improving — better optimizations, more library functions, smoother workflows — but the gap between "my sprites move" and "my game ships" will always require understanding what happens beneath the C.

If that sounds exciting rather than terrifying, you're in the right place.

---

## What OpenSNES Gives You

| | |
|---|---|
| **C11 compiler for the 65816** | cproc + QBE with a custom backend ([benchmark](docs/BENCHMARK.md)) |
| **28-module hardware library** | PPU, sprites, backgrounds, DMA, HDMA, input, audio, Mode 7, collision, SRAM... |
| **Asset pipeline** | PNG to tiles, fonts, Impulse Tracker to SPC700 |
| **41 examples** | From "Hello World" to Tetris with music — each with README and screenshot |
| **Debug emulator** | snes9x WASM with 212 automated checks |
| **Cross-platform** | Linux, macOS, Windows — CI-tested on all three |

## Quick Start

```bash
git clone --recursive https://github.com/k0b3n4irb/opensnes.git
cd opensnes
make
```

Open `examples/text/hello_world/hello_world.sfc` in [Mesen2](https://www.mesen.ca/) and you're running on a Super Nintendo.

For prerequisites and platform-specific setup, see the **[Getting Started guide](https://k0b3n4irb.github.io/opensnes/getting_started.html)**.

## Examples

41 examples organized as a progressive learning path — backgrounds, sprites, scrolling, HDMA effects, audio, input, save games, and complete games.

**[Browse all examples](examples/README.md)** · **[Learning path](https://k0b3n4irb.github.io/opensnes/learning_path.html)**

### Supported Platforms

| Platform | Architecture | Status |
|----------|-------------|--------|
| **Linux** | x86_64, arm64 | [![Linux](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=linux)](https://github.com/k0b3n4irb/opensnes/actions) |
| **macOS** | arm64 (Apple Silicon), x86_64 | [![macOS](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=macos)](https://github.com/k0b3n4irb/opensnes/actions) |
| **Windows** | x86_64 (MSYS2) | [![Windows](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=windows)](https://github.com/k0b3n4irb/opensnes/actions) |

---

## Acknowledgements

We stand on the shoulders of:

- **[QBE](https://c9x.me/compile/)** by Quentin Carbonneaux — compiler backend
- **[cproc](https://sr.ht/~mcf/cproc/)** by Michael Forney — C frontend
- **[WLA-DX](https://github.com/vhelin/wla-dx)** by Ville Helin — assembler
- **[SNESMOD](https://github.com/mukunda-/snesmod)** by Mukunda Johnson — audio driver

## Contributing

Contributions welcome! See the **[Contributing Guide](https://k0b3n4irb.github.io/opensnes/contributing.html)**.

- [Documentation](https://k0b3n4irb.github.io/opensnes/)
- [Open issues](https://github.com/k0b3n4irb/opensnes/issues)
- [Roadmap](ROADMAP.md)
- [Changelog](CHANGELOG.md)

## License

MIT License — See [LICENSE](LICENSE)
