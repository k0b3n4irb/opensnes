# OpenSNES Roadmap

Current state of the project and planned work.

---

## Current Status: v0.6.0

A modern, well-tested SNES SDK ready for serious hobby development and game jams,
building toward commercial-grade maturity. The compiler produces code 30% faster than
PVSnesLib+816-opt on our benchmark suite. 37 working examples cover all major subsystems,
with cross-platform CI on Linux, macOS, and Windows.

See [docs/MATURITY_REVIEW.md](docs/MATURITY_REVIEW.md) for a detailed comparison with PVSnesLib.

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
- [x] Signed division and modulo (`__sdiv16` / `__smod16`)

### Library Modules (28 headers)
| Module | Description |
|--------|-------------|
| `console` | Init, screen control, VBlank |
| `sprite` | OAM management, dynamic sprites, LUT-based |
| `background` | BG layers, tilemaps, scrolling with dirty bitmask |
| `dma` | DMA transfers (VRAM, CGRAM, OAM) |
| `hdma` | HDMA effects (gradients, wave, ripple, iris, parallax) |
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
| `scoring` | Score tracking |
| `debug` | Nocash messages, Mesen breakpoints, assertions |
| `video` | Video mode and display control |

### Examples (37)
- **Text**: hello_world, text_test
- **Sprites**: simple_sprite, animated_sprite, dynamic_sprite, metasprite, object_size
- **Backgrounds**: mode1, mode1_bg3_priority, mode1_lz77, mode7, mode7_perspective, continuous_scroll, mixed_scroll
- **Effects**: fading, hdma_wave, hdma_gradient, hdma_helpers, gradient_colors, parallax_scrolling, mosaic, transparency, window, transparent_window
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
- [x] 60 compiler regression tests
- [x] 25 unit test modules (~385 runtime + 119 compile-time assertions)
- [x] 37 example validations with memory overlap checking
- [x] Multi-platform CI (Linux, macOS, Windows)

### Documentation
- [x] Doxygen API reference with doxygen-awesome-css theme
- [x] Example READMEs with hardware explanations (37/37)
- [x] Progressive learning path (GETTING_STARTED, LEARNING_PATH, EXAMPLE_WALKTHROUGHS)
- [x] Hardware reference docs (MEMORY_MAP, OAM, REGISTERS)
- [x] Developer guides (CODE_STYLE, TROUBLESHOOTING, SNES_GRAPHICS_GUIDE, SNES_SOUND_GUIDE)
- [x] CHANGELOG, CONTRIBUTING, GitHub templates
- [x] Product maturity review (docs/MATURITY_REVIEW.md)

### Developer Tooling (8 tools)
- [x] **symmap.py** — Symbol map analysis, memory overlap detection, bank overflow checking
- [x] **cyclecount.py** — CPU cycle estimation
- [x] **check_mvn.py** — MVN/MVP operand linter
- [x] **snesdbg** — Mesen2 Lua debugging library (BDD-style tests)
- [x] **brr2it** — BRR to Impulse Tracker conversion
- [x] **benchmark** — Compiler performance comparison tool
- [x] **gen_hud_bar** — HUD bar generator
- [x] **font2snes** — Font to SNES tile converter

---

## Planned: v1.0

### Must-Have

| Item | Status | Why it matters |
|------|--------|----------------|
| Pre-built binary releases | Done (release.yml) | Blocks adoption — users shouldn't need to compile the compiler |
| Hardware verification docs | Not started | Credibility — document testing on real SNES via FXPak Pro |
| Showcase game (not a port) | In progress (RPG project) | Proves the SDK can ship a complete game |
| Published performance benchmark | Done (docs/BENCHMARK.md) | Marketing — show the 30% improvement with data |

### Nice-to-Have

| Item | Status | Why it matters |
|------|--------|----------------|
| Pixel mode (Mode 3 direct drawing) | Not started | Feature parity with PVSnesLib (niche) |
| Tiled map editor integration | Not started | Workflow convenience for level designers |
| VSCode extension / project template | Not started | Developer experience improvement |
| Video tutorials | Not started | Wider audience reach |
| More ported examples | In progress | Breadth of coverage |

### Short Term (v0.7.0)
- [x] Pre-built binary releases for Linux, macOS, Windows
- [x] Published benchmark results (docs/BENCHMARK.md — -30.3% vs PVSnesLib)
- [ ] More compiler peephole optimizations
- [ ] Performance profiling tools (VBlank usage, cycle counter)

### Medium Term (v0.8.0-v0.9.0)
- [ ] Tutorial series for beginners
- [ ] Mode 7 game example (racing or flying)
- [ ] Streaming audio support
- [ ] Hardware verification documentation

### Long Term (v1.0)
- [ ] At least 1 complete original game shipped
- [ ] Comprehensive test suite with hardware verification
- [ ] Complete API documentation coverage
- [ ] Video tutorial series

---

## Known Limitations

1. **No floating-point** — use fixed-point math (`math.h`)
2. **32-bit operations are slow** — prefer `u16`/`s16` when possible
3. **~4 KB VBlank DMA budget** — don't exceed per-frame transfer limits
4. **All C variables must be in bank $00, < $2000** — compiler generates `sta.l $0000,x`
5. **No floating-point emulation** — integer and fixed-point only

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines and [README.md](README.md) for build instructions.

*Last updated: 2026-03-09*
