# OpenSNES Roadmap

Current state of the project and planned work. The single source of truth for
known traps and trade-offs is [`KNOWN_LIMITATIONS.md`](KNOWN_LIMITATIONS.md);
the design principles every roadmap item must respect live in
[`PHILOSOPHY.md`](PHILOSOPHY.md). This file tracks **what is shipped**
and **what is next**.

---

## Current Status: post-v0.21.0 (developing toward v0.22.0)

A modern, well-tested SNES SDK ready for serious hobby development, game jams,
and educational use, building toward commercial-grade maturity. The compiler
produces code 30 % faster than PVSnesLib + 816-opt on the benchmark suite. 54
working examples cover every major subsystem, with cross-platform CI on Linux,
macOS, and Windows enforcing not just "it compiles" but the full functional
test suite (luna, cycle-accurate native — corpus liveness + visual
regression + functional probes; SA-1/Super FX/DSP-1 run natively). Run
`make tests`, or `python3 tools/luna-test/luna_runner.py --list` for the corpus —
the suite grows
with new chantiers and a single hard-coded number rots fast.

### Audience

OpenSNES targets developers with prior 65816 / SNES / PVSnesLib experience.
The hardware traps (VRAM only during VBlank, 4 KB DMA budget per frame, bank
$00 ROM overflow, push-order inversion vs PVSnesLib, etc.) are documented
honestly in [`KNOWN_LIMITATIONS.md`](KNOWN_LIMITATIONS.md) — read it before
starting a project. Newcomers to SNES development should ramp up on the
hardware via the tutorials before adopting the SDK in earnest.

---

## Recently shipped (post-v0.13.0 maintenance work)

This stretch focused on closing process gaps surfaced by an internal audit
(P0/P1/P2/P3/P4 remediation plan). Highlights:

### Build & process safety
- **Bank $00 ROM overflow check** runs on every link (`make/common.mk`).
  String-literal spills now fail the build with a fix-it message instead of
  producing garbage at runtime.
- **`make verify-toolchain`** compares `compiler/{cproc,qbe,wla-dx}` HEADs
  against [`compiler/PINS.md`](compiler/PINS.md). CI runs the same check
  before every build — `git submodule update --remote` can no longer drift
  the toolchain past tested commits.
- **clang `-Wall -Wextra -Werror` syntax check** runs in parallel with
  `cc65816` for every C source. The SDK is currently warning-clean and stays
  that way (caught two real bugs in `lib/source/hdma.c` and tetris).
- **Tag-on-main guard** (`.github/workflows/release.yml`): a tag whose commit
  isn't on `main` fails fast instead of producing a release ZIP.
- **Commit-message lint** (`.github/workflows/lint.yml`): rejects PRs whose
  commits violate Conventional Commits or sneak in a `Co-Authored-By:`
  trailer.

### Architecture & docs
- `lib/source/object.asm` (3 124 LOC game-engine) relocated to `lib/contrib/`
  — `lib/source/` now only contains hardware-wrapping modules.
- `compiler/ABI.md`: empirically verified calling-convention reference
  (push order, frame layout, return values, type sizes, calling C from ASM
  and vice versa).
- `KNOWN_LIMITATIONS.md`: 14-entry severity-tagged catalog of silent
  failures inherited from the 65816 / SNES architecture and the toolchain.
- `tools/luna-test/`: luna-driven test harness — visual baselines keyed on
  luna's cross-arch-stable `--print-fbhash`, regenerated with
  `luna_runner.py --update`.

### Test suite
- **luna** is the sole automated test backend (cycle-accurate native emulator,
  pinned binary; runs SA-1 / Super FX / DSP-1 natively).
- **CI runs the full luna suite** on Linux (coverage + visual regression +
  functional probes) plus the compiler C→ASM checks and the cycle benchmark.
- **2 new compiler regression guards** (`test_arg_push_order`,
  `test_section_directives`) cover the ABI invariants P2.2 identified.
- Visual baselines now carry provenance metadata (rom hash, snes9x commit,
  capture timestamp). The runner emits `[WARN]` on drift instead of
  silently miscomparing.

---

## Completed

### Toolchain
- [x] **cc65816** — C11 compiler (cproc + QBE w65816 backend)
- [x] **wla-65816 / wlalink** — Assembler and linker (WLA-DX)
- [x] **wla-spc700** — SPC700 audio assembler
- [x] **gfx4snes** — PNG/BMP to SNES tiles, palettes, tilemaps
- [x] **font2snes** — Font converter
- [x] **smconv** — Impulse Tracker (.it) to SNESMOD soundbank (rewritten in pure C)

### Compiler optimisations
- [x] Dead jump elimination + A-register cache
- [x] 8/16-bit mode tracking + byte immediates
- [x] Leaf and non-leaf function optimisation (param alias, frame elimination)
- [x] Comparison + branch fusion
- [x] `pea.w` for constant args, `.l` → `.w` address shortening
- [x] `stz` for zero stores, redundant `cmp #0` elimination
- [x] Dead store elimination + aggressive frame elimination
- [x] Inline multiply (×3, ×5, ×6, ×7, ×9, ×10, ×11–×15)
- [x] Composite constant multiply (×20, ×24, ×40, ×48, ×96)
- [x] `.ACCU 16` / `.INDEX 16` for WLA-DX register-size tracking
- [x] Variable shift codegen fix (`(1 << variable)`)
- [x] Signed division and modulo (`__sdiv16` / `__smod16`)
- [x] **Tail call optimisation** — shipped (chantiers C.1 / C.2.1 / C.2.2;
      QBE patches `ed840fb` and `bab0164` per `compiler/PINS.md`). The
      `tail_call`, `lazy_rep20`, `plx_cleanup` test cases that previously
      ran under `--allow-known-bugs` were investigated in chantier A3 and
      found to be testing optimisations that had already shipped; the
      markers were stale. CI no longer passes `--allow-known-bugs`.
- [x] **A-cache through `pha`** — shipped; chantier C.6's audit confirmed
      the optimisation already worked, the `acache_pha` test was stale.
      Hard-fails any regression as of chantier A3 (2026-05-09).

### Library modules
| Module | Description | Status |
|--------|-------------|--------|
| `console` | Init, screen control, VBlank | core |
| `sprite`, `sprite_dynamic`, `sprite_lut` | OAM management, dynamic sprites, metasprites | core |
| `background` | BG layers, tilemaps, scrolling | core |
| `dma`, `hdma` | DMA / HDMA transfers (gradients, wave, ripple, parallax) | core |
| `input` | Joypad, mouse, Super Scope, MultiPlayer5 | core |
| `text`, `text4bpp` | Text rendering (2bpp and 4bpp) | core |
| `snesmod` | Tracker music and SFX (.it format, multi-bank) | core |
| `mode7` | Mode 7 rotation/scaling | core |
| `window` | Window masking | core |
| `colormath` | Transparency, color blending | core |
| `collision` | Bounding-box collision | core |
| `math` | Fixed-point math, LUTs, hardware multiplier | core |
| `sram` | Save RAM (battery-backed persistence) | core |
| `mosaic` | Mosaic pixelation | core |
| `lzss` | LZ77 decompression to VRAM | core |
| `map` | Tile-based map engine with streaming | core |
| `debug` | Nocash messages, Mesen breakpoints | core |
| `video` | Video mode and display control | core |
| `sa1` | SA-1 enhancement-chip helpers | experimental |
| `superfx` | SuperFX (GSU) loader stubs (assembly only — no C compiler) | experimental |
| `object` | Object engine with physics and collision | **contrib** (`lib/contrib/`) |

### Examples (53)
- **Text**: hello_world, text_test
- **Sprites**: simple_sprite, animated_sprite, dynamic_sprite, dynamic_metasprite, metasprite, object_size
- **Backgrounds**: mode0, mode1, mode1_bg3_priority, mode1_lz77, mode3, mode5, mode7, mode7_perspective, continuous_scroll, mixed_scroll
- **Effects**: fading, hdma_wave, hdma_gradient, hdma_helpers, gradient_colors, parallax_scrolling, mosaic, transparency, window, transparent_window
- **Input**: two_players, mouse, superscope
- **Audio**: snesmod_music, snesmod_sfx, snesmod_music_large, snesmod_music_hirom
- **Memory**: save_game, hirom_demo, sa1_hello, sa1_speed, sa1_starfield, superfx_hello
- **Maps**: dynamic_map, mapscroll, slopemario, tiled
- **Basics**: collision_demo, random, timer
- **Games**: breakout, likemario, mapandobjects, tetris

### Build system
- [x] Cross-platform (Linux, macOS, Windows / MSYS2)
- [x] LoROM (default), HiROM, SA-1, and SuperFX modes
- [x] FastROM support (`USE_FASTROM := 1`)
- [x] SRAM support
- [x] `LIB_MODULES` selection with automatic dependency resolution
- [x] Multi-file C compilation (`CSRC` variable)
- [x] CI/CD pipeline (GitHub Actions) — build, test, docs, release
- [x] SDK release packaging (`make release`) with pre-built binaries per OS
- [x] Unified memmap files (single source of truth)
- [x] Memmap-template dependency tracking (no more `make clean` after edits)

### Testing & quality
- [x] **luna** test backend (cycle-accurate native; runs SA-1 / Super FX / DSP-1)
- [x] Automated luna suite: corpus liveness coverage, visual regression,
      functional probes (input→WRAM via `--assert`), audio + VRAM/ARAM +
      sprite-structure checks, and the `SNES_ASSERT`/WDM oracle
- [x] **Compiler regression tests** (C → ASM pattern checks, `make test-compiler`)
- [x] **Visual regression** keyed on luna's cross-arch-stable `--print-fbhash`
- [x] **Cycle-count benchmark** (`make bench`)
- [x] **Bank $00 ROM overflow** check on every link (build-time guard)
- [x] **Toolchain pin verification** (`make verify-toolchain`)
- [x] **clang `-Wall -Wextra -Werror`** syntax check on every C source
- [x] **Multi-platform CI** (Linux, macOS, Windows) with functional-tests
      gating PR merges
- [x] **Commit-message lint** (Conventional Commits + no `Co-Authored-By`)

### Documentation
- [x] Doxygen API reference (with doxygen-awesome-css theme)
- [x] [`KNOWN_LIMITATIONS.md`](KNOWN_LIMITATIONS.md) — severity-tagged catalog
      of silent failures
- [x] [`compiler/ABI.md`](compiler/ABI.md) — calling-convention reference
- [x] [`compiler/PINS.md`](compiler/PINS.md) — pinned submodule SHAs +
      local-patch lists
- [x] Example READMEs with hardware explanations (53 / 53)
- [x] Progressive learning path (GETTING_STARTED → LEARNING_PATH → tutorials)
- [x] Hardware reference docs (MEMORY_MAP, OAM, REGISTERS)
- [x] Tutorials (graphics, sprites, animation, scrolling, input, collision, audio, game states, SA-1)
- [x] Developer guides (CODE_STYLE, TROUBLESHOOTING, SNES_GRAPHICS_GUIDE, SNES_SOUND_GUIDE)
- [x] Published benchmark: 30 % faster than PVSnesLib + 816-opt
- [x] CHANGELOG, CONTRIBUTING (with branching policy), GitHub templates

### Developer tooling
- [x] **symmap.py** — Symbol-map analysis, memory-overlap detection, bank
      $00 overflow checking
- [x] **verify_toolchain.py** — Compiler-submodule pin enforcement
- [x] **lint_commits.py** — Conventional-Commits + no-`Co-Authored-By` lint
- [x] **cyclecount.py** — CPU cycle estimation
- [x] **check_mvn.py** — MVN/MVP operand linter
- [x] **snesdbg** — Mesen2 Lua debugging library (BDD-style tests)
- [x] **brr2it** — BRR → Impulse Tracker conversion
- [x] **benchmark** — Compiler-performance comparison tool
- [x] **gen_hud_bar** — HUD bar generator
- [x] **font2snes** — Font → SNES-tile converter

---

## Planned: v1.0

### Must-have

| Item | Status | Why it matters |
|------|--------|----------------|
| Pre-built binary releases | Done (`release.yml`) | Adoption blocker — users shouldn't need to compile the compiler |
| Hardware verification docs | Not started | Credibility — document testing on real SNES via FXPak Pro |
| Showcase game (not a port) | In progress (RPG project) | Proves the SDK can ship a complete game |
| Published performance benchmark | Done (`docs/BENCHMARK.md`) | Shows the 30 % improvement with data |
| Migration guide PVSnesLib → OpenSNES | Not started | Smoothest adoption path for existing PVSnesLib users |
| FAQ | Not started | Reduces support load |
| `examples/maps/dynamic_map` cleanup | Not started | Last example with non-trivial dead-code (`convertC64Sprite`) |

### Nice-to-have

| Item | Status | Why it matters |
|------|--------|----------------|
| Pixel mode (Mode 3 direct drawing) | Not started | Feature parity with PVSnesLib (niche) |
| Tiled map editor integration | Not started | Workflow convenience for level designers |
| Video tutorials | Not started | Wider audience reach |
| Project scaffolding (`opensnes init`) | Not started | Reduce friction for new users |

### Next steps (audit-driven, prioritised)

- [ ] **Sprite/text duplication audit** (P3.2) — `lib/source/` has parallel
      C and ASM implementations of `sprite` and `text`. Benchmark each,
      decide migration vs documentation, eliminate duplications. Largest
      remaining cleanup item (2–3 weeks, perf-critical so risk is real).
- [x] **SuperFX / SA-1 functional tests** (P3.4) — *(SUPERSEDED 2026-06-20: luna runs SA-1 / Super FX natively in the headless harness; the Mesen2 phase and the `opensnes-emu` submodule were removed. See `.claude/notes/chantiers/luna_migration.md`.)* Originally added a Mesen2-headless
      visual regression phase
      (`tools/opensnes-emu/test/phases/visual-mesen2.mjs`) running the
      vendored Mesen2 binary in `--testrunner` mode against the four
      chip-using examples. snes9x's libretro core does not detect the
      GSU chip in OpenSNES' SuperFX ROM headers, so the snes9x-based
      visual phase only validates "boots without crashing" for chip
      ROMs; Mesen2 actually executes the chip code and gives a
      meaningful baseline. The phase auto-skips when the binary is
      absent (`scripts/install-mesen2.sh` fetches it once per CI run,
      cached locally). 4 new checks, total 257 → 261. Done in
      ~1 day rather than the originally estimated 2 weeks.
- [x] **Retire residual TCO `[KNOWN_BUG]` test cases** — done in
      chantier A3 (2026-05-09). Investigation showed every optimisation
      the markers gated had already shipped; the test assertions had
      drifted away from current emit-pass behaviour rather than
      anticipating future work. Markers removed, assertions converted
      to hard-fail, `--allow-known-bugs` dropped from CI.
- [ ] **Mode 7 game example** (racing or flying)
- [ ] **Streaming audio support**
- [ ] **Hardware verification documentation**
- [ ] **Original-game release**
- [ ] **Video tutorial series**

---

## Known limitations

The full catalog with severity tags lives in
[`KNOWN_LIMITATIONS.md`](KNOWN_LIMITATIONS.md). Headlines:

- **No floating-point** — use fixed-point math (`<snes/math.h>`)
- **`int` is 2 bytes, `long` is 4 bytes** on this target (since chantier
  A1, 2026-05-08) — bare `int` is now correct, but `u16` / `s16` /
  `u32` from `<snes/types.h>` remain preferred for portability
- **~4 KB VBlank DMA budget** per frame
- **All C variables must be in bank $00, < $2000** — compiler emits
  `sta.l $0000,x`
- **Push order is LEFT-TO-RIGHT** (vs PVSnesLib's right-to-left) — see
  [`compiler/ABI.md`](compiler/ABI.md) before porting an ASM helper

---

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines, branch policy
(`main` = stable / `develop` = active), and PR rules. Build instructions
live in [`README.md`](README.md).

*Last updated: 2026-05-07. Anchored claims (version, examples count, framework
opt-in list) verified by `make lint-docs` — see `devtools/check_doc_drift.py`
and `.claude/rules/doc_consistency.md`.*
