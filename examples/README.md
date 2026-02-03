# OpenSNES Tutorial

Welcome to the OpenSNES learning path! This tutorial will guide you from your first "Hello World" to building complete SNES games.

## How to Use This Tutorial

Each example is a **lesson** designed to teach specific concepts. Work through them in order within each topic - they build on each other.

```
📁 examples/
├── 📁 text/          ← Start here! Learn the basics
├── 📁 basics/        ← Core concepts (HiROM, SRAM)
├── 📁 graphics/      ← Sprites, backgrounds, effects
├── 📁 input/         ← Controller handling
├── 📁 audio/         ← Sound effects and music
└── 📁 game/          ← Complete game examples
```

## Learning Path

### 🎯 Beginner: Text & Basics
**Goal:** Understand SNES architecture and display text on screen.

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 1 | [text/1_hello_world](text/1_hello_world/) | VRAM, tilemaps, Mode 0, your first ROM |
| 2 | [text/2_custom_font](text/2_custom_font/) | Asset pipeline, 2bpp tiles, font2snes tool |

### 🎮 Intermediate: Graphics
**Goal:** Display and animate sprites, create backgrounds.

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 1 | [graphics/6_simple_sprite](graphics/6_simple_sprite/) | OAM, sprites, 4bpp tiles, controller input |
| 2 | [graphics/21_animated_sprite](graphics/21_animated_sprite/) | Frame animation, timing |
| 3 | [graphics/4_scrolling](graphics/4_scrolling/) | Tilemaps, scrolling |
| 4 | [graphics/5_mode1](graphics/5_mode1/) | Mode 1 backgrounds |
| 5 | [graphics/9_parallax](graphics/9_parallax/) | Parallax scrolling effects |

See [graphics/README.md](graphics/README.md) for the complete list of 20+ graphics examples.

### 🕹️ Intermediate: Input
**Goal:** Handle player input from controllers.

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 1 | [input/2_two_players](input/2_two_players/) | Auto-read, button states, multiplayer input |

### 🔊 Advanced: Audio
**Goal:** Add sound effects and music.

| # | Example | What You'll Learn |
|---|---------|-------------------|
| 1 | [audio/1_tone](audio/1_tone/) | SPC700, BRR samples, tone generation |
| 2 | [audio/2_sfx](audio/2_sfx/) | Sound effects |
| 3 | [audio/6_snesmod_music](audio/6_snesmod_music/) | Tracker music with SNESMOD |
| 4 | [audio/7_snesmod_sfx](audio/7_snesmod_sfx/) | SNESMOD sound effects |

---

## SNES Architecture Overview

Before diving in, here's what you need to know about the SNES:

### The Two Processors

```
┌─────────────────┐     ┌─────────────────┐
│   65816 CPU     │     │   SPC700 APU    │
│  (Main CPU)     │────▶│  (Audio CPU)    │
│   ~3.58 MHz     │     │   ~1.024 MHz    │
└────────┬────────┘     └─────────────────┘
         │
         ▼
┌─────────────────┐
│      PPU        │
│ (Picture Proc.) │
│  Video output   │
└─────────────────┘
```

### Memory You'll Use

| Memory | Size | Purpose |
|--------|------|---------|
| **VRAM** | 64 KB | Tile graphics, tilemaps |
| **CGRAM** | 512 bytes | Color palettes (256 colors) |
| **OAM** | 544 bytes | Sprite attributes (128 sprites) |
| **WRAM** | 128 KB | Your variables, game state |

### Display Basics

- **Resolution:** 256×224 pixels (NTSC) or 256×239 (PAL)
- **Colors:** 256 colors on screen (from 32,768 possible)
- **Sprites:** Up to 128 sprites, 32 per scanline
- **Backgrounds:** Up to 4 layers depending on mode

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
# Open ROM in Mesen
/path/to/Mesen examples/text/1_hello_world/hello_world.sfc
```

---

## Tips for Learning

1. **Read the code comments** - They explain the "why"
2. **Experiment** - Change values and see what happens
3. **Use Mesen's debugger** - View VRAM, OAM, registers in real-time
4. **One concept at a time** - Don't skip ahead

## Getting Help

- 📖 [SNESdev Wiki](https://snes.nesdev.org/wiki/) - Hardware reference
- 💬 [SNESdev Discord](https://discord.gg/snesdev) - Community help
- 🐛 [OpenSNES Issues](https://github.com/opensnes/opensnes/issues) - Report bugs

---

**Ready?** Start with [text/1_hello_world](text/1_hello_world/) →
