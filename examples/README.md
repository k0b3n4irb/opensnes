# OpenSNES Tutorial

Welcome to the OpenSNES learning path! This tutorial will guide you from your first "Hello World" to building complete SNES games.

## How to Use This Tutorial

Each example is a **lesson** designed to teach specific concepts. The number prefix indicates complexity:
- `1_` = Beginner (simplest concepts)
- `2_` = Easy (basic usage)
- `3_` = Intermediate (multiple concepts)
- `4_` = Advanced (complex features)

```
examples/
├── text/                    # Start here! Learn the basics
│   ├── hello_world/
│   └── custom_font/
│
├── graphics/
│   ├── backgrounds/         # Background modes and scrolling
│   ├── sprites/             # Sprite handling
│   └── effects/             # Visual effects (HDMA, windows, etc.)
│
├── input/                   # Controller handling
├── audio/                   # Sound effects and music
├── memory/                  # SRAM saves, HiROM mode
├── basics/                  # Utility examples
└── games/                   # Complete game examples
```

## Learning Path

### 1. Start Here: Text
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [text/hello_world](text/hello_world/) | 1 | VRAM, tilemaps, Mode 0, your first ROM |
| [text/custom_font](text/custom_font/) | 2 | Asset pipeline, 2bpp tiles, font2snes |

### 2. Graphics: Backgrounds
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [graphics/backgrounds/mode0](graphics/backgrounds/mode0/) | 1 | Mode 0: 4 BG layers, 2bpp |
| [graphics/backgrounds/mode1](graphics/backgrounds/mode1/) | 1 | Mode 1: 2×4bpp + 1×2bpp |
| [graphics/backgrounds/scrolling](graphics/backgrounds/scrolling/) | 2 | Basic scrolling |
| [graphics/backgrounds/mode3](graphics/backgrounds/mode3/) | 3 | 256-color mode |
| [graphics/backgrounds/mode5](graphics/backgrounds/mode5/) | 3 | Hi-res mode |
| [graphics/backgrounds/parallax](graphics/backgrounds/parallax/) | 3 | Multi-layer parallax |
| [graphics/backgrounds/mode7](graphics/backgrounds/mode7/) | 4 | Rotation/scaling |
| [graphics/backgrounds/continuous_scroll](graphics/backgrounds/continuous_scroll/) | 4 | Dynamic tile loading |

### 3. Graphics: Sprites
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [graphics/sprites/simple_sprite](graphics/sprites/simple_sprite/) | 1 | OAM, basic sprite display |
| [graphics/sprites/animated_sprite](graphics/sprites/animated_sprite/) | 2 | Frame animation |
| [graphics/sprites/animation_system](graphics/sprites/animation_system/) | 3 | State machine animation |
| [graphics/sprites/metasprite](graphics/sprites/metasprite/) | 3 | Multi-tile sprites |
| [graphics/sprites/dynamic_sprite](graphics/sprites/dynamic_sprite/) | 4 | VRAM management |

### 4. Graphics: Effects
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [graphics/effects/fading](graphics/effects/fading/) | 1 | Screen brightness |
| [graphics/effects/mosaic](graphics/effects/mosaic/) | 2 | Pixelation effect |
| [graphics/effects/window](graphics/effects/window/) | 2 | Hardware windows |
| [graphics/effects/transparency](graphics/effects/transparency/) | 3 | Color math |
| [graphics/effects/hdma_gradient](graphics/effects/hdma_gradient/) | 3 | Per-scanline colors |

### 5. Input
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [input/two_players](input/two_players/) | 2 | Auto-read, multiplayer input |

### 6. Audio
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [audio/tone](audio/tone/) | 1 | Basic sound generation |
| [audio/sfx](audio/sfx/) | 2 | Sound effects |
| [audio/snesmod_music](audio/snesmod_music/) | 3 | Tracker music |
| [audio/snesmod_sfx](audio/snesmod_sfx/) | 3 | SNESMOD SFX |

### 7. Memory
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [memory/save_game](memory/save_game/) | 2 | SRAM saves |
| [memory/hirom_demo](memory/hirom_demo/) | 3 | HiROM mode |

### 8. Basics
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [basics/calculator](basics/calculator/) | 2 | Math operations |
| [basics/collision_demo](basics/collision_demo/) | 3 | Collision detection |
| [basics/smooth_movement](basics/smooth_movement/) | 3 | Interpolation |

### 9. Complete Games
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [games/breakout](games/breakout/) | 4 | Complete game structure |
| [games/entity_demo](games/entity_demo/) | 4 | Entity system |

---

## Building Examples

```bash
# Build everything
cd opensnes
make

# Build a specific example
cd examples/text/hello_world
make

# Clean and rebuild
make clean && make
```

## Running in Emulator

We recommend [Mesen2](https://github.com/SourMesen/Mesen2) for accurate emulation:

```bash
/path/to/Mesen examples/text/hello_world/hello_world.sfc
```

---

## Tips for Learning

1. **Read the code comments** - They explain the "why"
2. **Experiment** - Change values and see what happens
3. **Use Mesen's debugger** - View VRAM, OAM, registers in real-time
4. **Follow the complexity order** - Start with 1_, then 2_, etc.

## Getting Help

- [SNESdev Wiki](https://snes.nesdev.org/wiki/) - Hardware reference
- [SNESdev Discord](https://discord.gg/snesdev) - Community help
- [OpenSNES Issues](https://github.com/opensnes/opensnes/issues) - Report bugs

---

**Ready?** Start with [text/hello_world](text/hello_world/) →
