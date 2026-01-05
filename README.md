# OpenSNES

An open source SDK for Super Nintendo (SNES) development.

> **Status**: Early Development - Hello World working!

## Vision

OpenSNES is a modern, open source SDK for creating SNES games. It provides:

- **Clean C Compiler**: QBE-based 65816 backend (cc65816)
- **Robust Assembler**: WLA-DX for 65816 and SPC700
- **Real Game Templates**: Not just demos, but actual game starting points
- **Modern Tooling**: Asset converters, build system

## Lineage

OpenSNES is a spiritual successor to [PVSnesLib](https://github.com/alekmaul/pvsneslib) by Alekmaul. We build upon the SNES homebrew community's years of work while taking a fresh approach to architecture and tooling.

See [ATTRIBUTION.md](ATTRIBUTION.md) for full credits.

## Project Status

| Component | Status | Notes |
|-----------|--------|-------|
| Compiler (cc65816) | Working | cproc + QBE w65816 backend |
| Assembler (WLA-DX) | Working | wla-65816, wla-spc700, wlalink |
| Tools | In Progress | font2snes, gfx2snes |
| Library | Planned | SNES hardware abstraction |
| Templates | Planned | Platformer, RPG, Shmup starters |

## Building

```bash
# Clone with submodules
git clone --recursive https://github.com/yourusername/opensnes.git
cd opensnes

# Build compiler and tools
make

# Build examples
make examples

# Build a single example
cd examples/text/1_hello_world
make
```

## Roadmap

### Phase 1: Foundation (Current)
- [x] QBE 65816 backend
- [x] Hello World example working
- [ ] Minimal library (PPU, input, sound)
- [ ] One complete game template (platformer)

### Phase 2: Tools
- [ ] Asset pipeline (graphics, tilemaps, audio)
- [ ] Hot reload development mode
- [ ] Comprehensive test suite

### Phase 3: Ecosystem
- [ ] Game engine with component system
- [ ] VSCode extension
- [ ] Community template repository

## Contributing

OpenSNES is in early development. If you're interested in contributing:

1. Read [ATTRIBUTION.md](ATTRIBUTION.md) for code contribution guidelines
2. Check the issues for "good first issue" labels
3. Join discussions about architecture decisions

## License

MIT License - See [LICENSE](LICENSE)

## Acknowledgments

- **Alekmaul** and PVSnesLib contributors for pioneering SNES C development
- **Quentin Carbonneaux** for QBE compiler infrastructure
- **SNESdev community** for decades of hardware documentation
- All the SNES homebrew developers who came before us
