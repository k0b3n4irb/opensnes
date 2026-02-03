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
│   ├── 1_hello_world/
│   └── 2_custom_font/
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
| [text/1_hello_world](text/1_hello_world/) | 1 | VRAM, tilemaps, Mode 0, your first ROM |
| [text/2_custom_font](text/2_custom_font/) | 2 | Asset pipeline, 2bpp tiles, font2snes |

### 2. Graphics: Backgrounds
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [graphics/backgrounds/1_mode0](graphics/backgrounds/1_mode0/) | 1 | Mode 0: 4 BG layers, 2bpp |
| [graphics/backgrounds/1_mode1](graphics/backgrounds/1_mode1/) | 1 | Mode 1: 2×4bpp + 1×2bpp |
| [graphics/backgrounds/2_scrolling](graphics/backgrounds/2_scrolling/) | 2 | Basic scrolling |
| [graphics/backgrounds/3_mode3](graphics/backgrounds/3_mode3/) | 3 | 256-color mode |
| [graphics/backgrounds/3_mode5](graphics/backgrounds/3_mode5/) | 3 | Hi-res mode |
| [graphics/backgrounds/3_parallax](graphics/backgrounds/3_parallax/) | 3 | Multi-layer parallax |
| [graphics/backgrounds/4_mode7](graphics/backgrounds/4_mode7/) | 4 | Rotation/scaling |
| [graphics/backgrounds/4_continuous_scroll](graphics/backgrounds/4_continuous_scroll/) | 4 | Dynamic tile loading |

### 3. Graphics: Sprites
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [graphics/sprites/1_simple_sprite](graphics/sprites/1_simple_sprite/) | 1 | OAM, basic sprite display |
| [graphics/sprites/2_animated_sprite](graphics/sprites/2_animated_sprite/) | 2 | Frame animation |
| [graphics/sprites/3_animation_system](graphics/sprites/3_animation_system/) | 3 | State machine animation |
| [graphics/sprites/3_metasprite](graphics/sprites/3_metasprite/) | 3 | Multi-tile sprites |
| [graphics/sprites/4_dynamic_sprite](graphics/sprites/4_dynamic_sprite/) | 4 | VRAM management |

### 4. Graphics: Effects
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [graphics/effects/1_fading](graphics/effects/1_fading/) | 1 | Screen brightness |
| [graphics/effects/2_mosaic](graphics/effects/2_mosaic/) | 2 | Pixelation effect |
| [graphics/effects/2_window](graphics/effects/2_window/) | 2 | Hardware windows |
| [graphics/effects/3_transparency](graphics/effects/3_transparency/) | 3 | Color math |
| [graphics/effects/3_hdma_gradient](graphics/effects/3_hdma_gradient/) | 3 | Per-scanline colors |

### 5. Input
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [input/2_two_players](input/2_two_players/) | 2 | Auto-read, multiplayer input |

### 6. Audio
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [audio/1_tone](audio/1_tone/) | 1 | Basic sound generation |
| [audio/2_sfx](audio/2_sfx/) | 2 | Sound effects |
| [audio/3_snesmod_music](audio/3_snesmod_music/) | 3 | Tracker music |
| [audio/3_snesmod_sfx](audio/3_snesmod_sfx/) | 3 | SNESMOD SFX |

### 7. Memory
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [memory/2_save_game](memory/2_save_game/) | 2 | SRAM saves |
| [memory/3_hirom_demo](memory/3_hirom_demo/) | 3 | HiROM mode |

### 8. Basics
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [basics/2_calculator](basics/2_calculator/) | 2 | Math operations |
| [basics/3_collision_demo](basics/3_collision_demo/) | 3 | Collision detection |
| [basics/3_smooth_movement](basics/3_smooth_movement/) | 3 | Interpolation |

### 9. Complete Games
| Example | Complexity | What You'll Learn |
|---------|------------|-------------------|
| [games/4_breakout](games/4_breakout/) | 4 | Complete game structure |
| [games/4_entity_demo](games/4_entity_demo/) | 4 | Entity system |

---

## Building Examples

```bash
# Build everything
cd opensnes
make

# Build a specific example
cd examples/text/1_hello_world
make

# Clean and rebuild
make clean && make
```

## Running in Emulator

We recommend [Mesen2](https://github.com/SourMesen/Mesen2) for accurate emulation:

```bash
/path/to/Mesen examples/text/1_hello_world/hello_world.sfc
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

**Ready?** Start with [text/1_hello_world](text/1_hello_world/) →
