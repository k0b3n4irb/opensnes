# OpenSNES Examples

The example corpus: 88 small ROMs, each a folder with its `main.c`, its
assets, a `Makefile` and a `README.md` that says what it shows. They go from
a first line of text to complete games.

- **In what order?** [Learning Path](../docs/LEARNING_PATH.md) is the route
  through them, stage by stage.
- **Every example, by topic:** [Examples by Category](../docs/EXAMPLES_BY_CATEGORY.md)
  is the full catalogue, one line each.

The table below counts them per folder.

## Categories

| Category | Examples | What It Covers |
|----------|----------|----------------|
| [text/](text/) | 3 | Text display, fonts, tilemaps |
| [fundamentals/](fundamentals/) | 1 | Under-the-hood: raw tiles, direct VRAM writes |
| [basics/](basics/) | 7 | Collision, timing, scene stack, randomness, fixed-point, aiming, HUD panels, game skeleton |
| [backgrounds/](backgrounds/) | 9 | BG modes 0/1/2/3/4/5/6, priority, LZ77, hi-res, offset-per-tile |
| [sprites/](sprites/) | 8 | Sprite display, animation, OAM, metasprites, VRAM streaming, swarm, Aseprite pipeline |
| [hdma/](hdma/) | 4 | Per-scanline HDMA effects: gradients, waves, raster |
| [color/](color/) | 6 | Palette cycling, colour math, shadow/tint, direct colour, HiColor, pseudo-hires blend |
| [windows/](windows/) | 3 | Hardware window masking, shaped per scanline |
| [transitions/](transitions/) | 2 | Screen transitions: fade, mosaic pixelate |
| [scrolling/](scrolling/) | 3 | Layer scrolling: parallax, streaming, per-scanline HDMA |
| [mode7/](mode7/) | 5 | Mode 7: rotation, scaling, per-scanline perspective, DSP-1 ground, EXTBG |
| [input/](input/) | 4 | Joypads, mouse, Super Scope, multi-player |
| [audio/](audio/) | 9 | Music and sound effects: SNESMOD and raw APU/DSP |
| [maps/](maps/) | 3 | Tile maps, dynamic streaming, slopes |
| [memory/](memory/) | 2 | HiROM mode, battery-backed saves |
| [chips/](chips/) | 8 | Enhancement chips: SA-1, SuperFX/GSU and DSP-1 coprocessors |
| [games/](games/) | 8 | Complete game projects (falling blocks, Breakout, Mario-like, map+objects, 1942-style shmup, Mode 7 racing + flying, Tiled-driven RPG) |

## Building

```bash
# Build all examples
cd opensnes
make

# Build a single example
make -C examples/text/print_string

# Clean and rebuild
make clean && make
```

## Running

luna, the SDK's emulator (`scripts/install-luna.sh` installs it), plays any
example in a window:

```bash
testing/bin/luna-gui examples/text/print_string/print_string.sfc
```

The headless `luna` dumps VRAM, OAM, palettes and registers
(`luna assets-dump`, `luna state`) and is a full debugger over MCP: see
[Debugging](../docs/tutorials/debugging.md). Any other SNES emulator works too.

## Tips

1. **Read the source** -- every `main.c` is commented to explain the "why"
2. **Experiment** -- change values, break things, see what happens
3. **Look at the machine** -- `luna assets-dump` shows what the code put in VRAM, OAM and the palettes
