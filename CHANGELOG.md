# Changelog

All notable changes to OpenSNES are documented in this file.

OpenSNES is forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib). This changelog
covers changes made since the fork.

## [Unreleased]

### Compiler (cc65816 — QBE w65816 backend)

- **-31.3% cycle count** vs PVSnesLib + 816-opt on benchmark suite
- 13 optimization phases: dead jump elimination, A-register cache, 8/16-bit mode tracking,
  leaf and non-leaf function optimization, comparison+branch fusion, dead store elimination,
  tail call optimization, and more
- Fixed unsigned integer promotion (u16 comparisons with values >= 32768)
- Fixed signed right shift (`>>` on negative values)
- Fixed function return values being clobbered by epilogue
- Fixed `__mul16` calling convention mismatch
- Added inline multiply for *3, *5, *6, *7, *9, *10
- String literals placed in ROM (saves WRAM)
- `pea.w` for constant arguments
- `.l` to `.w` address shortening for bank $00 symbols
- `stz` for zero stores to global symbols

### Library

- New modules: `mosaic`, `window`, `colormath`, `collision`, `entity`, `animation`, `sram`, `mode7`
- Modernized API with Doxygen documentation on all public headers
- `consoleInit()` sets sensible BG1 defaults (no extra setup needed for simple programs)
- Fixed CopyInitData 16-bit byte overrun (corrupted initialized static variables)
- HDMA support with direct and indirect tables

### Examples

- 23 working examples covering all subsystems:
  - **Games**: Breakout, LikeMario (platformer with scrolling)
  - **Graphics**: sprites (simple, dynamic, animated), backgrounds (Mode 1, Mode 7,
    Mode 7 perspective, continuous scroll), effects (fading, HDMA wave, gradient colors,
    transparency, window, mosaic)
  - **Audio**: SNESMOD music, sound effects
  - **Input**: two-player joypad
  - **Memory**: SRAM save/load, HiROM demo
  - **Text**: hello world, text formatting
  - **Basics**: collision demo

### Build System

- Unified `make/common.mk` for all examples
- HiROM support (`USE_HIROM=1`)
- SRAM support (`USE_SRAM=1`)
- Library module selection (`LIB_MODULES=console sprite dma`)
- `make release` for SDK packaging

### Testing

- 50 compiler regression tests (`tests/compiler/run_tests.sh`)
- Example validation with memory overlap checking (`tests/examples/validate_examples.sh`)
- CI pipeline on Linux, macOS, and Windows (MSYS2)

### Documentation

- Doxygen-documented public API
- Example READMEs with hardware explanations
- Progressive learning path from hello world to full games
