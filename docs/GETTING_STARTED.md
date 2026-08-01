# Getting Started with OpenSNES {#getting_started}

This guide will get you from zero to running your first SNES ROM in about 10 minutes.

## Choose Your Path

| | **I want to make SNES games** | **I want to contribute to the SDK** |
|---|---|---|
| **What** | Download the pre-built SDK, write C code, build ROMs | Clone the repo, modify compiler/library/tools |
| **Prerequisites** | `make` + text editor | clang, cmake, git, python3 |
| **Time to start** | ~5 minutes | ~15 minutes |
| **Go to** | [Path A: Game Developer](#path-a-game-developer) | [Path B: SDK Developer](#path-b-sdk-developer) |

---

## Path A: Game Developer

You want to write SNES games in C. The SDK is already compiled — you just need
to download it, write code, and run `make`.

### A1. Install Prerequisites

You only need `make` (the build tool) and an emulator. No compiler installation
required — the SDK ships with its own cross-compiler.

**macOS:**
```bash
xcode-select --install
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install make
```

**Linux (Fedora):**
```bash
sudo dnf install make
```

**Windows:**
1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal
3. Run: `pacman -S make`

### A2. Get an Emulator

luna (the SDK test/debug backend, installed by `scripts/install-luna.sh`)
covers headless debugging — see `tutorials/debugging.md`. For playing your
ROM in a window, pick any GUI emulator:

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

### A4. Run Your First ROM

The SDK comes with pre-built example ROMs:

```bash
cd opensnes/examples/text/print_string

# Open in your emulator
mesen print_string.sfc        # Linux
open -a Mesen print_string.sfc  # macOS
start Mesen.exe print_string.sfc  # Windows
```

> **Note:** `mesen` must be on your `PATH` for the Linux command above. Install
> Mesen from [mesen.ca](https://www.mesen.ca/) and either add its directory to
> `PATH` or alias it (`alias mesen=~/Mesen/Mesen`). No emulator installed? The
> SDK's own `luna` backend also plays ROMs: `luna print_string.sfc` (install it
> once with `scripts/install-luna.sh`).

You should see "Hello World!" on screen.

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

Other commands: `opensnes build`, `opensnes clean`, and `opensnes doctor` (checks
your toolchain, library, and emulator and tells you what is missing). Run
`opensnes help` for the full list.

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
└── test/           # Project tests (optional — see below)
    ├── manifest.toml
    └── baselines.json
```

### Test Your Game

Projects can declare automated tests that run in **luna**, the same
cycle-accurate emulator the SDK's own test suite uses. Opt-in is simply the
presence of `test/manifest.toml` (the `game` template ships one):

```toml
default_steps = 3_000_000

# Visual baseline (fbhash) + WRAM asserts by symbol name.
# Assert values are little-endian hex bytes (an s16 of 120 -> "7800").
[tests.boot]
assert = ["player_x = 7800"]

# Input-driven: hold RIGHT for 60 frames, then check the game state.
# Input format is "frame:buttons_hex" (RIGHT = 0x100).
[tests.walk_right]
input = "30:0x100,90:0"
assert = ["player_x = b400"]
```

Workflow:

```bash
scripts/install-luna.sh   # once, from the SDK root: fetch the pinned luna
make test-update          # seed test/baselines.json + reference PNGs
make test                 # from now on: exit 0 = green, 1 = regression
```

(Or `opensnes test` / `opensnes test --update` from the project directory.)

Three oracles run per test:

- **WRAM asserts** — `symbol = hexbytes` entries are checked by luna
  directly, with symbol names resolved from your ROM's `.sym` file;
- **Visual baselines** — tests *without* an `input` script compare a
  framebuffer hash per capture point (`steps` can be a list for
  multi-point capture); failures leave the actual PNG in `test/actual/`
  for eyeballing;
- **In-ROM assertions** — any `SNES_ASSERT` that fires during a visual
  run fails the test for free.

Commit `test/` to your repo; rerun `make test-update` when you
intentionally change what the game shows or does.

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

This takes a few minutes. Expected output:
```
Building cc65816 compiler...
Building WLA-DX assembler...
Building OpenSNES library...
Building examples... (74 ROMs)
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

See [CLAUDE.md](../CLAUDE.md) for architecture details and coding conventions.

---

## What's Next?

Explore by complexity:

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

The full set (all 19 tutorials, always current) is on the docs home page —
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
| SA-1 Coprocessor | [tutorials/sa1.md](tutorials/sa1.md) |
| SuperFX (GSU) | [tutorials/superfx.md](tutorials/superfx.md) |
| Debugging | [tutorials/debugging.md](tutorials/debugging.md) |

## Troubleshooting

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

### Build fails with "unhandled op" or assembly errors

This is usually a compiler limitation. Check:
- Are you using `u32`/`s32`? Prefer `u16`/`s16` when possible
- Use `u8`, `u16`, `s16`, `u32` types from `snes.h` (not `int` or `long`)

## Getting Help

- **Issues**: [github.com/k0b3n4irb/opensnes/issues](https://github.com/k0b3n4irb/opensnes/issues)
- **SNES Dev Wiki**: [snes.nesdev.org](https://snes.nesdev.org/)

## Which API do I need?

See [API_INDEX.md](API_INDEX.md) — the SDK indexed by *what you are
trying to do*, with the example that does it. Worth a scan before you
write a helper: several already exist.
