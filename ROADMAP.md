# OpenSNES Roadmap

Current state of the project and planned work.

---

## Current Status: v0.11.0

A modern, well-tested SNES SDK ready for serious hobby development, game jams, and educational use,
building toward commercial-grade maturity. The compiler produces code 30% faster than
PVSnesLib+816-opt on our benchmark suite. 52 working examples cover all major subsystems,
with cross-platform CI on Linux, macOS, and Windows. Pre-built SDK binaries are available
for all three platforms.

---

## Completed

### Toolchain
- [x] **cc65816** — C11 compiler (cproc + QBE w65816 backend)
- [x] **wla-65816 / wlalink** — Assembler and linker (WLA-DX)
- [x] **wla-spc700** — SPC700 audio assembler
- [x] **gfx4snes** — PNG/BMP to SNES tiles, palettes, tilemaps
- [x] **font2snes** — Font converter
- [x] **smconv** — Impulse Tracker (.it) to SNESMOD soundbank (rewritten in pure C)

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
| `sprite` | OAM management, dynamic sprites, metasprites, LUT-based |
| `background` | BG layers, tilemaps, scrolling with dirty bitmask |
| `dma` | DMA transfers (VRAM, CGRAM, OAM) |
| `hdma` | HDMA effects (gradients, wave, ripple, iris, parallax) |
| `input` | Joypad, mouse, Super Scope, MultiPlayer5 |
| `text` | Text rendering (2bpp and 4bpp) |
| `snesmod` | Tracker music and SFX (IT format, multi-bank support) |
| `mode7` | Mode 7 rotation/scaling |
| `window` | Window masking |
| `colormath` | Transparency, color blending |
| `collision` | Bounding box collision |
| `animation` | Sprite animation system |
| `entity` | Game entity management |
| `math` | Fixed-point math, lookup tables, hardware multiplier |
| `sram` | Save RAM (battery-backed persistence) |
| `mosaic` | Mosaic pixelation effect |
| `interrupt` | NMI/IRQ handlers |
| `lzss` | LZ77 decompression to VRAM |
| `map` | Tile-based map engine with streaming |
| `object` | Object engine with physics and collision |
| `scoring` | Score tracking |
| `debug` | Nocash messages, Mesen breakpoints, assertions |
| `video` | Video mode and display control |
| `sa1` | SA-1 enhancement chip (10.74 MHz coprocessor) |

### Examples (52)
- **Text**: hello_world, text_test
- **Sprites**: simple_sprite, animated_sprite, dynamic_sprite, dynamic_metasprite, metasprite, object_size
- **Backgrounds**: mode0, mode1, mode1_bg3_priority, mode1_lz77, mode3, mode5, mode7, mode7_perspective, continuous_scroll, mixed_scroll
- **Effects**: fading, hdma_wave, hdma_gradient, hdma_helpers, gradient_colors, parallax_scrolling, mosaic, transparency, window, transparent_window
- **Input**: two_players, mouse, superscope
- **Audio**: snesmod_music, snesmod_sfx, snesmod_music_large, snesmod_music_hirom
- **Memory**: save_game, hirom_demo, sa1_hello, sa1_speed, sa1_starfield
- **Maps**: dynamic_map, mapscroll, slopemario, tiled
- **Basics**: collision_demo, random, timer
- **Games**: breakout, likemario, mapandobjects, tetris

### Build System
- [x] Cross-platform (Linux, macOS, Windows/MSYS2)
- [x] LoROM (default), HiROM, and SA-1 support
- [x] SRAM support
- [x] Library module selection (`LIB_MODULES=...`)
- [x] Automatic module dependency resolution
- [x] Multi-file C compilation (`CSRC` variable)
- [x] CI/CD pipeline (GitHub Actions) — build, test, docs, release
- [x] SDK release packaging (`make release`) with pre-built binaries
- [x] Unified memmap files (single source of truth)
- [x] VS Code project configuration

### Testing & Quality
- [x] opensnes-emu debug emulator (snes9x WASM) — single source of truth
- [x] 212 automated checks across 7 phases
- [x] 60 compiler regression tests (C → ASM pattern checks)
- [x] Visual regression (pixel-exact screenshot baselines)
- [x] Lag frame detection
- [x] Multi-platform CI (Linux, macOS, Windows)
- [x] Zero compiler warnings (72 Clang warnings fixed)

### Documentation
- [x] Doxygen API reference with doxygen-awesome-css theme
- [x] Example READMEs with hardware explanations (52/52)
- [x] Progressive learning path (GETTING_STARTED → LEARNING_PATH → tutorials)
- [x] Hardware reference docs (MEMORY_MAP, OAM, REGISTERS)
- [x] 9 tutorials (graphics, sprites, animation, scrolling, input, collision, audio, game states, SA-1)
- [x] Developer guides (CODE_STYLE, TROUBLESHOOTING, SNES_GRAPHICS_GUIDE, SNES_SOUND_GUIDE)
- [x] Published benchmark: 30% faster than PVSnesLib+816-opt
- [x] CHANGELOG, CONTRIBUTING, GitHub templates (issues + PR)

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
| Migration guide PVSnesLib → OpenSNES | Not started | Smoothest adoption path for existing PVSnesLib users |
| FAQ | Not started | Reduces support load, answers common questions upfront |

### Nice-to-Have

| Item | Status | Why it matters |
|------|--------|----------------|
| Pixel mode (Mode 3 direct drawing) | Not started | Feature parity with PVSnesLib (niche) |
| Tiled map editor integration | Not started | Workflow convenience for level designers |
| ~~VSCode project configuration~~ | Done (v0.7.1) | Developer experience improvement |
| Video tutorials | Not started | Wider audience reach |
| Project scaffolding (`opensnes init`) | Not started | Reduce friction for new users creating their own game |

### Next Steps

- [ ] More compiler peephole optimizations
- [ ] Performance profiling tools (VBlank usage, cycle counter)
- [ ] Mode 7 game example (racing or flying)
- [ ] Streaming audio support
- [ ] Hardware verification documentation
- [ ] At least 1 complete original game shipped
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

*Last updated: 2026-03-22*
