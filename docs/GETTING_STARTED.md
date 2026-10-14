# Getting Started with OpenSNES {#getting_started}

This guide will get you from zero to running your first SNES ROM in about 10 minutes.

## Choose Your Path

| | **I want to make SNES games** | **I want to contribute to the SDK** |
|---|---|---|
| **What** | Download the pre-built SDK, write C code, build ROMs | Clone the repo, modify compiler/library/tools |
| **Prerequisites** | `make`, a host C compiler + text editor | clang, cmake, git, python3 |
| **Time to start** | ~5 minutes | ~15 minutes |
| **Go to** | [Path A: Game Developer](#path-a-game-developer) | [Path B: SDK Developer](#path-b-sdk-developer) |

---

## Path A: Game Developer

You want to write SNES games in C. The SDK is already compiled — you just need
to download it, write code, and run `make`.

### A1. Install Prerequisites

You need `make` (the build tool), a host C compiler and an emulator. The SDK
ships its own cross-compiler for the SNES; the host compiler (`cc`, `clang`
or `gcc`) only lends it its C preprocessor. Nothing else: no Python, no
runtime. `opensnes doctor` checks all of it.

**macOS:**
```bash
xcode-select --install
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install make clang
```

**Linux (Fedora):**
```bash
sudo dnf install make clang
```

**Windows:**
1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 UCRT64** terminal
3. Run: `pacman -S make mingw-w64-ucrt-x86_64-clang`

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
- **`--template game`** — a sprite you steer with the D-pad: the SDK's
  starter project. Its picture, `res/player.png`, has a settings file beside
  it (`res/player.png.toml`) that the build reads to convert it, the model
  for every asset you add; `test/` holds two luna tests that `make test`
  runs.

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

**Makefile:** copy the template of @ref tools_build, which also lists every
variable the build reads, and change the lines that are yours: where the SDK
is, the ROM's name, and the modules this program uses.

```makefile
OPENSNES := /path/to/opensnes        # where you extracted the SDK
TARGET   := my_game.sfc
ROM_NAME := MY GAME
LIB_MODULES := console dma text background
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
~/opensnes/testing/bin/luna-gui my_game.sfc     # or: opensnes run
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

You've built and run a ROM. From here:

- **Follow the route.** @ref learning_path walks the examples as a developer's
  questions — "can I put something on screen?", "can I build a world?" — each
  stage buying real confidence, never a wall you can't climb.
- **Find an example by topic.** @ref examples_by_category lists every example,
  one line each.
- **Find the function.** @ref api_index indexes the SDK by what you are trying
  to do, with the example that does it. Worth a scan before you write a
  helper: several already exist.
- **Make your own assets.** @ref tools turns your PNGs, Tiled maps, fonts, WAVs
  and tracker modules into SNES data.
- **Everything else** — the tutorials, the game-craft guides, the hardware
  reference — is on the docs home page, @ref documentation.

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

### Anything else

A black screen, a compiler that stops with "refusing to emit silently-wrong
code", sprites that do not show: @ref troubleshooting goes from the symptom to
the cause.

## Getting Help

- **Issues**: [github.com/k0b3n4irb/opensnes/issues](https://github.com/k0b3n4irb/opensnes/issues)
- **SNES Dev Wiki**: [snes.nesdev.org](https://snes.nesdev.org/)
