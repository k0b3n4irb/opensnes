# OpenSNES

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/license/mit/)
[![Build Status](https://github.com/k0b3n4irb/opensnes/actions/workflows/opensnes_build.yml/badge.svg)](https://github.com/k0b3n4irb/opensnes/actions)

**OpenSNES** is a modern, open-source SDK for SNES development.

This project would not exist without **[PVSnesLib](https://github.com/alekmaul/pvsneslib)**
by [Alekmaul](https://github.com/alekmaul) and its community of contributors. OpenSNES is
a fork that builds on their years of work — the library, the toolchain, the examples — with
a focus on documentation, compiler improvements, and developer experience. Credit where
credit is due: PVSnesLib laid the foundation for everything here.

> **Work in Progress**: This project is under active development. APIs may change
> and some features are incomplete.

---

## A Fair Warning

SNES development is hard. Not "takes a weekend to figure out" hard —
**fundamentally, structurally hard.**

The SNES was designed in 1989 by hardware engineers, for assembly programmers,
with no concessions to convenience. There is no operating system. There is no standard
library. There is no debugger that pauses the world while you inspect variables.
The CPU runs at 3.58 MHz, has 128 KB of RAM, and doesn't know what a `float` is.

You will need to understand how a PPU renders tiles scanline by scanline. You will need
to know why writing to VRAM outside of VBlank silently fails. You will need to care about
individual clock cycles, because on this hardware, every single one matters.

**OpenSNES lets you write game logic in C.** That's a real advantage — you get `if`/`else`,
functions, structs, and all the abstraction C provides. The SDK handles initialization,
DMA transfers, joypad reading, sprite management, and audio playback through a clean API.
For many things, you'll never touch a register directly.

**But C alone won't get you to a finished game.**

The PPU has quirks that no C abstraction can fully hide. Performance-critical inner loops,
custom HDMA effects, and advanced hardware tricks eventually require reading — and writing —
65816 assembly. The SDK handles audio through SNESMOD (music and sound effects from C,
no assembly needed), and it handles sprites, backgrounds, and DMA through its library.
But the moment you push past what the library provides, you're on the hardware's terms.

This is not a flaw in the SDK. It's the nature of the machine. OpenSNES will keep
improving — better optimizations, more library functions, smoother workflows — but the
gap between "my sprites move" and "my game ships" will always require understanding
what happens beneath the C.

If that sounds exciting rather than terrifying, you're in the right place.

---

## What OpenSNES Gives You

- **A C compiler for the 65816** — write game logic in C11, compiled through cproc + QBE
  with a custom backend that generates tight 65816 assembly
- **A hardware library** — clean APIs for PPU, sprites, backgrounds, DMA, input, HDMA,
  text, and audio (SNESMOD tracker playback)
- **Asset tools** — convert PNG images to SNES tiles (`gfx4snes`), fonts (`font2snes`),
  and Impulse Tracker music to SPC700 format (`smconv`)
- **Working examples** — from "Hello World" to a playable Breakout clone, each with
  a walkthrough that explains not just *what* the code does, but *why*
- **Cross-platform builds** — `make` on Linux, macOS, and Windows (MSYS2)

## Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/k0b3n4irb/opensnes.git
cd opensnes

# Build everything (compiler, tools, library, examples)
make

# Run an example
# Open examples/text/hello_world/hello_world.sfc in Mesen2
```

You'll need: Clang, GNU Make, and a SNES emulator
([Mesen2](https://www.mesen.ca/) recommended).

## Examples

Every example has a detailed README with a full walkthrough that explains not just
*what* the code does, but *why*. Start with Hello World and work your way up.

### Start Here

| Example | What you'll learn |
|---------|------------------|
| [Hello World](examples/text/hello_world/) | Tiles, tilemaps, palettes — the PPU fundamentals |
| [Custom Font](examples/text/custom_font/) | Building a complete character set, raw register access |
| [Calculator](examples/basics/calculator/) | Joypad input with edge detection, UI with background tiles |

### Browse All Examples

| Category | What's inside |
|----------|--------------|
| [Text](examples/text/) | Fonts, tile-based text rendering |
| [Basics](examples/basics/) | Input handling, collision, movement |
| [Sprites](examples/graphics/sprites/) | Static, animated, dynamic, and metasprites |
| [Backgrounds](examples/graphics/backgrounds/) | Modes 0/1/3/5/7, scrolling, parallax |
| [Effects](examples/graphics/effects/) | HDMA, fading, transparency, mosaic, windows |
| [Input](examples/input/) | Joypad reading, multiplayer |
| [Audio](examples/audio/) | Sound effects, tracker music (SNESMOD) |
| [Memory](examples/memory/) | HiROM, SRAM save games |
| [Games](examples/games/) | Breakout, platformer, and more |

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
| **wla-spc700** | WLA-DX assembler for SPC700 audio code |
| **wlalink** | Linker — combines objects into a ROM |
| **gfx4snes** | PNG → SNES tiles (.pic), palettes (.pal), tilemaps (.map) |
| **font2snes** | PNG font grid → C header with tile data |
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

# Build a single example
make -C examples/games/breakout
```

---

## Learning Path

There's no shortcut, but there is a path.

**Week 1 — Get your bearings.** Build the SDK. Open Hello World in Mesen2. Read the
walkthrough. Modify the message. Change the colors. Break it on purpose, then fix it.
Understand what every register write does.

**Week 2-3 — Work through the examples.** Go in order. Each one builds on the last.
By the time you finish Continuous Scroll, you'll understand tilemaps, sprites, DMA,
input handling, and VBlank timing. These are the fundamentals of every SNES game.

**Week 4+ — Start your game.** Copy an example as a template. Add features one at a
time. When something doesn't work, check the generated assembly (`main.c.asm`) — it
will teach you more about the 65816 than any tutorial.

**Eventually — Learn assembly.** You don't need it on day one. But the day you want
to write a custom HDMA effect, optimize a tight loop, or push the hardware beyond what
the library offers, you'll be glad you started reading it in `main.c.asm` weeks ago.
The transition from "reading" to "writing" assembly is smaller than you think.

Resources that will help:

- [SFC Development Wiki](https://wiki.superfamicom.org/) — hardware reference
- [SNESdev Wiki](https://snes.nesdev.org/wiki/SNESdev_Wiki) — community knowledge base
- [65816 Instruction Set](https://undisbeliever.net/snesdev/65816-opcodes.html) — when you're ready
- [Mesen2](https://www.mesen.ca/) — the best SNES emulator for development (debugger, trace logger, memory viewer)
- [Super NES Programming](https://en.wikibooks.org/wiki/Super_NES_Programming/) — Wikibook tutorial

---

## Emulator vs. Real Hardware

**A ROM that works in an emulator can fail on a real console.** This is one of the
most common — and most frustrating — surprises in SNES development.

Emulators are forgiving. They tolerate DMA transfers that slightly overrun VBlank.
They don't crash when HDMA and DMA collide on the bus. They handle IRQ timing edge
cases gracefully. Real hardware does none of these things.

Common issues that show up only on console:

- **VBlank overrun** — you transfer 5 KB of tile data per frame, it works in Mesen2,
  but on hardware the screen glitches because the transfer didn't finish before
  rendering started
- **DMA/HDMA collision** — on CPU revision 1 consoles, a DMA completing just as HDMA
  starts can crash the system entirely. Emulators don't reproduce this.
- **Bus timing** — the exact cycle when a register write takes effect can differ by
  1-2 cycles between emulator and hardware. Usually invisible, until it isn't.
- **Audio sync** — SPC700 communication timing is approximate in most emulators.
  A handshake that works in software may fail on silicon.

**Test on real hardware.** Even if you develop entirely in an emulator, verify on
console before calling something "done."

### Flash Cartridges

Flash cartridges let you load ROMs from an SD card and run them on a real SNES.
For development, they're essential:

| Cartridge | Notes |
|-----------|-------|
| [**FXPak Pro**](https://krikzz.com/our-products/cartridges/fxpak-pro.html) | The reference. Formerly SD2SNES. FPGA-based, supports nearly all enhancement chips (Super FX, SA-1, Cx4). USB port for debugging. Made by [Krikzz](https://krikzz.com/). |

> **Avoid cheap clones.** The SD2SNES design is open-source, so clones exist — but
> quality varies wildly. Glitchy behavior, missing audio features, or outright failure.
> For development work where you need to trust the hardware, buy from Krikzz or a
> reputable reseller.

[Mesen2](https://www.mesen.ca/) remains the best emulator for day-to-day development —
its debugger, trace logger, and memory viewer are indispensable. But the final test is
always the real machine.

---

## Lineage & Acknowledgements

OpenSNES stands on the shoulders of the SNES homebrew community:

### The Foundation
- **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by [Alekmaul](https://github.com/alekmaul) and contributors — the library, toolchain, and examples that made this project possible
- **[SNES-SDK](http://code.google.com/p/snes-sdk/)** by Ulrich Hecht — the original SNES C SDK that started it all

### Toolchain
- **[QBE](https://c9x.me/compile/)** by Quentin Carbonneaux — compiler backend
- **[WLA-DX](https://github.com/vhelin/wla-dx)** by Ville Helin — assembler suite
- **[cproc](https://sr.ht/~mcf/cproc/)** by Michael Forney — C compiler frontend

### Audio
- **[SNESMOD](https://github.com/mukunda-/snesmod)** by Mukunda Johnson — tracker audio driver

## Contributing

Contributions are welcome! Check the [Issues](https://github.com/k0b3n4irb/opensnes/issues)
for open tasks, or the [Roadmap](ROADMAP.md) for planned features.

## License

MIT License — See [LICENSE](LICENSE)

This project maintains the same open-source spirit as PVSnesLib.
See [pvsneslib_license.txt](pvsneslib/pvsneslib_license.txt) for the original license.
