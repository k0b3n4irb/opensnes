# OpenSNES documentation

Everything the SDK ships, indexed. The rendered version of these pages lives at
<https://k0b3n4irb.github.io/opensnes/> (built by `make docs`); this file is the
map for people reading the repository on GitHub.

## Start here

| Document | What it gives you |
|---|---|
| [Getting Started](GETTING_STARTED.md) | Install, build and run your first ROM |
| [Learning Path](LEARNING_PATH.md) | A route through the examples, ordered by the question you are asking |
| [Examples by Category](EXAMPLES_BY_CATEGORY.md) | The exhaustive index of the example corpus |
| [API Index](API_INDEX.md) | The SDK indexed by *what you are trying to do* |
| [FAQ](FAQ.md) | Short answers to the questions newcomers actually ask |
| [Troubleshooting](TROUBLESHOOTING.md) | Symptoms and their causes |
| [Migrating from PVSnesLib](MIGRATING_FROM_PVSNESLIB.md) | Porting an existing project, and the five traps that bite |

## Guides

| Document | Topic |
|---|---|
| [SNES Graphics Guide](SNES_GRAPHICS_GUIDE.md) | PPU architecture, BG modes, tiles, palettes, sprites |
| [SNES Sound Guide](SNES_SOUND_GUIDE.md) | SPC700, DSP registers, BRR samples, SNESMOD |

## Tutorials

Twenty-six task-shaped walkthroughs under [tutorials/](tutorials/).

**Drawing the screen**

| Tutorial | Topic |
|---|---|
| [Graphics & Backgrounds](tutorials/graphics.md) | Tilemaps, BG modes, VRAM layout |
| [Sprites & Animation](tutorials/sprites.md) | OAM, sprite sheets, metasprites |
| [Animation Techniques](tutorials/animation.md) | Clips, frame timing, dynamic sprites |
| [Scrolling & Parallax](tutorials/scrolling.md) | Map scrolling, layered depth |
| [Scrolling Maps & the Tiled Pipeline](tutorials/map.md) | Tiled levels, streaming columns |
| [Mode 7](tutorials/mode7.md) | Rotation, scaling, perspective |

**Effects**

| Tutorial | Topic |
|---|---|
| [DMA](tutorials/dma.md) | Bulk transfers and the VBlank budget |
| [HDMA](tutorials/hdma.md) | Per-scanline register writes |
| [Colour Math](tutorials/colormath.md) | Transparency, blending, the sub-screen |
| [Window Masking](tutorials/window.md) | Clipping layers, spotlights, iris fades |
| [Mosaic](tutorials/mosaic.md) | The pixelation transition |

**Game systems**

| Tutorial | Topic |
|---|---|
| [Controller Input](tutorials/input.md) | Button masks and multi-player |
| [Collision Detection](tutorials/collision.md) | Rectangles, tiles, slopes |
| [Game States](tutorials/game_states.md) | State machines and transitions |
| [Text & Fonts](tutorials/text.md) | Printing, the tilemap buffer and the flush |
| [The Object Engine](tutorials/object.md) | Entity pool, gravity, map collision |
| [9-Slice Panels](tutorials/panel.md) | Dialog boxes and HUD frames |
| [Audio & Music](tutorials/audio.md) | SNESMOD playback and sound effects |
| [Fixed-Point Math](tutorials/math.md) | 8.8 and 16.16 arithmetic |
| [SRAM Saves](tutorials/sram.md) | Battery-backed save data |
| [Far RAM](tutorials/far_ram.md) | Buffers beyond the 8 KB band |

**Coprocessors and tooling**

| Tutorial | Topic |
|---|---|
| [SA-1](tutorials/sa1.md) | The 10.74 MHz second CPU |
| [SuperFX](tutorials/superfx.md) | The GSU RISC coprocessor |
| [DSP-1](tutorials/dsp1.md) | Hardware matrix and projection maths |
| [Debugging with luna](tutorials/debugging.md) | Stepping, breakpoints, memory inspection |
| [Profiling](tutorials/profiling.md) | Measuring where the frame goes |

## Game-craft

Design guides under [craft/](craft/) — the decisions *before* the code.

| Guide | Question it answers |
|---|---|
| [Planning your game](craft/planning.md) | What can this machine hold? |
| [The frame budget](craft/frame-budget.md) | What fits in 1/60th of a second? |
| [Composing with backgrounds](craft/backgrounds.md) | Which layers, which mode? |
| [From tiles to levels](craft/tiles-to-levels.md) | How do I turn art into a world? |
| [Following the player](craft/camera.md) | How should the camera move? |
| [Game feel](craft/game-feel.md) | Why does this feel dead? |

## Tools

The asset pipeline, one page per tool, under [tools/](tools/).

| Tool | Turns |
|---|---|
| [gfx4snes](tools/gfx4snes.md) | Images into tiles, palettes and maps |
| [img2snes](tools/img2snes.md) | RGB artwork into indexed palettes |
| [palplan](tools/palplan.md) | Many images into one shared palette plan |
| [font2snes](tools/font2snes.md) | Font images into text tiles |
| [tmx2snes](tools/tmx2snes.md) | Tiled levels into SNES map data |
| [aseprite2snes](tools/aseprite2snes.md) | Aseprite animations into clip tables |
| [smconv](tools/smconv.md) | Tracker modules into SNESMOD soundbanks |
| [wav2brr](tools/wav2brr.md) | WAV files into BRR samples |
| [luna](tools/luna.md) | The emulator: every subcommand and flag |

## Hardware reference

| Document | Content |
|---|---|
| [Hardware Overview](hardware/README.md) | CPU, PPU and APU architecture |
| [Memory Map](hardware/MEMORY_MAP.md) | Address space layout |
| [Registers](hardware/REGISTERS.md) | PPU, DMA and I/O registers |
| [OAM](hardware/OAM.md) | Sprite attribute memory |

## For contributors

| Document | Content |
|---|---|
| [CONTRIBUTING.md](../CONTRIBUTING.md) | Branch model, commit format, PR rules |
| [KNOWN_LIMITATIONS.md](../KNOWN_LIMITATIONS.md) | Every trap that fails silently, with its mitigation |
| [Code Style](CODE_STYLE.md) | Coding standards |
| [Benchmark](BENCHMARK.md) | Compiler performance against PVSnesLib |
| [Third Party](THIRD_PARTY.md) | Attribution and licences |

## Header → tutorial map

Which public header each tutorial covers, and where the documentation is thin.
Generated by matching the functions declared in `lib/include/snes/*.h` against
the prose, so "partial" below means the tutorial teaches the topic without
naming most of the API.

| Header | Covered by | State |
|---|---|---|
| `anim.h` | [Animation](tutorials/animation.md) | ✅ |
| `apu.h` | [Audio](tutorials/audio.md) | ✅ |
| `audio.h` | [Audio](tutorials/audio.md) | ✅ |
| `background.h` | [Scrolling](tutorials/scrolling.md), [Graphics](tutorials/graphics.md) | ✅ |
| `collision.h` | [Collision](tutorials/collision.md) | ✅ |
| `colormath.h` | [Colour Math](tutorials/colormath.md) | ✅ |
| `console.h` | scattered across tutorials | 🟡 partial — no page owns init, brightness, fades, region |
| `dma.h` | [DMA](tutorials/dma.md) | ✅ |
| `dsp1.h` | [DSP-1](tutorials/dsp1.md) | ✅ |
| `hdma.h` | [HDMA](tutorials/hdma.md) | ✅ |
| `input.h` | [Input](tutorials/input.md) | 🟡 partial — button masks only; the pad, mouse, Super Scope and multitap functions are undocumented |
| `interrupt.h` | [HDMA](tutorials/hdma.md) | 🟡 partial — the raw IRQ path only, via the H-timer effects |
| `map.h` | [Maps](tutorials/map.md) | ✅ |
| `math.h` | [Fixed-Point Math](tutorials/math.md) | ✅ |
| `mode7.h` | [Mode 7](tutorials/mode7.md) | ✅ |
| `mosaic.h` | [Mosaic](tutorials/mosaic.md) | ✅ |
| `panel.h` | [Panels](tutorials/panel.md) | ✅ |
| `profile.h` | [Profiling](tutorials/profiling.md) | ✅ |
| `sa1.h` | [SA-1](tutorials/sa1.md) | ✅ |
| `scene.h` | [Game States](tutorials/game_states.md) | ✅ |
| `snesmod.h` | [Audio](tutorials/audio.md) | ✅ |
| `sprite.h` | [Sprites](tutorials/sprites.md), [Animation](tutorials/animation.md) | ✅ |
| `sram.h` | [SRAM](tutorials/sram.md) | ✅ |
| `superfx.h` | [SuperFX](tutorials/superfx.md) | ✅ |
| `video.h` | [Graphics](tutorials/graphics.md) | ✅ |
| `window.h` | [Window](tutorials/window.md) | ✅ |
| `asset.h` | — | ❌ no tutorial (asset bundles; see [API Index](API_INDEX.md)) |
| `debug.h` | [Debugging](tutorials/debugging.md) mentions the channel | ❌ the two functions are undocumented |
| `fixed32.h` | — | ❌ no tutorial ([Math](tutorials/math.md) covers 8.8 only) |
| `gameloop.h` | — | ❌ no tutorial (opt-in loop framework) |
| `lzss.h` | — | ❌ no tutorial (decompression) |
| `object.h` | [The Object Engine](tutorials/object.md) | ✅ — writing it found five engine defects, listed in the page's Gotchas |
| `text.h` | [Text & Fonts](tutorials/text.md) | ✅ |
| `registers.h`, `system.h`, `types.h` | — | reference headers: macros and types, no prose needed |

The ❌ rows are the documentation backlog, in rough order of how much API is
behind them: `fixed32.h`, `gameloop.h`, `asset.h`, `lzss.h`, `debug.h`. The
two largest — `object.h` and `text.h` — were closed on 2026-09-18, and
writing those two pages found six defects in the code they document.

## Building the documentation

```bash
make docs          # generate docs/build/html (warnings printed)
make docs-strict   # the CI gate: the same build with warnings as errors
```

Doxygen 1.16.1 is pinned: earlier versions mangle the Markdown these pages use.
