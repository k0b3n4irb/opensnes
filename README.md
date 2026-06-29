<div align="center">

<img src="assets/logo_128x128.png" alt="OpenSNES" width="128" height="128"/>

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

OpenSNES lets you write Super Nintendo games in **standard C11** — no proprietary toolchain, no assembly required to get started. One `make` command builds the compiler, tools, library, and all 56 example ROMs.

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

## Who is OpenSNES for?

OpenSNES meets you at your level. The SDK does the hardware bookkeeping so you
can start in C, and gets out of the way when you're ready to go deeper. There's
a lane here whether you're curious, comfortable, or hardcore:

- **New to the SNES but fluent in C?** Start with
  [`examples/text/hello_world`](examples/text/hello_world/) and follow the
  [learning path](docs/LEARNING_PATH.md). You'll have a ROM running in minutes
  and meet the hardware one example at a time — no assembly required to get
  moving.
- **Comfortable with retro hardware?** The library covers sprites, backgrounds,
  DMA, HDMA, input, audio and Mode 7 through a clean C API, so your time goes
  into the game, not the boilerplate.
- **Porting from PVSnesLib, or chasing cycles?** Drop to 65816 assembly
  whenever you want — [`compiler/ABI.md`](compiler/ABI.md) documents the calling
  convention with worked examples, and the SDK never hides the hardware from you.

Two honest expectations before you start:

- **The SNES doesn't hide its hardware, and neither do we.** The language is the
  easy part; the machine is the hard part. You'll meet VRAM-only-in-VBlank, DMA
  budgets, and hex addresses — that's the nature of the target, not a flaw in
  the SDK. The difference here is that you can learn it incrementally, in C, with
  a worked example for each step.
- **The SDK is late beta.** The test suite is green and CI is enforced on three
  platforms, but several optimisations and chip APIs are still planned (see the
  [roadmap](ROADMAP.md)). It's a great place to learn and build; pin a commit if
  you need a frozen toolchain for a hard deadline.

**Read [`KNOWN_LIMITATIONS.md`](KNOWN_LIMITATIONS.md) before you start.** It
catalogs every silent failure we know about, with severity tags and
mitigations. Skim it once; refer back when something mysterious happens.
For ASM-writing contributors, [`compiler/ABI.md`](compiler/ABI.md) documents
the calling convention with worked examples (push order, frame layout, return
values) — required reading before porting any function from PVSnesLib.

### Enhancement-chip maturity

| Mode / Chip | Status | Notes |
|-------------|:------:|-------|
| **LoROM** | Stable | Default. Production-ready. |
| **HiROM** | Stable | Set `USE_HIROM := 1` in your Makefile. |
| **FastROM** | Stable | Set `USE_FASTROM := 1`. Adds ~33 % CPU bandwidth. |
| **SA-1** | Experimental | C wrapper is minimal; coprocessor code lives in a per-example `sa1_boot.asm`. SIWP register init is an assumption, not a published spec. |
| **SuperFX** | Experimental | GSU is assembly-only (no C compiler exists for the RISC ISA). **Validated by [luna](https://github.com/k0b3n4irb/luna)**, which detects and executes the GSU natively in the headless test harness. |

---

## What OpenSNES Gives You

| | |
|---|---|
| **C11 compiler for the 65816** | cproc + QBE with a custom backend ([benchmark](docs/BENCHMARK.md)) |
| **30 hardware modules** | PPU, sprites, backgrounds, DMA, HDMA, input, audio, Mode 7, collision, SRAM... |
| **Asset pipeline** | PNG to tiles, fonts, Impulse Tracker to SPC700 |
| **56 examples** | From "Hello World" to Tetris with music — each with README and screenshot |
| **Framework opt-ins** | Game loop, scene stack, asset bundles — drop them in if they fit, ignore them otherwise |
| **Debug emulator** | [luna](https://github.com/k0b3n4irb/luna) (cycle-accurate native emulator) — corpus liveness + visual regression + functional probes; SA-1 / Super FX / DSP-1 run natively |
| **Cross-platform** | Linux, macOS, Windows — CI-enforced on all three |

## Design Philosophy

OpenSNES is a **2D game engine** for SNES, not a thin C-over-asm wrapper.
Five principles guide every design decision:

1. **Sane defaults, escape hatches** — the 90% case is a one-liner; the
   10% case stays possible.
2. **Hide quirks, document the escape** — hardware traps don't reach
   the user, but the explanation is one click away.
3. **Modules are opt-in, never all-or-nothing** — `LIB_MODULES` selects
   what links into your ROM.
4. **Type-safe at the boundary** — structs and enums beat positional
   `u16` arguments.
5. **Predictable performance** — no hidden allocations, no lazy
   patterns, every frame-time cost is documented.

See **[PHILOSOPHY.md](PHILOSOPHY.md)** for the full statement and the
explicit positioning vs. PVSnesLib (the two projects sit at different
altitudes of the same stack and are complementary).

## Quick Start

```bash
git clone --recursive https://github.com/k0b3n4irb/opensnes.git
cd opensnes
make
```

Open `examples/text/hello_world/hello_world.sfc` in [Mesen2](https://www.mesen.ca/) and you're running on a Super Nintendo.

To start your own game, the build installs an `opensnes` CLI in `bin/`:

```bash
bin/opensnes init my-game --template game   # scaffolds a working project
cd my-game && ../bin/opensnes run           # builds and launches your emulator
```

`opensnes doctor` checks your toolchain, library, and emulator. For prerequisites
and platform-specific setup, see the **[Getting Started guide](https://k0b3n4irb.github.io/opensnes/getting_started.html)**.

## Examples

56 examples organized as a progressive learning path — backgrounds, sprites, scrolling, HDMA effects, audio, input, save games, and complete games.

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

- **[PVSnesLib](https://github.com/alekmaul/pvsneslib)** by Alekmaul — the foundation
- **[QBE](https://c9x.me/compile/)** by Quentin Carbonneaux — compiler backend
- **[cproc](https://sr.ht/~mcf/cproc/)** by Michael Forney — C frontend
- **[WLA-DX](https://github.com/vhelin/wla-dx)** by Ville Helin — assembler
- **[SNESMOD](https://github.com/mukunda-/snesmod)** by Mukunda Johnson — audio driver

See [ATTRIBUTION.md](ATTRIBUTION.md) for the full list of dependencies, licenses, and contributors.

## Contributing

Contributions welcome! See the **[Contributing Guide](https://k0b3n4irb.github.io/opensnes/contributing.html)**.

- [Documentation](https://k0b3n4irb.github.io/opensnes/)
- [Open issues](https://github.com/k0b3n4irb/opensnes/issues)
- [Roadmap](ROADMAP.md)
- [Changelog](CHANGELOG.md)
- [Known limitations](KNOWN_LIMITATIONS.md) — silent failures and trade-offs to read before starting

## License

MIT License — See [LICENSE](LICENSE)
