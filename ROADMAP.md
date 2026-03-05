# OpenSNES Roadmap

Current state of the project and planned work.

---

## Current Status: v0.3.0

The SDK is functional for SNES game development. The compiler produces code
22% faster than PVSnesLib+816-opt on our benchmark suite. 36 working examples
cover all major subsystems, with cross-platform CI on Linux, macOS, and Windows.

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
- [x] Inline multiply (*3, *5, *6, *7, *9, *10, *11-*15)
- [x] Composite constant multiply (*20, *24, *40, *48, *96)
- [x] `.ACCU 16` / `.INDEX 16` for WLA-DX register size tracking
- [x] Variable shift codegen fix (`(1 << variable)`)

### Library Modules (23)
| Module | Description |
|--------|-------------|
| `console` | Init, screen control, VBlank |
| `sprite` | OAM management, dynamic sprites, LUT-based |
| `background` | BG layers, tilemaps, scrolling with dirty bitmask |
| `dma` | DMA transfers (VRAM, CGRAM, OAM) |
| `hdma` | HDMA effects (gradients, wave, parallax) |
| `input` | Joypad, mouse, Super Scope |
| `text` | Text rendering (2bpp and 4bpp) |
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
| `lzss` | LZ77 decompression to VRAM |
| `map` | Tile-based map engine with streaming |
| `object` | Object engine with physics and collision |
| `debug` | Nocash messages, Mesen breakpoints, assertions |
| `video` | Video mode and display control |

### Examples (36)
- **Text**: hello_world, text_test
- **Sprites**: simple_sprite, animated_sprite, dynamic_sprite, metasprite, object_size
- **Backgrounds**: mode1, mode1_bg3_priority, mode1_lz77, mode7, mode7_perspective, continuous_scroll, mixed_scroll
- **Effects**: fading, hdma_wave, hdma_gradient, gradient_colors, parallax_scrolling, mosaic, transparency, window, transparent_window
- **Input**: two_players, mouse, superscope
- **Audio**: snesmod_music, snesmod_sfx
- **Memory**: save_game, hirom_demo
- **Maps**: dynamic_map, slopemario
- **Basics**: collision_demo
- **Games**: breakout, likemario, mapandobjects

### Build System
- [x] Cross-platform (Linux, macOS, Windows/MSYS2)
- [x] LoROM (default) and HiROM support
- [x] SRAM support
- [x] Library module selection (`LIB_MODULES=...`)
- [x] Automatic module dependency resolution
- [x] Multi-file C compilation (`CSRC` variable)
- [x] CI/CD pipeline (GitHub Actions)
- [x] SDK release packaging (`make release`)

### Testing
- [x] 57 compiler regression tests
- [x] 25 unit test modules
- [x] 36 example validations with memory overlap checking
- [x] Multi-platform CI (Linux, macOS, Windows)

### Documentation
- [x] Doxygen API reference with doxygen-awesome-css theme
- [x] Example READMEs with hardware explanations
- [x] Progressive learning path
- [x] CHANGELOG, CONTRIBUTING, GitHub templates

---

## Planned

### Short Term
- [ ] More compiler peephole optimizations
- [ ] Performance profiling tools (VBlank usage, cycle counter)

### Medium Term
- [ ] Tutorial series for beginners
- [ ] Mode 7 game examples (racing, flying)
- [ ] RPG game template
- [ ] Streaming audio support

### Long Term (v1.0)
- [ ] Comprehensive test suite with hardware verification
- [ ] Complete API documentation coverage
- [ ] Video tutorial series

---

## Known Limitations

1. **No floating-point** — use fixed-point math (`math.h`)
2. **32-bit operations are slow** — prefer `u16`/`s16` when possible
3. **~4 KB VBlank DMA budget** — don't exceed per-frame transfer limits
4. **All C variables must be in bank $00, < $2000** — compiler generates `sta.l $0000,x`

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines and [README.md](README.md) for build instructions.

*Last updated: 2026-03-03*
