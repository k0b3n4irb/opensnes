# OpenSNES Claude Code Configuration

This directory contains configuration and documentation for Claude Code integration with the OpenSNES SDK.

## Key Documentation

| File | Purpose |
|------|---------|
| **KNOWLEDGE.md** | SNES development knowledge, debugging gotchas, WRAM mirroring |
| **SNES_HARDWARE_REFERENCE.md** | Complete SNES technical reference (CPU, PPU, APU, DMA, cartridge format) |
| **SNES_ROSETTA_STONE.md** | OpenSNES vs PVSnesLib comparison, implementation details |
| **CODE_REVIEW.md** | Code review notes, bug fix history |
| **PVSNESLIB_SPRITES.md** | PVSnesLib sprite system analysis |
| **settings.json** | Claude Code permissions and environment |

## Skills (Slash Commands)

| Command | Description |
|---------|-------------|
| `/build` | Build SDK components |
| `/test` | Run tests |
| `/new-example` | Create new example project |
| `/analyze-rom` | Analyze SNES ROM file |

## Contents

```
.claude/
├── README.md                    # This file
├── KNOWLEDGE.md                 # SNES development knowledge base
├── SNES_HARDWARE_REFERENCE.md   # Complete SNES technical bible (CPU, PPU, APU, DMA, etc.)
├── SNES_ROSETTA_STONE.md        # OpenSNES/PVSnesLib comparison and implementation guide
├── PVSNESLIB_SPRITES.md         # PVSnesLib sprite system deep dive
├── CODE_REVIEW.md               # Code review notes and fix log
├── settings.json                # Claude Code permissions
├── settings.local.json          # Local settings (gitignored)
├── hooks.json                   # Build hooks (disabled)
└── skills/                      # Slash command skills
    ├── analyze-rom.md           # /analyze-rom
    ├── build.md                 # /build
    ├── new-example.md           # /new-example
    ├── test.md                  # /test
    └── spc700-audio.md          # SPC700 audio programming guide
```

## Project Status

OpenSNES is a working SDK with:
- **Compiler**: QBE-based cc65816 (cproc + QBE w65816 backend)
- **Assembler**: WLA-DX (wla-65816, wla-spc700)
- **Library**: Console, input, sprites, text, DMA, audio
- **Audio**: Dual-mode (legacy BRR driver + SNESMOD tracker)
- **Tools**: gfx4snes, font2snes, smconv
- **Examples**: 27 working examples across text, graphics, audio, basics, input, game

## Debugging Tools

- **symmap**: Memory overlap detection (`python3 tools/symmap/symmap.py --check-overlap game.sym`)
- **Mesen2**: Emulator with debugging support

## Usage with Claude Code

Claude Code automatically reads files in this directory. The skills directory provides structured guidance for common tasks.

### Example Session
```
You: /build examples
Claude: Building all examples...
        Built 27 ROMs successfully.

You: /test audio/1_tone
Claude: Building and launching tone.sfc...
```
