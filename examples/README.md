# OpenSNES Examples

Learn SNES development step by step. 54 examples organized by topic, building
from basic concepts to complete games.

## Categories

| Category | Examples | What It Covers |
|----------|----------|----------------|
| [text/](text/) | 2 | Text display, fonts, tilemaps |
| [basics/](basics/) | 4 | Collision, timing, scene stack, randomness |
| [graphics/](graphics/) | 26 | Backgrounds, sprites, visual effects, Mode 7 |
| [input/](input/) | 4 | Joypads, mouse, Super Scope, multi-player |
| [audio/](audio/) | 4 | Music and sound effects via SNESMOD |
| [maps/](maps/) | 4 | Tile maps, dynamic streaming, slopes |
| [memory/](memory/) | 5 | HiROM mode, battery-backed saves, SA-1, SuperFX |
| [games/](games/) | 4 | Complete game projects (Tetris, Breakout, Mario-like, map+objects) |

## Learning Path

### Level 1 -- First Steps

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 1 | [text/hello_world](text/hello_world/) | PPU, backgrounds, tiles, palette -- your first ROM |
| 2 | [text/text_test](text/text_test/) | Text positioning, formatting, consoleDrawText |
| 3 | [graphics/sprites/simple_sprite](graphics/sprites/simple_sprite/) | OAM, sprite display, CGRAM split |
| 4 | [input/two_players](input/two_players/) | Joypad reading, multiplayer input |

### Level 2 -- Graphics Fundamentals

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 5 | [graphics/backgrounds/mode1](graphics/backgrounds/mode1/) | Mode 1 multi-layer backgrounds |
| 6 | [graphics/backgrounds/mode1_bg3_priority](graphics/backgrounds/mode1_bg3_priority/) | BG3 priority bit in Mode 1 |
| 7 | [graphics/backgrounds/mode1_lz77](graphics/backgrounds/mode1_lz77/) | LZ77-compressed background data |
| 8 | [graphics/sprites/animated_sprite](graphics/sprites/animated_sprite/) | Frame animation, sprite sheets, H-flip |
| 9 | [graphics/sprites/dynamic_sprite](graphics/sprites/dynamic_sprite/) | VRAM streaming, dynamic tile uploads |
| 10 | [graphics/sprites/object_size](graphics/sprites/object_size/) | OBJSEL sprite size configurations |
| 11 | [graphics/effects/fading](graphics/effects/fading/) | Brightness control, screen transitions |
| 12 | [graphics/effects/mosaic](graphics/effects/mosaic/) | Mosaic pixelation effect |

### Level 3 -- Scrolling and Effects

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 13 | [graphics/backgrounds/continuous_scroll](graphics/backgrounds/continuous_scroll/) | Streaming background scroll with dynamic tile loading |
| 14 | [graphics/backgrounds/mixed_scroll](graphics/backgrounds/mixed_scroll/) | Multiple BG layers scrolling at different rates |
| 15 | [graphics/effects/hdma_wave](graphics/effects/hdma_wave/) | HDMA scanline wave distortion |
| 16 | [graphics/effects/gradient_colors](graphics/effects/gradient_colors/) | HDMA + CGRAM color gradients |
| 17 | [graphics/effects/parallax_scrolling](graphics/effects/parallax_scrolling/) | HDMA parallax scrolling |
| 18 | [graphics/effects/transparency](graphics/effects/transparency/) | Color math (add/subtract blending) |
| 19 | [graphics/effects/window](graphics/effects/window/) | Hardware window masking |
| 20 | [graphics/effects/transparent_window](graphics/effects/transparent_window/) | Color math + HDMA windowed transparency |

### Level 4 -- Advanced Topics

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 21 | [graphics/backgrounds/mode7](graphics/backgrounds/mode7/) | Mode 7 rotation and scaling |
| 22 | [graphics/backgrounds/mode7_perspective](graphics/backgrounds/mode7_perspective/) | Pseudo-3D perspective (F-Zero style) |
| 23 | [graphics/sprites/metasprite](graphics/sprites/metasprite/) | Multi-tile composite sprites |
| 24 | [input/mouse](input/mouse/) | Mouse detection, cursor, sensitivity |
| 25 | [input/superscope](input/superscope/) | Light gun detection, PPU H/V counters |
| 26 | [memory/hirom_demo](memory/hirom_demo/) | HiROM vs LoROM memory mapping |
| 27 | [memory/save_game](memory/save_game/) | SRAM persistence (battery saves) |
| 28 | [audio/snesmod_music](audio/snesmod_music/) | SPC700 music playback via SNESMOD |
| 29 | [audio/snesmod_sfx](audio/snesmod_sfx/) | Sound effects via SNESMOD |

### Level 5 -- Maps and Complete Projects

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 30 | [maps/dynamic_map](maps/dynamic_map/) | Dynamic tile map streaming |
| 31 | [maps/slopemario](maps/slopemario/) | Slopes and tile-based collision |
| 32 | [basics/collision_demo](basics/collision_demo/) | Bounding-box sprite collision |
| 33 | [games/breakout](games/breakout/) | Complete game: sprites, input, game logic |
| 34 | [games/likemario](games/likemario/) | Platformer with scrolling and animation |
| 35 | [games/mapandobjects](games/mapandobjects/) | Maps with interactive objects |

## Building

```bash
# Build all examples
cd opensnes
make

# Build a single example
make -C examples/text/hello_world

# Clean and rebuild
make clean && make
```

## Running

We recommend [Mesen2](https://github.com/SourMesen/Mesen2) for accurate SNES emulation:

```bash
mesen examples/text/hello_world/hello_world.sfc
```

Use Mesen's built-in debugger to inspect VRAM, OAM, palettes, and registers in real time.

## Tips

1. **Follow the order** -- each example builds on concepts from earlier ones
2. **Read the source** -- every `main.c` is commented to explain the "why"
3. **Experiment** -- change values, break things, see what happens
4. **Use the debugger** -- Mesen2's PPU viewer is invaluable for understanding VRAM

---

**Ready?** Start with [text/hello_world](text/hello_world/) and build your first SNES ROM.
