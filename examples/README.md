# OpenSNES Examples

Learn SNES development step by step. Each example teaches specific hardware concepts,
building on what came before.

## Learning Path

### Level 1 — First Steps

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 1 | [text/hello_world](text/hello_world/) | PPU, backgrounds, tiles, palette — your first ROM |
| 2 | [sprites/simple_sprite](graphics/sprites/simple_sprite/) | OAM, sprite display, CGRAM split |
| 3 | [input/two_players](input/two_players/) | Joypad reading, multiplayer input |

### Level 2 — Graphics Fundamentals

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 4 | [backgrounds/mode1](graphics/backgrounds/mode1/) | Mode 1 multi-layer backgrounds |
| 5 | [sprites/animated_sprite](graphics/sprites/animated_sprite/) | Frame animation, sprite sheets, H-flip |
| 6 | [sprites/dynamic_sprite](graphics/sprites/dynamic_sprite/) | VRAM streaming, dynamic tile uploads |
| 7 | [effects/fading](graphics/effects/fading/) | Brightness control, screen transitions |
| 8 | [effects/mosaic](graphics/effects/mosaic/) | Mosaic pixelation effect |

### Level 3 — Advanced Effects

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 9 | [backgrounds/continuous_scroll](graphics/backgrounds/continuous_scroll/) | Streaming background scroll |
| 10 | [effects/hdma_wave](graphics/effects/hdma_wave/) | HDMA scanline effects |
| 11 | [effects/gradient_colors](graphics/effects/gradient_colors/) | HDMA + CGRAM color gradients |
| 12 | [effects/transparency](graphics/effects/transparency/) | Color math / blending |
| 13 | [effects/window](graphics/effects/window/) | Hardware window masking |

### Level 4 — Expert Topics

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 14 | [backgrounds/mode7](graphics/backgrounds/mode7/) | Mode 7 rotation and scaling |
| 15 | [backgrounds/mode7_perspective](graphics/backgrounds/mode7_perspective/) | Pseudo-3D perspective (F-Zero style) |
| 16 | [memory/hirom_demo](memory/hirom_demo/) | HiROM vs LoROM memory mapping |
| 17 | [memory/save_game](memory/save_game/) | SRAM persistence (battery saves) |
| 18 | [audio/snesmod_music](audio/snesmod_music/) | SPC700 music playback |
| 19 | [audio/snesmod_sfx](audio/snesmod_sfx/) | Sound effects |

### Level 5 — Complete Projects

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 20 | [basics/collision_demo](basics/collision_demo/) | Collision detection mechanics |
| 21 | [games/breakout](games/breakout/) | Complete game: sprites, input, game logic |
| 22 | [games/likemario](games/likemario/) | Platformer with scrolling and animation |

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
- [OpenSNES Issues](https://github.com/opensnes/opensnes/issues) — Report bugs or ask questions

---

**Ready?** Start with [text/hello_world](text/hello_world/) and build your first SNES ROM.
