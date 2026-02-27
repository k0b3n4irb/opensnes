# OpenSNES Examples

Learn SNES development step by step. Each example teaches specific hardware concepts,
building on what came before.

## Learning Path

### Level 1 — First Steps

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 1 | [text/hello_world](text/hello_world/) | PPU, backgrounds, tiles, palette — your first ROM |
| 2 | [text/text_test](text/text_test/) | Text positioning, formatting, consoleDrawText |
| 3 | [sprites/simple_sprite](graphics/sprites/simple_sprite/) | OAM, sprite display, CGRAM split |
| 4 | [input/two_players](input/two_players/) | Joypad reading, multiplayer input |

### Level 2 — Graphics Fundamentals

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 5 | [backgrounds/mode1](graphics/backgrounds/mode1/) | Mode 1 multi-layer backgrounds |
| 6 | [sprites/animated_sprite](graphics/sprites/animated_sprite/) | Frame animation, sprite sheets, H-flip |
| 7 | [sprites/dynamic_sprite](graphics/sprites/dynamic_sprite/) | VRAM streaming, dynamic tile uploads |
| 8 | [effects/fading](graphics/effects/fading/) | Brightness control, screen transitions |
| 9 | [effects/mosaic](graphics/effects/mosaic/) | Mosaic pixelation effect |

### Level 3 — Advanced Effects

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 10 | [backgrounds/continuous_scroll](graphics/backgrounds/continuous_scroll/) | Streaming background scroll |
| 11 | [effects/hdma_wave](graphics/effects/hdma_wave/) | HDMA scanline effects |
| 12 | [effects/gradient_colors](graphics/effects/gradient_colors/) | HDMA + CGRAM color gradients |
| 13 | [effects/parallax_scrolling](graphics/effects/parallax_scrolling/) | HDMA parallax scrolling |
| 14 | [effects/transparency](graphics/effects/transparency/) | Color math / blending |
| 15 | [effects/window](graphics/effects/window/) | HDMA triangle window masking |
| 16 | [effects/transparent_window](graphics/effects/transparent_window/) | Color math + HDMA transparent rectangle |

### Level 4 — Expert Topics

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 17 | [backgrounds/mode7](graphics/backgrounds/mode7/) | Mode 7 rotation and scaling |
| 18 | [backgrounds/mode7_perspective](graphics/backgrounds/mode7_perspective/) | Pseudo-3D perspective (F-Zero style) |
| 19 | [memory/hirom_demo](memory/hirom_demo/) | HiROM vs LoROM memory mapping |
| 20 | [memory/save_game](memory/save_game/) | SRAM persistence (battery saves) |
| 21 | [audio/snesmod_music](audio/snesmod_music/) | SPC700 music playback |
| 22 | [audio/snesmod_sfx](audio/snesmod_sfx/) | Sound effects |

### Level 5 — Complete Projects

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 23 | [basics/collision_demo](basics/collision_demo/) | Collision detection mechanics |
| 24 | [games/breakout](games/breakout/) | Complete game: sprites, input, game logic |
| 25 | [games/likemario](games/likemario/) | Platformer with scrolling and animation |

---

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
# Open a ROM in Mesen2
mesen examples/text/hello_world/hello_world.sfc
```

Use Mesen's built-in debugger to inspect VRAM, OAM, palettes, and registers in real time.

## Tips

1. **Follow the order** — each example builds on concepts from earlier ones
2. **Read the source** — every `main.c` is commented to explain the "why"
3. **Experiment** — change values, break things, see what happens
4. **Use the debugger** — Mesen2's PPU viewer is invaluable for understanding what's in VRAM

## Resources

- [SNESdev Wiki](https://snes.nesdev.org/wiki/) — Hardware reference
- [fullsnes](https://problemkaputt.de/fullsnes.htm) — Detailed technical documentation
- [OpenSNES Issues](https://github.com/k0b3n4irb/opensnes/issues) — Report bugs or ask questions

---

**Ready?** Start with [text/hello_world](text/hello_world/) and build your first SNES ROM.
