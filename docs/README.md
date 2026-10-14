# OpenSNES documentation

The map of `docs/` for people reading the repository on GitHub. The rendered
site, <https://k0b3n4irb.github.io/opensnes/> (built by `make docs`), follows
the same plan from its home page.

## Start

1. [Getting Started](GETTING_STARTED.md) — install, your first ROM, your own project
2. [Learning Path](LEARNING_PATH.md) — the route through the examples
3. [Examples by Category](EXAMPLES_BY_CATEGORY.md) — every example, by topic

When something goes wrong: [Troubleshooting](TROUBLESHOOTING.md) (symptom →
cause), [FAQ](FAQ.md), [Migrating from PVSnesLib](MIGRATING_FROM_PVSNESLIB.md).

## Tutorials

Task-shaped walkthroughs under [tutorials/](tutorials/).

- **Drawing the screen:** [Graphics](tutorials/graphics.md),
  [Sprites](tutorials/sprites.md), [Animation](tutorials/animation.md),
  [Scrolling](tutorials/scrolling.md), [Maps and Tiled](tutorials/map.md),
  [Mode 7](tutorials/mode7.md)
- **Effects:** [DMA](tutorials/dma.md), [HDMA](tutorials/hdma.md),
  [Interrupts](tutorials/interrupts.md), [Colour math](tutorials/colormath.md),
  [Windows](tutorials/window.md), [Mosaic](tutorials/mosaic.md)
- **Game systems:** [Input](tutorials/input.md),
  [Collision](tutorials/collision.md), [Game States](tutorials/game_states.md),
  [The Opt-In Framework](tutorials/framework.md), [Text](tutorials/text.md),
  [The Object Engine](tutorials/object.md) and
  [how it works](tutorials/object_engine.md), [9-Slice Panels](tutorials/panel.md),
  [Music with SNESMOD](tutorials/audio.md),
  [Samples and the raw APU](tutorials/audio_samples.md), [Fixed-Point Math](tutorials/math.md),
  [SRAM Saves](tutorials/sram.md), [Far RAM](tutorials/far_ram.md)
- **Coprocessors and tooling:** [SA-1](tutorials/sa1.md),
  [Super FX: a first job](tutorials/superfx.md),
  [Super FX: running a game around the GSU](tutorials/superfx_advanced.md), [DSP-1](tutorials/dsp1.md),
  [Debugging with luna](tutorials/debugging.md),
  [Profiling](tutorials/profiling.md)

## Craft

[Game-craft guides](craft/) — the design decisions before the code:
[planning](craft/planning.md), [the frame budget](craft/frame-budget.md),
[backgrounds](craft/backgrounds.md), [tiles to levels](craft/tiles-to-levels.md),
[the camera](craft/camera.md), [game feel](craft/game-feel.md).

## Tools

[tools/](tools/) — one page per tool: the `opensnes` CLI, the `opensnes-*`
asset converters, luna, and [your project's Makefile](tools/build.md).

## Reference

- [API Index](API_INDEX.md) — the SDK by what you are trying to do, and which
  tutorial covers each header
- [Known limitations](../KNOWN_LIMITATIONS.md) — every trap that fails
  silently, with its mitigation
- [Frame costs](PERF.md) and [Benchmark](BENCHMARK.md)
- Hardware: [overview](hardware/README.md), [PPU](hardware/PPU.md),
  [APU](hardware/APU.md), [memory map](hardware/MEMORY_MAP.md),
  [registers](hardware/REGISTERS.md), [OAM](hardware/OAM.md)

## Project

[Philosophy](../PHILOSOPHY.md), [What 1.0 will freeze](STABILITY.md),
[Upgrading from 0.48 to 0.49](UPGRADING.md),
[Hardware verification](HARDWARE_VERIFICATION.md),
[Code Style](CODE_STYLE.md), [Contributing](../CONTRIBUTING.md),
[Third Party](THIRD_PARTY.md).

## Building the documentation

```bash
make docs          # generate docs/build/html (warnings printed)
make docs-strict   # the CI gate: the same build with warnings as errors
```

Doxygen 1.16.1 is pinned: earlier versions mangle the Markdown these pages use.
