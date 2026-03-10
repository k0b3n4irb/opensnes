# OpenSNES Documentation

Welcome to the OpenSNES documentation.

## For Users

| Document | Description |
|----------|-------------|
| [Getting Started](GETTING_STARTED.md) | Quick start guide — zero to first ROM |
| [Example Walkthroughs](EXAMPLE_WALKTHROUGHS.md) | Line-by-line code explanations |
| [SNES Graphics Guide](SNES_GRAPHICS_GUIDE.md) | PPU, tiles, palettes, modes |
| [SNES Sound Guide](SNES_SOUND_GUIDE.md) | SPC700, BRR samples, SNESMOD |
| [Troubleshooting](TROUBLESHOOTING.md) | Common problems and solutions |

### Tutorials

| Tutorial | Topic |
|----------|-------|
| [Graphics & Backgrounds](tutorials/graphics.md) | Tilemaps, BG modes, scrolling |
| [Sprites & Animation](tutorials/sprites.md) | OAM, sprite sheets, animation |
| [Controller Input](tutorials/input.md) | Joypad reading, button states |
| [Audio & Music](tutorials/audio.md) | SNESMOD tracker playback |

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
| [Maturity Review](MATURITY_REVIEW.md) | OpenSNES vs PVSnesLib comparison and v1.0 roadmap |
| [Code Style](CODE_STYLE.md) | Coding standards |
| [Third Party](THIRD_PARTY.md) | Attribution and licenses |

## Documentation Standards

All documentation should:

1. **Be clear and concise** - Write for developers new to SNES
2. **Include examples** - Show, don't just tell
3. **Stay current** - Update when code changes
4. **Cross-reference** - Link to related docs
