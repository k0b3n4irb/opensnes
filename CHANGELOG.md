# Changelog

All notable changes to OpenSNES are documented in this file.

OpenSNES is forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib). This changelog
covers changes made since the fork.

## [0.7.1] - 2026-03-10

### Build System

- **WLA-DX submodule updated** to latest upstream (42 commits) — assembler/linker
  improvements and bug fixes.
- **Fixed ASCIITABLE warnings**: moved `.ASCIITABLE` definition from `crt0.asm` into
  all memmap include files so every assembly unit gets the identity ASCII mapping.
- **Added `.ACCU 16` / `.INDEX 16`** to `runtime.asm` for correct WLA-DX register
  width tracking in math functions.
- **Release asset filenames now include version tag**
  (e.g. `opensnes_v0.7.1_linux_x86_64.zip`).

### Tooling

- **VS Code project configuration**: shared `settings.json` (file associations,
  IntelliSense), `tasks.json` (12 build/test tasks), `extensions.json`
  (WLA-DX syntax + C/C++ recommended).

### Dependencies

- **Updated vendored lodepng** from 20230410 to 20260119 in gfx4snes and img2snes.

### Housekeeping

- Added `.gitattributes` to fix GitHub language detection (all `.h` → C).
- Removed accidentally tracked `CLAUDE.md` files from repository.
- Updated `ATTRIBUTION.md` with complete dependency and contributor credits.

## [0.7.0] - 2026-03-10

### CI/CD

- **Pre-built binary releases**: new `release.yml` workflow triggered on version tags.
  Builds SDK on Linux, macOS, and Windows, runs tests, creates GitHub Release with
  platform zips attached automatically.
- Release zip filenames now include architecture (`opensnes_linux_x86_64.zip`,
  `opensnes_darwin_arm64.zip`, `opensnes_windows_x86_64.zip`).
- Build workflow skips tag pushes (handled by release workflow).

### Documentation

- **opensnes-emu debug emulator (single source of truth for testing)
  of OpenSNES vs PVSnesLib across 8 dimensions.
- **Published benchmark** (`docs/BENCHMARK.md`): 34-function comparison showing
  -30.3% total cycles vs PVSnesLib + 816-opt (32/34 function wins).
- Updated ROADMAP from v0.3.0 to v0.6.0 with structured v1.0 milestones.
- Updated README: beta status badge, comparison table, accurate counts (37 examples,
  28 headers, 60 compiler tests, 25 unit modules).
- Getting started guide now offers pre-built SDK download as primary option.

## [0.6.0] - 2026-03-09

### Compiler

- **Signed division and modulo**: emit `__sdiv16` / `__smod16` for signed `/` and `%`
  operators.
- **Fixed cproc `mktype()` uninitialized fields**: garbage in `type->qual` could cause
  mutable structs to be emitted as `.rodata` (ROM). Fixed by initializing all fields.
- Eliminated redundant loads in phi-move A-cache.

### Library

- **HDMA effect helpers**: new high-level library functions for wave, ripple, iris wipe,
  brightness gradient (`hdmaWaveEffect`, `hdmaBrightnessGradient`, etc.).
- Fixed `oamDrawMeta` return value and metasprite mode switching.
- Migrated color gradient HDMA to bank $00 RAM.
- Double-buffer ripple mode fix and edge wrapping.
- Iris tables moved to bank $00 for correct HDMA bank byte.

### Runtime

- Signed 16-bit division and modulo (`sdiv16` / `smod16`) in `runtime.asm`.
- Division-by-zero guard and X register preservation in software division.

### Examples

- HDMA helpers demo example.
- Migrated hdma_wave and hdma_gradient to library helpers.
- Fixed continuous_scroll: replaced fragile nmiSetBank callback with bgSetScroll.

### Build System

- Flattened `templates/common/` to `templates/`.
- Removed dead startup code from crt0.asm and libsnes.asm.

## [0.5.0] - 2026-03-08

### Compiler

- Fixed memory leaks in cproc (cleanup functions added).

## [0.4.0] - 2026-03-07

### Toolchain

- **smconv rewritten in pure C** (was C++). Simpler build, no C++ dependency.

### CI/CD

- cproc segfault retry for Windows MSYS2 stability.
- Replaced cproc|qbe pipe with temp file to catch cproc crashes.
- GitHub Pages deployment workflow for Doxygen docs.

### Documentation

- Restructured documentation with learning path and navigation hub.
- Removed Co-Authored-By requirement from commit messages.

### API

- **Removed 4 deprecated functions**: `padUpdate`, `dmaCopyToVRAM`, `dmaCopyToCGRAM`,
  `dmaCopyToOAM`. Use `padHeld`/`padPressed` and `dmaCopyVram`/`dmaCopyCGram`/`dmaCopyOam`.

## [0.3.0] - 2026-03-05

### Examples

- **Transparency rewrite**: Replaced placeholder color math demo with proper PVSnesLib-style
  transparency example — landscape (BG1, 4bpp) blended with scrolling clouds (BG3, 2bpp) via
  color addition, using assembly DMA loader for correct bank bytes.
- **HDMA gradient fix**: Improved step formula for smoother fade and deferred HDMA activation
  to button press.
- **Asset pipeline modernization**: All 36 examples now use `res/` subdirectories with gfx4snes
  Makefile rules for reproducible asset conversion from source PNGs. Removed tracked generated
  files (`.inc`, `_data.as`, `.pic`, `.pal`, `.map`) — these are now built at compile time.
- Converted all remaining BMP source assets to PNG.
- Added missing `.inc`/`_data.as`/`_meta.inc` entries to clean rules.

### Documentation

- Comprehensive README rewrite for all 36 examples with consistent formatting, hardware
  explanations, and build instructions.
- Added Mesen2 screenshots for key examples (breakout, likemario, collision_demo, etc.).

### Devtools

- Reorganized `devtools/` into one directory per tool, each with its own README.

### Housekeeping

- Removed unused project templates.
- Removed tracked gfx4snes-generated binary files from the repository.
- Fixed sprite32.pal size and removed unused assets.

## [0.2.0] - 2026-03-03

### Compiler

- **Fixed variable shift codegen** — `(1 << variable)` generated broken code because WLA-DX
  defaulted to 8-bit index registers per object file. Fixed by emitting `.ACCU 16` / `.INDEX 16`
  before each function.
- **Fixed variable shift stack offset** — `emitload_adj` now correctly adjusts +2 after `pha`
  in the shift loop.
- **-22% runtime cycles** vs PVSnesLib + 816-opt on benchmark suite (improved from -31% in v0.1.0)
- Added composite constant multiply (*20, *24, *40, *48, *96)
- Added inline multiply for *11 through *15
- Dead store elimination for inline multiply A-cache compatibility

### Library

- New modules: `map`, `object`, `debug`, `lzss`, `video`
- Mouse and Super Scope input drivers
- Assembly `oamSet()` — C version had framesize=158, causing visible slowdown with >2 sprites
- WAI-based `WaitForVBlank()` — saves CPU power
- Conditional BG scroll writes via `bg_scroll_dirty` bitmask in NMI handler
- NMI callback skip optimization (~260 cycles saved when using DefaultNmiCallback)
- `oamSetFast` macro for zero-overhead OAM writes
- Fixed `mode7Rotate` degree overflow
- Fixed SNESMOD FIFO clear
- Fixed `colorMathSetSource` CGWSEL bit 1 polarity
- Fixed write-only register reads in library code

### Examples

- 36 working examples (up from 25), 11 new:
  - **Games**: mapandobjects (object engine platformer), slopemario (slope collision)
  - **Maps**: dynamic_map (tilemap sprite engine)
  - **Graphics**: metasprite, object_size, mixed_scroll, mode1_bg3_priority, mode1_lz77, hdma_gradient
  - **Input**: mouse, superscope
  - **Basics**: collision_demo (AABB + tile collision)
- Removed variable-shift workarounds (collision_demo, background.c)
- Added GFX conversion rules to all example Makefiles (CI clean-build safe)

### Build System

- Multi-file C compilation support (`CSRC` variable)
- Automatic module dependency resolution in `common.mk`
- Bank $00 free space threshold warning in `symmap.py`

### Testing

- 57 compiler regression tests (up from 54)
- 25 unit test modules
- 36 example validations
- POSIX-compatible test scripts (macOS `grep -oE` instead of `grep -oP`)

## [0.1.0] - 2026-02-16

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

- 25 working examples covering all subsystems:
  - **Games**: Breakout, LikeMario (platformer with scrolling)
  - **Graphics**: sprites (simple, dynamic, animated), backgrounds (Mode 1, Mode 7,
    Mode 7 perspective, continuous scroll), effects (fading, HDMA wave, gradient colors,
    transparency, window, mosaic)
  - **Audio**: SNESMOD music, sound effects
  - **Input**: two-player joypad
  - **Memory**: SRAM save/load, HiROM demo
  - **Text**: hello world, text formatting

### Build System

- Unified `make/common.mk` for all examples
- HiROM support (`USE_HIROM=1`)
- SRAM support (`USE_SRAM=1`)
- Library module selection (`LIB_MODULES=console sprite dma`)
- CI pipeline on Linux, macOS, and Windows (MSYS2)
- `make release` for SDK packaging

### Testing

- 54 compiler regression tests
- Example validation with memory overlap checking
- Multi-platform CI (Linux, macOS, Windows)

### Documentation

- Doxygen-documented public API
- Example READMEs with hardware explanations
- Progressive learning path from hello world to full games
