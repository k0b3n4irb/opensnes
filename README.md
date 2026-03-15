<div align="center">

# OpenSNES

**Modern, open-source SDK for Super Nintendo development**

*Write SNES games in C. Build on Linux, macOS, and Windows.*

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/license/mit/)
[![Build Status](https://github.com/k0b3n4irb/opensnes/actions/workflows/opensnes_build.yml/badge.svg)](https://github.com/k0b3n4irb/opensnes/actions)
[![Beta](https://img.shields.io/badge/status-beta-yellow.svg)]()
[![C11](https://img.shields.io/badge/language-C11-blue.svg)]()
[![65816](https://img.shields.io/badge/target-65816-purple.svg)]()

</div>

---

### Supported Platforms

| Platform | Architecture | Toolchain | Status |
|----------|-------------|-----------|--------|
| **Linux** | x86_64, arm64 | GCC / Clang + GNU Make | [![Linux](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=linux)](https://github.com/k0b3n4irb/opensnes/actions) |
| **macOS** | arm64 (Apple Silicon), x86_64 | Clang + GNU Make | [![macOS](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=macos)](https://github.com/k0b3n4irb/opensnes/actions) |
| **Windows** | x86_64 | MSYS2 (UCRT64) + Clang | [![Windows](https://img.shields.io/github/actions/workflow/status/k0b3n4irb/opensnes/opensnes_build.yml?branch=develop&label=windows)](https://github.com/k0b3n4irb/opensnes/actions) |

---

This project builds on **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by [Alekmaul](https://github.com/alekmaul) and its community. OpenSNES is a fork focused on a modern C11 compiler, comprehensive testing, and developer experience.

---

## What OpenSNES Gives You

| Feature | Details |
|---------|---------|
| **C11 compiler for the 65816** | cproc + QBE with a custom backend ([benchmark](docs/BENCHMARK.md)) |
| **Hardware library** (28 headers, modular linking) | PPU, sprites, backgrounds, DMA, HDMA, input, text, audio, Mode 7, collision, animation, SRAM... |
| **Asset pipeline** | `gfx4snes` (PNG to tiles), `font2snes` (font converter), `smconv` (Impulse Tracker to SPC700) |
| **41 examples** | From "Hello World" to Tetris with music — each with README + screenshot |
| **Debug emulator** | [opensnes-emu](tools/opensnes-emu/README.md): snes9x WASM, MCP server, 212 automated checks |
| **Cross-platform** | `make` on Linux, macOS, and Windows (MSYS2). CI-tested on all three. |

## Quick Start

### Prerequisites

<details>
<summary><b>Linux</b> (Debian / Ubuntu)</summary>

```bash
sudo apt install build-essential clang cmake make git
```
</details>

<details>
<summary><b>macOS</b></summary>

```bash
xcode-select --install
brew install cmake
```
</details>

<details>
<summary><b>Windows</b> (MSYS2 UCRT64)</summary>

```bash
pacman -S mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-cmake make git
```
</details>

You'll also want [Mesen2](https://www.mesen.ca/) — the best SNES emulator for development (debugger, trace logger, memory viewer).

### Build & Run

```bash
git clone --recursive https://github.com/k0b3n4irb/opensnes.git
cd opensnes
make                  # builds compiler, tools, library, and all 41 examples

# Open examples/text/hello_world/hello_world.sfc in Mesen2
```

## Examples

41 examples with README and screenshot, organized as a progressive learning path.

**[Browse all examples](examples/README.md)** · **[Learning path](docs/LEARNING_PATH.md)**

---

## The Toolchain

```
 C source       cproc         QBE w65816       wla-65816       wlalink
┌────────┐    ┌────────┐    ┌──────────┐    ┌──────────┐    ┌─────────┐
│ main.c │ →  │  C11   │ →  │  65816   │ →  │ assemble │ →  │  link   │ → game.sfc
│        │    │frontend│    │ assembly │    │          │    │         │
└────────┘    └────────┘    └──────────┘    └──────────┘    └─────────┘
                                                                  ↑
                                              crt0.asm ──────────┘
                                              runtime.asm ───────┘
                                              library modules ───┘
```

| Tool | What it does |
|------|-------------|
| **cc65816** | C compiler (cproc + QBE with 65816 backend) |
| **wla-65816** | WLA-DX assembler for 65816 code |
| **wlalink** | Linker — combines objects into a ROM |
| **gfx4snes** | PNG → SNES tiles, palettes, tilemaps |
| **smconv** | Impulse Tracker (.it) → SNESMOD soundbank for SPC700 |

## Build Commands

```bash
make                    # Build everything
make compiler           # Build cc65816 and WLA-DX
make tools              # Build gfx4snes, font2snes, smconv
make lib                # Build the OpenSNES library
make examples           # Build all example ROMs
make docs               # Generate API documentation (requires Doxygen)
make clean              # Clean all build artifacts
```

---

## Testing

All testing is handled by **[opensnes-emu](tools/opensnes-emu/README.md)** — a SNES debug emulator (snes9x WASM) that serves as the single source of truth.

```bash
cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
```

7 phases, 212 checks: preconditions, compiler tests, build, static analysis, runtime execution, visual regression, lag detection.

---

## Learning Path

**Week 1** — Build the SDK. Open Hello World in Mesen2. Modify it. Break it. Fix it.

**Week 2-3** — Work through the examples in order. By Continuous Scroll, you'll understand tilemaps, sprites, DMA, input, and VBlank timing.

**Week 4+** — Start your game. Copy an example as a template.

**Eventually** — Learn assembly. Check `main.c.asm` — the generated code teaches you the 65816.

Resources:
- [SFC Development Wiki](https://wiki.superfamicom.org/)
- [SNESdev Wiki](https://snes.nesdev.org/wiki/SNESdev_Wiki)
- [Mesen2](https://www.mesen.ca/)

---

## Lineage & Acknowledgements

- **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by Alekmaul — the foundation
- **[QBE](https://c9x.me/compile/)** by Quentin Carbonneaux — compiler backend
- **[WLA-DX](https://github.com/vhelin/wla-dx)** by Ville Helin — assembler
- **[cproc](https://sr.ht/~mcf/cproc/)** by Michael Forney — C frontend
- **[SNESMOD](https://github.com/mukunda-/snesmod)** by Mukunda Johnson — audio driver

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md).

- [Open issues](https://github.com/k0b3n4irb/opensnes/issues)
- [Roadmap](ROADMAP.md)
- [Changelog](CHANGELOG.md)

## License

MIT License — See [LICENSE](LICENSE)
