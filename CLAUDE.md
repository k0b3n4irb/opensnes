# CLAUDE.md - OpenSNES Development Guide

This file provides guidance to Claude Code when working with OpenSNES.

## Project Overview

OpenSNES is an open source SDK for SNES game development. It is a spiritual successor to PVSnesLib with goals of:
- Clean QBE-based C compiler for 65816
- Real game templates (not just demos)
- Modern tooling and documentation

## Project Status

**Current Phase**: Foundation (setting up project structure)

## Directory Structure

```
opensnes/
├── compiler/
│   ├── qbe/           # QBE 65816 backend (in development)
│   └── tcc/           # Temporary: 816-tcc for initial builds
├── lib/
│   ├── include/       # Public headers
│   └── source/        # Assembly and C library source
├── tools/             # Asset conversion tools
│   ├── gfx/           # Graphics converters
│   ├── audio/         # Sound/music tools
│   └── map/           # Tilemap converters
├── templates/         # Game starter templates
│   ├── platformer/    # Side-scrolling platformer
│   ├── rpg/           # Top-down RPG
│   └── shmup/         # Shoot-em-up
├── tests/             # Automated test suite
├── docs/              # Documentation
└── .claude/           # Claude Code configuration
```

## Build Commands

```bash
# Set environment
export OPENSNES_HOME=/path/to/opensnes

# Build everything (when ready)
make

# Build specific template
cd templates/platformer && make

# Run tests
cd tests && ./run_tests.sh
```

## Development Phases

### Phase 1: Foundation (Current)
- Set up project structure
- Port minimal library from PVSnesLib (with attribution)
- Create first game template (platformer)
- Establish test infrastructure

### Phase 2: QBE Compiler
- Study QBE architecture
- Design 65816 backend
- Implement code generation
- Test against real game code

### Phase 3: Ecosystem
- Game engine with components
- VSCode extension
- Asset pipeline
- Community templates

## Technical Notes

### SNES Hardware Quick Reference
- **CPU**: 65816 (16-bit mode) @ 3.58 MHz
- **RAM**: 128KB Work RAM
- **VRAM**: 64KB for graphics
- **OAM**: 128 sprites, 544 bytes
- **Resolution**: 256x224 (NTSC), 256x239 (PAL)

### 65816 Considerations
- 16-bit accumulator and index registers (when in 16-bit mode)
- 24-bit address space (but only 64KB per bank)
- Direct page for fast variable access
- No hardware multiply/divide (use lookup tables or software)

### Code Attribution
When porting code from PVSnesLib or other sources:
1. Add header comment with source URL and author
2. Update ATTRIBUTION.md
3. Document any modifications made

## Context7 MCP Resources

Available documentation via Context7:
- `/websites/snes_nesdev_wiki` - SNES hardware documentation
- `/alekmaul/pvsneslib` - PVSnesLib examples (reference only)
- `/websites/cc65_github_io_doc` - 6502/65816 assembly reference

## Code Style

### C Code
- Use fixed-width types: `u8`, `u16`, `s16`, `s32`
- Keep functions small (SNES has limited stack)
- Prefer direct page variables for hot paths
- Document register clobbering in inline assembly

### Assembly (WLA-DX syntax)
```asm
; Function: myFunction
; Input: A = parameter
; Output: A = result
; Clobbers: X, Y
.section ".text"
myFunction:
    rep #$20        ; 16-bit accumulator
    ; ... code ...
    rtl
.ends
```

## Testing

Every feature should have:
1. Unit test (ROM that tests the feature)
2. Build validation (compiles without errors)
3. Documentation example

## Attribution Requirements

All code derived from external sources must:
1. Have source URL in file header
2. Be listed in ATTRIBUTION.md
3. Have compatible license (MIT, BSD, CC0)
