<div align="center">

# OpenSNES

**Modern, open-source SDK for Super Nintendo development**

*Write SNES games in C. Build on Linux, macOS, and Windows.*

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/license/mit/)
[![Build Status](https://github.com/k0b3n4irb/opensnes/actions/workflows/opensnes_build.yml/badge.svg)](https://github.com/k0b3n4irb/opensnes/actions)
[![Beta](https://img.shields.io/badge/status-beta-yellow.svg)]()
[![C11](https://img.shields.io/badge/language-C11-blue.svg)]()
[![65816](https://img.shields.io/badge/target-65816-purple.svg)]()

**[Documentation](https://k0b3n4irb.github.io/opensnes/)** · **[Getting Started](https://k0b3n4irb.github.io/opensnes/getting-started.html)** · **[API Reference](https://k0b3n4irb.github.io/opensnes/api/)** · **[Contributing](https://k0b3n4irb.github.io/opensnes/contributing.html)**

</div>

---

## Why OpenSNES?

OpenSNES lets you write Super Nintendo games in **standard C11** — no proprietary toolchain, no assembly required to get started. One `make` command builds the compiler, tools, library, and all 41 example ROMs.

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

For prerequisites and platform-specific setup, see the **[Getting Started guide](https://k0b3n4irb.github.io/opensnes/getting-started.html)**.

## Examples

41 examples organized as a progressive learning path — backgrounds, sprites, scrolling, HDMA effects, audio, input, save games, and complete games.

**[Browse all examples](examples/README.md)** · **[Learning path](docs/LEARNING_PATH.md)**

### Supported Platforms

| Platform | Architecture | Status |
|----------|-------------|--------|
| **Linux** | x86_64, arm64 | [![Linux](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=linux)](https://github.com/k0b3n4irb/opensnes/actions) |
| **macOS** | arm64 (Apple Silicon), x86_64 | [![macOS](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=macos)](https://github.com/k0b3n4irb/opensnes/actions) |
| **Windows** | x86_64 (MSYS2) | [![Windows](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=windows)](https://github.com/k0b3n4irb/opensnes/actions) |

---

## Lineage & Acknowledgements

This project builds on **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by [Alekmaul](https://github.com/alekmaul) and its community. OpenSNES is a fork focused on a modern C11 compiler, comprehensive testing, and developer experience.

We also stand on the shoulders of:

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
