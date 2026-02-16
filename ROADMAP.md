# OpenSNES Roadmap

Current state of the project and planned work.

---

## Current Status: v0.1.0 (Alpha)

The SDK is functional for SNES game development. The compiler produces code
31% faster than PVSnesLib+816-opt on our benchmark suite. 23 working examples
cover all major subsystems.

---

## Completed

### Toolchain
- [x] **cc65816** — C11 compiler (cproc + QBE w65816 backend)
- [x] **wla-65816 / wlalink** — Assembler and linker (WLA-DX)
- [x] **wla-spc700** — SPC700 audio assembler
- [x] **gfx4snes** — PNG/BMP to SNES tiles, palettes, tilemaps
- [x] **font2snes** — Font converter
- [x] **smconv** — Impulse Tracker (.it) to SNESMOD soundbank

### Compiler Optimizations (13 phases)
- [x] Dead jump elimination + A-register cache
- [x] 8/16-bit mode tracking + byte immediates
- [x] Leaf and non-leaf function optimization (param alias, frame elimination)
- [x] Comparison+branch fusion
- [x] `pea.w` for constant args, `.l` to `.w` address shortening
- [x] `stz` for zero stores, redundant `cmp #0` elimination
- [x] Dead store elimination + aggressive frame elimination
- [x] Tail call optimization
- [x] Inline multiply (*3, *5, *6, *7, *9, *10)

### Library Modules (18)
| Module | Description |
|--------|-------------|
| `console` | Init, screen control, VBlank |
| `sprite` | OAM management, dynamic sprites |
| `background` | BG layers, tilemaps, scrolling |
| `dma` | DMA transfers (VRAM, CGRAM, OAM) |
| `hdma` | HDMA effects (gradients, wave, perspective) |
| `input` | Joypad reading |
| `text` | Text rendering on BG layers |
| `snesmod` | Tracker music and SFX (IT format) |
| `mode7` | Mode 7 rotation/scaling |
| `window` | Window masking |
| `colormath` | Transparency, color blending |
| `collision` | Bounding box collision |
| `animation` | Sprite animation system |
| `entity` | Game entity management |
| `math` | Fixed-point math, lookup tables |
| `sram` | Save RAM (battery-backed persistence) |
| `mosaic` | Mosaic pixelation effect |
| `interrupt` | NMI/IRQ handlers |

### Examples (23)
- **Text**: hello_world, text_test
- **Sprites**: simple_sprite, animated_sprite, dynamic_sprite
- **Backgrounds**: mode1, mode7, mode7_perspective, continuous_scroll
- **Effects**: fading, hdma_wave, gradient_colors, mosaic, transparency, window
- **Input**: two_players
- **Audio**: snesmod_music, snesmod_sfx
- **Memory**: save_game, hirom_demo
- **Basics**: collision_demo
- **Games**: breakout, likemario

### Build System
- [x] Cross-platform (Linux, macOS, Windows/MSYS2)
- [x] LoROM (default) and HiROM support
- [x] SRAM support
- [x] Library module selection (`LIB_MODULES=...`)
- [x] CI/CD pipeline (GitHub Actions)
- [x] SDK release packaging (`make release`)

### Testing
- [x] 50+ compiler regression tests
- [x] Example validation with memory overlap checking
- [x] Integration smoke tests (minimal, hello_world)
- [x] Multi-platform CI (Linux, macOS, Windows)

### Documentation
- [x] Doxygen API reference with doxygen-awesome-css theme
- [x] Example READMEs with hardware explanations
- [x] Progressive learning path (5 levels, 22 examples)
- [x] CHANGELOG, CONTRIBUTING, GitHub templates

---

## Planned

### Short Term
- [ ] SNESMOD HiROM support (soundbank addressing)
- [ ] More compiler peephole optimizations
- [ ] Additional examples: Mode 3 (256-color), parallax scrolling

### Medium Term
- [ ] Game templates (platformer, shmup, RPG starter)
- [ ] Sprite scaling via HDMA
- [ ] Streaming audio support
- [ ] Performance profiling tools

### Long Term (v1.0)
- [ ] Comprehensive test suite with hardware verification
- [ ] Memory usage analyzer
- [ ] Complete API documentation coverage
- [ ] Tutorial series (video or written)

---

## Known Limitations

1. **No floating-point** — use fixed-point math (`math.h`)
2. **32-bit operations are slow** — prefer `u16`/`s16` when possible
3. **~4 KB VBlank DMA budget** — don't exceed per-frame transfer limits
4. **Single C source file per project** — use `#include` or assembly for multi-file

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines and [README.md](README.md) for build instructions.

*Last updated: 2026-02-16*
