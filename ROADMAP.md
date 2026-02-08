# OpenSNES Roadmap

This document tracks the current state of the project and planned features.

---

## Current Status: v0.1.0 (Alpha)

The SDK is functional for basic SNES game development.

---

## Completed

### Toolchain
- [x] **cc65816** - C compiler for WDC 65816 (cproc + QBE backend)
- [x] **wla-65816** - Assembler
- [x] **wlalink** - Linker
- [x] **wla-spc700** - SPC700 audio assembler
- [x] **gfx4snes** - Graphics converter (PNG to SNES tiles/palettes)
- [x] **font2snes** - Font converter
- [x] **smconv** - SNESMOD audio converter

### Library Modules
| Module | Description | Status |
|--------|-------------|--------|
| `console` | Console init, screen control, VBlank | Done |
| `sprite` | OAM management, sprite display | Done |
| `background` | BG layers, tilemaps, scrolling | Done |
| `dma` | DMA transfers (VRAM, CGRAM, OAM) | Done |
| `hdma` | HDMA effects (gradients, parallax) | Done |
| `input` | Joypad reading with validation | Done |
| `text` | Text rendering on BG layers | Done |
| `audio` | Basic audio playback | Done |
| `snesmod` | Tracker music (IT format) | Done |
| `mode7` | Mode 7 rotation/scaling | Done |
| `window` | Window masking effects | Done |
| `colormath` | Transparency, fades, shadows | Done |
| `collision` | Bounding box collision | Done |
| `animation` | Sprite animation system | Done |
| `entity` | Game entity management | Done |
| `math` | Fixed-point math, lookup tables | Done |
| `sram` | Save RAM support | Done |
| `interrupt` | NMI/IRQ handlers | Done |

### Examples (31 total)
- **Text**: Hello World, Custom Font
- **Graphics**: Tiles, Animation, Sprites, Mode 1, Fading, Mode 0, Parallax, Mode 7, HDMA Gradient
- **Input**: Joypad, Two Players
- **Audio**: Sound Effects, SNESMOD Music
- **Basics**: Collision Demo, HiROM Demo
- **Game**: Breakout, Pong

### Build System
- [x] Cross-platform support (Linux, Windows, macOS)
- [x] Clang compiler for all platforms
- [x] Parallel builds
- [x] CI/CD pipeline (GitHub Actions)
- [x] SDK release packaging

### ROM Modes
- [x] **LoROM** - Standard 32KB bank mode (default)
- [x] **HiROM Basic** - 64KB bank mode without library (February 2026)

### Documentation
- [x] Doxygen API documentation
- [x] Example READMEs with educational content
- [x] CLAUDE.md project guide
- [x] Hardware reference documentation

---

## In Progress

### HiROM Phase 2 - Library Support
- [x] Library builds for HiROM mode (dual builds: `lib/build/lorom/` and `lib/build/hirom/`)
- [x] HiROM demo using library (console, DMA, VBlank work correctly)
- [x] Documentation updates (KNOWLEDGE.md, CLAUDE.md)
- [x] C library functions with return values work correctly in HiROM mode
  - Was the same function epilogue bug (`tsa` overwriting return value in A) that affected all modes
  - Fixed by the `tax`/`txa` pair to preserve A across stack adjustment in emit.c
- [ ] SNESMOD support for HiROM (soundbank addressing)

### Game Examples (paused)
- [ ] More game examples (platformer, RPG, shmup)
- [ ] Template projects with full game structure

---

## Planned

### v0.2.0 - Enhanced Graphics
- [ ] Mosaic effects module
- [ ] More Mode 7 examples (racing, flying)
- [ ] HDMA wavy/distortion effects
- [ ] Sprite scaling via HDMA

### v0.3.0 - Audio Enhancements
- [ ] Sound effect library (common game sounds)
- [ ] Streaming audio support
- [ ] Better SNESMOD integration

### v0.4.0 - Game Templates
- [ ] Platformer template (physics, camera, tiles)
- [ ] RPG template (menus, dialogue, maps)
- [ ] Shmup template (bullets, enemies, patterns)

### v1.0.0 - Production Ready
- [ ] Comprehensive test suite
- [ ] Performance profiling tools
- [ ] Memory usage analyzer
- [ ] Complete API documentation
- [ ] Tutorial series

---

## Known Limitations

1. **No floating-point** - Use fixed-point math (`math.h`)
2. **32-bit operations slow** - Prefer `u16`/`s16` when possible
3. **~4KB VBlank DMA budget** - Don't exceed per-frame transfer limits
4. **Static initializers in ROM** - Use `static u8 x;` not `static u8 x = 0;`
5. ~~**HiROM function returns**~~ - Resolved. Was the same function epilogue bug fixed in emit.c

---

## Contributing

See [README.md](README.md) for build instructions and contribution guidelines.

---

*Last updated: 2026-02-08*
