# OpenSNES Documentation

Welcome to the OpenSNES documentation.

## For Users

| Document | Description |
|----------|-------------|
| [Getting Started](GETTING_STARTED.md) | Quick start guide — zero to first ROM |
| [SNES Graphics Guide](SNES_GRAPHICS_GUIDE.md) | PPU, tiles, palettes, modes |
| [SNES Sound Guide](SNES_SOUND_GUIDE.md) | SPC700, BRR samples, SNESMOD |
| [Troubleshooting](TROUBLESHOOTING.md) | Common problems and solutions |

### Tutorials

| Tutorial | Topic |
|----------|-------|
| [Graphics & Backgrounds](tutorials/graphics.md) | Tilemaps, BG modes, scrolling |
| [Sprites & Animation](tutorials/sprites.md) | OAM, sprite sheets, animation |
| [Scrolling & Parallax](tutorials/scrolling.md) | Map scrolling, parallax layers |
| [Collision Detection](tutorials/collision.md) | Tile and sprite collision |
| [Controller Input](tutorials/input.md) | Joypad reading, button states |
| [Audio & Music](tutorials/audio.md) | SNESMOD tracker playback |
| [Game States](tutorials/game_states.md) | State machines, transitions |
| [SA-1 Coprocessor](tutorials/sa1.md) | 10.74 MHz second CPU, shared I-RAM |

### Hardware Reference

| Document | Description |
|----------|-------------|
| [Hardware Overview](hardware/README.md) | SNES specs, key concepts |
| [Memory Map](hardware/MEMORY_MAP.md) | RAM, ROM, and register layout |
| [Registers](hardware/REGISTERS.md) | Complete hardware register reference |
| [OAM](hardware/OAM.md) | Sprite attribute memory documentation |

### API Reference

```bash
# Generate API docs from source (requires Doxygen)
make docs

# View locally
open docs/build/html/index.html
```

## For Contributors

| Document | Description |
|----------|-------------|
| [Benchmark](BENCHMARK.md) | Compiler performance: OpenSNES vs PVSnesLib (-30% cycles) |
| [Code Style](CODE_STYLE.md) | Coding standards |
| [Third Party](THIRD_PARTY.md) | Attribution and licenses |

## Documentation Standards

All documentation should:

1. **Be clear and concise** - Write for developers new to SNES
2. **Include examples** - Show, don't just tell
3. **Stay current** - Update when code changes
4. **Cross-reference** - Link to related docs
