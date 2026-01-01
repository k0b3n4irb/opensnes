# OpenSNES

An open source SDK for Super Nintendo (SNES) development.

> **Status**: Early Development - Not yet usable

## Vision

OpenSNES is a modern, open source SDK for creating SNES games. It aims to provide:

- **Clean C Compiler**: QBE-based 65816 backend (in development)
- **Robust Library**: Battle-tested assembly routines for SNES hardware
- **Real Game Templates**: Not just demos, but actual game starting points
- **Modern Tooling**: YAML configs, asset pipelines, IDE integration

## Lineage

OpenSNES is a spiritual successor to [PVSnesLib](https://github.com/alekmaul/pvsneslib) by Alekmaul. We build upon the SNES homebrew community's years of work while taking a fresh approach to architecture and tooling.

See [ATTRIBUTION.md](ATTRIBUTION.md) for full credits.

## Project Status

| Component | Status | Notes |
|-----------|--------|-------|
| Library | Planned | Porting essential routines from PVSnesLib |
| Compiler (816-tcc) | Temporary | Using existing compiler initially |
| Compiler (QBE) | Research | New 65816 backend in planning |
| Tools | Planned | Graphics, audio, map converters |
| Templates | Planned | Platformer, RPG, Shmup starters |
| Documentation | In Progress | Architecture docs being written |

## Roadmap

### Phase 1: Foundation
- [ ] Minimal working SDK (library + existing compiler)
- [ ] One complete game template (platformer)
- [ ] Comprehensive test suite
- [ ] Core documentation

### Phase 2: Compiler
- [ ] QBE 65816 backend development
- [ ] Optimization passes for SNES constraints
- [ ] Replace 816-tcc with QBE

### Phase 3: Ecosystem
- [ ] Game engine with component system
- [ ] VSCode extension
- [ ] Asset pipeline with hot reload
- [ ] Community template repository

## Building

```bash
# Not yet available - check back soon!
```

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
