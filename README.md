# OpenSNES

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/license/mit/)
[![Build Status](https://github.com/k0b3n4irb/opensnes/actions/workflows/opensnes_build.yml/badge.svg)](https://github.com/k0b3n4irb/opensnes/actions)

**OpenSNES** is a modern, open-source SDK for Super Nintendo (SNES) development, forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib).

> **Work in Progress**: This project is under active development. APIs may change and some features are incomplete.

## About

OpenSNES aims to make SNES development more accessible by providing:

- A clean, modern C development experience
- Improved tooling and build system
- Better documentation and examples
- Cross-platform support (Linux, Windows, macOS)

The project inherits from the excellent work done by [Alekmaul](https://github.com/alekmaul) and the PVSnesLib community, while focusing on developer experience and ease of use.

## Goals

- **Simplicity**: Make SNES development approachable for developers with basic C knowledge
- **Documentation**: Provide clear guides and well-documented examples
- **Modern Workflow**: Simple `make` builds, CI/CD integration, cross-platform compatibility
- **Respect for Hardware**: Stay true to SNES capabilities and constraints

## Getting Started

Before diving into SNES development, we recommend familiarizing yourself with:

- C programming fundamentals
- Basic understanding of how retro consoles work

Useful resources:
- [SFC Development Wiki](https://wiki.superfamicom.org/)
- [SNESdev Wiki](https://snes.nesdev.org/wiki/SNESdev_Wiki)
- [Super NES Programming](https://en.wikibooks.org/wiki/Super_NES_Programming/)

## Documentation

- **[Wiki](https://github.com/k0b3n4irb/opensnes/wiki)** - Guides and tutorials
- **[API Documentation](https://k0b3n4irb.github.io/opensnes/)** - Generated from source (Doxygen)
- **[Examples](examples/)** - Working code examples organized by category
- **[ROADMAP.md](ROADMAP.md)** - Project status and planned features

## Lineage & Acknowledgements

OpenSNES builds upon decades of SNES homebrew community work:

### Core Heritage
- **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by [Alekmaul](https://github.com/alekmaul) - The foundation this project is built upon
- **[SNES-SDK](http://code.google.com/p/snes-sdk/)** by Ulrich Hecht - Original SNES C SDK

### Toolchain
- **[QBE](https://c9x.me/compile/)** by Quentin Carbonneaux - Compiler backend
- **[WLA-DX](https://github.com/vhelin/wla-dx)** by Ville Helin - Assembler suite
- **[cproc](https://sr.ht/~mcf/cproc/)** by Michael Forney - C compiler frontend

### Audio
- **[SNESMOD](https://github.com/mukunda-/snesmod)** by Mukunda Johnson - Tracker audio driver
- **sixpack/optimore** by Mic_ - Audio tools

### PVSnesLib Contributors
Special thanks to all PVSnesLib contributors whose work made this possible:
- [RetroAntho](https://github.com/RetroAntho) - WLA-DX updates, Makefile optimizations
- [DigiDwrf](https://github.com/DigiDwrf/) - HiROM/FastROM support, mouse & superscope
- [Mills32](https://github.com/mills32/) - Mode 7 examples
- [undisbeliever](https://github.com/undisbeliever/) - VBlank code, map engine
- And the entire [PVSnesLib Discord community](https://discord.gg/DzEFnhB)

## Contributing

Contributions are welcome! Please check the [Issues](https://github.com/k0b3n4irb/opensnes/issues) for open tasks.

## License

MIT License - See [LICENSE](LICENSE)

This project maintains the same open-source spirit as PVSnesLib. See [pvsneslib_license.txt](pvsneslib/pvsneslib_license.txt) for the original license.
