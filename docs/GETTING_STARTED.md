# Getting Started with OpenSNES {#getting_started}

This guide will get you from zero to running your first SNES ROM in about 10 minutes.

## Choose Your Path

| | **I want to make SNES games** | **I want to contribute to the SDK** |
|---|---|---|
| **What** | Download the pre-built SDK, write C code, build ROMs | Clone the repo, modify compiler/library/tools |
| **Prerequisites** | `make`, `python3` + text editor | clang, cmake, git, python3 |
| **Time to start** | ~5 minutes | ~15 minutes |
| **Go to** | [Path A: Game Developer](#path-a-game-developer) | [Path B: SDK Developer](#path-b-sdk-developer) |

---

## Path A: Game Developer

You want to write SNES games in C. The SDK is already compiled — you just need
to download it, write code, and run `make`.

### A1. Install Prerequisites

You need `make` (the build tool), `python3` (the build's post-link checks —
ROM size, bank budgets — are Python scripts) and an emulator. No compiler
installation required — the SDK ships with its own cross-compiler.

**macOS:**
```bash
xcode-select --install
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install make python3
```

**Linux (Fedora):**
```bash
sudo dnf install make
```

**Windows:**
1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal
3. Run: `pacman -S make python`

### A2. Get an Emulator

**luna** is the SDK's emulator: it plays ROMs, runs the test harness and
debugs (`tutorials/debugging.md`). `scripts/install-luna.sh` fetches the
pinned release for Linux, macOS or Windows (MSYS2 / Git Bash) into
`testing/bin/`: `luna` (headless: tests, state, debugging) and
`luna-gui` (a window you play in). Any other SNES emulator works too, for a second
opinion:

| Emulator | Best For | Download |
|----------|----------|----------|
| [Mesen](https://www.mesen.ca/) | Debugging, accuracy | mesen.ca |
| [bsnes](https://github.com/bsnes-emu/bsnes) | Cycle accuracy | GitHub releases |
| [Snes9x](https://www.snes9x.com/) | Performance | snes9x.com |

### A3. Download OpenSNES SDK

Download the latest release for your platform from the
[GitHub Releases page](https://github.com/k0b3n4irb/opensnes/releases):

| Platform | File |
|----------|------|
| Linux x86_64 | `opensnes_<version>_linux_x86_64.zip` |
| Linux aarch64 | `opensnes_<version>_linux_arm64.zip` |
| macOS arm64 | `opensnes_<version>_darwin_arm64.zip` |
| Windows x86_64 | `opensnes_<version>_windows_x86_64.zip` |

Extract the archive somewhere permanent (e.g., `~/opensnes` or `C:\opensnes`).
It holds what a project build needs: the toolchain and asset tools in `bin/`,
the library, the build system, the starter project. The documentation you
are reading is online at https://k0b3n4irb.github.io/opensnes/ and the
examples come as a separate, platform-independent archive on the same
releases page: `opensnes-examples_<version>.zip` (sources, assets, and
every ROM already built under `examples/bin/`).

### A4. Run Your First ROM

Extract the examples archive next to the SDK and pick a ROM:

```bash
cd opensnes-examples_<version>/examples/text/print_string

# Play it in luna's window (install it once from the SDK root: scripts/install-luna.sh)
~/opensnes/testing/bin/luna-gui print_string.sfc
```

> Any other SNES emulator opens the `.sfc` as well (Mesen, bsnes, Snes9x —
> see the table above).

You should see "TEXT MODULE TEST" in white on a dark blue screen.

### A5. Create Your Own Project

The SDK ships an **`opensnes` CLI** (in the extracted `bin/` directory) that
scaffolds, builds, and runs a project for you. Put it on your `PATH` once — from
the extracted SDK directory:

```bash
export PATH="$PWD/bin:$PATH"   # add to your shell profile to make it permanent
```

Then create, build, and run your first project in three commands:

```bash
opensnes init my-game --template game   # scaffolds Makefile + main.c + res/
cd my-game
opensnes run                            # builds the ROM and launches your emulator
```

`opensnes init` gives you a project that builds and runs from the start. Pick a
template:

- **`--template blank`** — a "HELLO SNES!" text screen, the simplest starting point.
- **`--template game`** — a white sprite you move with the D-pad, a starting
  point for an action game.

Other commands (@ref tools_opensnes): `opensnes build`, `opensnes clean`, and `opensnes doctor` (checks
your toolchain, library, and emulator and tells you what is missing). Run
`opensnes --help` for the full list.

#### Manual setup (the long way)

Prefer to wire it by hand, or curious what `init` generates? Create a new
directory anywhere on your machine:

```bash
mkdir ~/my-snes-game
cd ~/my-snes-game
```

Create two files:

**Makefile:**
```makefile
# Point to your OpenSNES installation
OPENSNES := /path/to/opensnes

# ROM settings
TARGET   := my_game.sfc
ROM_NAME := MY GAME

# Source files
CSRC     := main.c

# Use OpenSNES library
USE_LIB  := 1
LIB_MODULES := console dma text background

# Include the build system
include $(OPENSNES)/make/common.mk
```

**main.c:**
```c
#include <snes.h>

int main(void) {
    textModeInit();                     /* sets up the PPU + text engine in one call */
    textPrintAt(8, 10, "Hello SNES!");  /* NMI auto-flushes the text to VRAM */
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
```

Build and run:
```bash
make
mesen my_game.sfc
```

That's it — you're making SNES games.

### Project Structure

```
my-snes-game/
├── Makefile        # Build configuration
├── main.c          # Your game code
├── res/            # Assets (optional)
│   ├── tiles.png
│   └── music.it
└── test/           # Project tests (optional — see below): one luna manifest each
    ├── boot.toml
    └── walk_right.toml
```

### Test Your Game

Projects can declare automated tests that run in **luna**, the same
cycle-accurate emulator the SDK's own test suite uses, with luna's own
manifests: one `test/<name>.toml` per test, run by `luna test`. Opt-in is
simply the presence of a `test/*.toml` (the `game` template ships two):

```toml
# test/boot.toml — a visual baseline and WRAM values at frame 180
rom = "../my-snes-game.sfc"
frames = 180

[asserts]
fbhash = ""                       # the frame's hash; `make test-update` fills it

[[checkpoint]]
at_frame = 180
[checkpoint.values]
player_x = 120                    # by the variable's name in your C
player_y = 100
```

```toml
# test/walk_right.toml — hold RIGHT for 60 frames, then check the game state
rom = "../my-snes-game.sfc"
frames = 150
input = "30:0x100,90:0"           # "frame:buttons_hex" (RIGHT = 0x100), until frame 90

[[checkpoint]]
at_frame = 150
[checkpoint.values]
player_x = 180                    # 120 + 60
```

Workflow:

```bash
scripts/install-luna.sh   # once, from the SDK root: fetch the pinned luna
make test-update          # seed the visual baselines (asserts.fbhash) in place
make test                 # from now on: exit 0 = green, 1 = regression
```

(Or `opensnes test` / `opensnes test --update` from the project directory.)

What a manifest can judge:

- **WRAM values** — `[checkpoint.values]` entries are read by luna at
  `at_frame`, with symbol names resolved from your ROM's `.sym` file:
  write the name the variable has in your C. Two sources may define a
  `static` of the same name; luna then refuses the bare name and lists
  the candidates, each with its source file as a suffix
  (`"player_x.main"` for the one of `main.c`, quoted because of the
  dot). A value is a number, or `{ eq = 0x10, width = 1 }` to
  set the width; `[checkpoint.delta]` says `"increased"` / `"unchanged"`
  between two checkpoints;
- **Visual baselines** — `asserts.fbhash` is the hash of the frame at
  `frames`; `make test-update` rewrites it in place, comments kept, after
  an intended change of what the game shows;
- **The machine** — `region = "pal"` runs the test under PAL, `power_on`
  / `seed` the RAM the ROM boots on; `[asserts.dma] unsafe_writes = 0`
  fails on a VRAM write outside blanking.

`luna test --help` lists everything a manifest can say. Commit `test/` to
your repo; rerun `make test-update` when you intentionally change what the
game shows.

---

## Path B: SDK Developer

You want to modify the compiler, library, tools, or build system itself.
This requires building the entire SDK from source.

### B1. Install Prerequisites

You need a full C/C++ development environment.

**macOS:**
```bash
xcode-select --install
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install build-essential clang cmake make git python3
```

**Linux (Fedora):**
```bash
sudo dnf install clang cmake make git python3
```

**Windows:**
1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal
3. Run:
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-cmake base-devel git python
```

### B2. Clone and Build

```bash
# Clone with submodules (--recursive is required!)
git clone --recursive https://github.com/k0b3n4irb/opensnes.git
cd opensnes

# Build everything: compiler → tools → library → examples
make
```

The first build compiles the toolchain (cproc, QBE, WLA-DX) and takes a few
minutes; later builds take seconds. It ends with:
```
OpenSNES SDK build complete!
```

### B3. Run Tests

```bash
make tests   # luna: coverage + visual regression + probes
```

### B4. Development Workflow

```bash
make clean && make     # Full rebuild (required after compiler changes)
make lib               # Rebuild library only
make examples          # Rebuild examples only
make -C examples/text/print_string  # Rebuild one example
```

See [CLAUDE.md](https://github.com/k0b3n4irb/opensnes/blob/develop/CLAUDE.md) for architecture details and coding conventions.

---

## What's Next?

You've built and run a ROM. Three complementary ways forward — pick by how you
like to learn, or use all three:

- **Follow the journey.** @ref learning_path walks the examples as a developer's
  questions — "can I put something on screen?", "can I build a world?" — each
  stage buying real confidence, never a wall you can't climb.
- **Make your own assets.** @ref tools is the converter pipeline that turns your
  PNGs, Tiled maps, fonts, WAVs and tracker modules into SNES data — `gfx4snes`,
  `tmx2snes`, `wav2brr` and friends, most wired into the build for you.
- **Decide what to build.** @ref craft is hardware-grounded *design* advice:
  budget your VRAM before you draw, choose a background mode from your genre,
  compose layers, and scope a first game you can actually finish.

Or explore the examples by complexity:

| Level | Examples | What You'll Learn |
|-------|----------|-------------------|
| **Beginner** | `text/print_string`, `text/scroll_message` | Console output, text formatting |
| **Intermediate** | `sprites/simple_sprite`, `input/two_players` | Sprites, controller input |
| **Advanced** | `mode7/rotate_scale`, `audio/snesmod_music` | Mode 7, tracker music |
| **Expert** | `games/breakout`, `games/likemario` | Complete game structure |
| **SA-1 Coprocessor** | `chips/sa1_hello`, `chips/sa1_starfield` | 10.74 MHz second CPU ([tutorial](tutorials/sa1.md)) |
| **SuperFX (GSU)** | `chips/superfx_hello`, `chips/superfx_3d` | RISC coprocessor, 3D rendering ([tutorial](tutorials/superfx.md)) |

Browse all examples:
```bash
ls examples/*/
```

## Tutorials

The full set of tutorials, always current, is on the docs home page —
see @ref index "the tutorial navigation in mainpage". The most common
starting points:

| Topic | Guide |
|-------|-------|
| Graphics & Backgrounds | [tutorials/graphics.md](tutorials/graphics.md) |
| Sprites & Animation | [tutorials/sprites.md](tutorials/sprites.md), [tutorials/animation.md](tutorials/animation.md) |
| Scrolling & Parallax | [tutorials/scrolling.md](tutorials/scrolling.md) |
| Collision Detection | [tutorials/collision.md](tutorials/collision.md) |
| Input Handling | [tutorials/input.md](tutorials/input.md) |
| Audio & Music | [tutorials/audio.md](tutorials/audio.md) |
| Math & Fixed-Point | [tutorials/math.md](tutorials/math.md) |
| DMA | [tutorials/dma.md](tutorials/dma.md) |
| HDMA Effects | [tutorials/hdma.md](tutorials/hdma.md) |
| Color Math | [tutorials/colormath.md](tutorials/colormath.md) |
| Hardware Windows | [tutorials/window.md](tutorials/window.md) |
| Mosaic | [tutorials/mosaic.md](tutorials/mosaic.md) |
| Mode 7 | [tutorials/mode7.md](tutorials/mode7.md) |
| Game States | [tutorials/game_states.md](tutorials/game_states.md) |
| SRAM Saves | [tutorials/sram.md](tutorials/sram.md) |
| Far RAM (`FAR` objects) | [tutorials/far_ram.md](tutorials/far_ram.md) |
| SA-1 Coprocessor | [tutorials/sa1.md](tutorials/sa1.md) |
| SuperFX (GSU) | [tutorials/superfx.md](tutorials/superfx.md) |
| Debugging | [tutorials/debugging.md](tutorials/debugging.md) |

## Troubleshooting the setup

### "command not found: make"

Install build tools (see prerequisites for your path above).

### "fatal: repository not found" or empty compiler folder

You forgot `--recursive` when cloning. Fix it:
```bash
git submodule update --init --recursive
```

### Build fails with "Library not built"

Run `make` from the SDK root first — the library must be compiled before examples:
```bash
cd /path/to/opensnes && make lib
```

### Black screen when running ROM

Your ROM built but doesn't display anything. Common causes:
1. Missing `setScreenOn()` call
2. Missing `WaitForVBlank()` in main loop
3. Wrong `LIB_MODULES` — check that you include all needed modules

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more solutions.

### Build fails with "refusing to emit silently-wrong code"

The compiler stops, with the feature named, rather than generate wrong code
for what it does not support: struct assignment, structs passed or returned
by value, variadic functions and inline assembly. Pass a pointer, copy the
fields, or move the code to a `.asm` file. 32-bit `u32`/`s32` arithmetic is
fully supported (it is slower than 16-bit, not wrong). See
`KNOWN_LIMITATIONS.md` for the full list.

## Getting Help

- **Issues**: [github.com/k0b3n4irb/opensnes/issues](https://github.com/k0b3n4irb/opensnes/issues)
- **SNES Dev Wiki**: [snes.nesdev.org](https://snes.nesdev.org/)

## Which API do I need?

See [API_INDEX.md](API_INDEX.md) — the SDK indexed by *what you are
trying to do*, with the example that does it. Worth a scan before you
write a helper: several already exist.
