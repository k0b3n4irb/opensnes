# OpenSNES vs PVSnesLib — Product Maturity Review

*March 2026 — OpenSNES v0.6.0 vs PVSnesLib v4.5.0*

This review evaluates OpenSNES as a product, comparing it to PVSnesLib (the established
reference SDK for SNES homebrew development). The lens is what a new user or team would
experience — not internal code quality, but practical readiness.

---

## 1. Maturity & Stability

### PVSnesLib
- **14+ years** of development (2012-2026), 15 tagged releases
- **Proven in production**: commercial games shipped (Yo-Yo Shuriken, Eyra, Sydney Hunter)
- **Stable API**: slow evolution, backward-compatible changes
- **Known quantities**: community has identified and documented most pitfalls
- **Zero automated tests**: stability comes from time-tested usage, not validation

### OpenSNES
- **3 months** of active development (Jan-Mar 2026), 6 releases (v0.1.0-v0.6.0)
- **No shipped games** yet (only demo examples)
- **API still evolving**: deprecated functions removed between versions
- **Strong regression protection**: 60 compiler tests + 25 unit modules + 37 ROM validations
- **3-platform CI**: Linux, macOS, Windows — every push validated

### Verdict

**PVSnesLib wins on track record.** 14 years and commercial games is hard to beat. But
PVSnesLib's stability relies on "it works because people use it" — there are zero tests
to catch regressions. OpenSNES compensates for its youth with automated testing that
PVSnesLib completely lacks. A silent regression in PVSnesLib could go unnoticed for months;
in OpenSNES it's caught in minutes.

**Risk profile**: PVSnesLib = low risk (known), high blind spots (untested).
OpenSNES = higher risk (young), lower blind spots (well-tested).

---

## 2. Ease of Getting Started

### PVSnesLib
- Set `PVSNESLIB_HOME`, include `snes_rules`, write C, `make`
- Wiki with installation instructions per platform
- Docker support for isolated builds
- **78 examples** to copy-paste from
- **BUT**: no getting started guide in the repo itself (wiki-only)
- **BUT**: tcc816 is C89-only — no `const`, no `_Static_assert`, no `//` comments
- **BUT**: error messages from tcc816 are often cryptic

### OpenSNES
- Set `OPENSNES`, include `common.mk`, write C, `make`
- `GETTING_STARTED.md` in repo with per-platform instructions
- `LEARNING_PATH.md` with progressive curriculum
- `EXAMPLE_WALKTHROUGHS.md` with detailed code explanations
- **37 examples**, each with its own README.md
- C11 support: `const`, `_Static_assert`, modern syntax
- cc65816 wrapper retries on crashes, cleaner error output
- **BUT**: requires building compiler from source (no pre-built binaries yet)

### Verdict

**OpenSNES wins on onboarding documentation.** Every example has a README. There's a
learning path. The getting started guide is in-repo, not on an external wiki. C11 is a
major quality-of-life upgrade over C89.

**PVSnesLib wins on pre-built binaries.** Download ZIP, set path, start building.
OpenSNES requires `make compiler` first.

**Net**: Tie with different strengths. PVSnesLib = faster first build.
OpenSNES = better learning path.

---

## 3. Ease of SDK Evolution

### PVSnesLib
- **Monolithic library**: all modules compiled into one .a, always linked
- **tcc816 compiler**: fork of an old TCC version, custom patches. Hard to modify —
  any change risks the fragile 816-opt peephole optimizer
- **816-opt post-processor**: 38 regex peephole rules applied to assembly output.
  Brittle — new patterns can break existing optimizations
- **No tests**: changing the library means manually testing affected examples
- **Assembly-heavy**: ~85% of library is hand-written 65816 ASM
- **Single maintainer** (alekmaul) — bus factor of 1

### OpenSNES
- **Modular library**: `LIB_MODULES=console sprite dma` — only link what you use
- **Modern compiler**: cproc (C11 frontend) + QBE (established IR compiler).
  QBE is a well-documented, academically-grounded project
- **13 optimization phases** in QBE backend, each independently testable
- **60 compiler tests**: any change to codegen is immediately validated
- **Mixed C/ASM library**: new modules can be written in C
- **Documented architecture**: CLAUDE.md, ABI conventions, memory layout,
  module dependencies
- **Contributor-friendly**: CONTRIBUTING.md, code style guide, PR templates

### Verdict

**OpenSNES wins decisively.** The modular library, modern compiler architecture,
comprehensive tests, and contributor documentation make the SDK far easier to evolve.

The compiler story is particularly stark: PVSnesLib's tcc816+816-opt is a fragile pipeline
where peephole patterns interact unpredictably. OpenSNES's QBE backend has structured
optimization phases with regression tests.

---

## 4. Documentation Quality

### PVSnesLib
- **API reference**: Doxygen-generated, published online
- **16 headers** with Doxygen comments
- **Wiki**: external (GitHub wiki), installation + some guides
- **Examples**: 78 but only 4 have README files
- **No CONTRIBUTING.md**, no code style guide, no architectural docs
- **No CHANGELOG** in repo

### OpenSNES
- **API reference**: Doxygen-generated, 28 headers with full markup
- **In-repo documentation**: README, CONTRIBUTING, CHANGELOG, ROADMAP
- **Developer guides**: GETTING_STARTED, LEARNING_PATH, EXAMPLE_WALKTHROUGHS,
  CODE_STYLE, TROUBLESHOOTING, SNES_GRAPHICS_GUIDE, SNES_SOUND_GUIDE
- **Examples**: 37, each with its own README.md (hardware explanations, code walkthroughs)
- **Test documentation**: tests/CLAUDE.md with module inventory, assertion counts
- **Hardware reference**: docs/hardware/ with MEMORY_MAP, OAM, REGISTERS

### Verdict

**OpenSNES wins clearly.** Documentation density and quality are in a different league.
PVSnesLib has good Doxygen API docs but almost nothing else.

The example README gap is especially significant: a PVSnesLib user looking at
`snes-examples/graphics/Effects/HDMAGradient/` finds only source code. An OpenSNES user
finds a README explaining the PPU concept, HDMA registers, code walkthrough, and exercises.

---

## 5. Features & API

### PVSnesLib
- **16 headers** covering: backgrounds, sprites, video, DMA/HDMA, console, input,
  audio, objects, maps, LZSS, pixel mode, scoring
- **Rich convenience API**: `bgInitTileSet()` does tile loading + palette + VRAM config
  in one call
- **SNESMOD audio**: full tracker-based music + SFX engine
- **Object engine**: sprite management with animation, collision, spawning
- **Map engine**: tile-based level streaming
- **All graphics modes**: 0, 1, 3, 5, 6, 7
- **All input devices**: joypad (2P), mouse, Super Scope
- **Pixel mode**: 256-color direct drawing (Mode 3)
- **Scoring system**: built-in score tracking

### OpenSNES
- **28 headers** covering: backgrounds, sprites, video, DMA, HDMA, console, input,
  audio, objects, maps, LZSS, entity, collision, animation, Mode 7, mosaic, window,
  colormath, math, text, debug, scoring, sram, system, types, registers
- **Modular API**: select only needed modules (`LIB_MODULES`)
- **SNESMOD audio**: same tracker engine (ported from PVSnesLib)
- **Object + Entity engines**: two levels of abstraction
- **HDMA helpers**: wave, ripple, color gradient, iris wipe, brightness
- **All input devices**: joypad (2P), mouse, Super Scope
- **Debug module**: Mesen2 breakpoints, nocash messages, assertions
- **Math library**: fixed-point, sin/cos lookup, random
- **Text engine**: 2bpp and 4bpp with DMA-accelerated rendering
- **Missing vs PVSnesLib**: pixel mode (Mode 3 direct drawing)

### Verdict

**Feature parity, with different strengths.**

PVSnesLib has more convenience functions and pixel mode. OpenSNES has more modules
(28 vs 16 headers), a debug module, higher-level HDMA helpers, and better math/text
libraries.

**API design philosophy**:
- PVSnesLib: "one function does everything" (`bgInitTileSet`, `oamSet` with 8 params)
- OpenSNES: "composable primitives" (`dmaCopyVram` + `bgSetMapPtr` + `bgSetGfxPtr`)

PVSnesLib's approach is faster for prototyping. OpenSNES's approach gives more control
and is easier to debug when something goes wrong.

---

## 6. Tooling & Developer Experience

### PVSnesLib
- **gfx4snes**: PNG/BMP to SNES tile conversion
- **smconv**: tracker to SNESMOD
- **snestools**: various small utilities (constify, bin2txt)
- **tmx2snes**: Tiled map editor integration
- **No debug library**, no memory analysis tools

### OpenSNES
- **gfx4snes**: same tool (forked, maintained)
- **smconv**: rewritten in pure C (was C++)
- **font2snes**: font to SNES tiles
- **symmap.py**: symbol map analysis (memory overlap detection, bank overflow checking)
- **cyclecount.py**: CPU cycle estimation
- **check_mvn.py**: MVN/MVP operand linter
- **snesdbg**: Mesen2 Lua debugging library (BDD-style tests, symbol-aware memory access)
- **brr2it**: BRR to Impulse Tracker conversion
- **benchmark**: compiler performance comparison tool
- **GitHub templates**: issue templates, PR template

### Verdict

**OpenSNES wins on developer tooling.** symmap.py for memory analysis, snesdbg for
Mesen2 debugging, and cycle counting are tools PVSnesLib simply doesn't have.

PVSnesLib's tmx2snes (Tiled integration) is useful and not replicated in OpenSNES.

---

## 7. Honest Assessment of Weaknesses

### Where PVSnesLib is Better
1. **Pre-built binaries**: download and start, no compile-from-source
2. **More examples**: 78 vs 37 (though many are trivial variations)
3. **Proven in production**: commercial games shipped
4. **Community**: established Discord, wiki, 14 years of Q&A
5. **Quick prototyping**: high-level convenience functions
6. **Pixel mode**: 256-color direct drawing not available in OpenSNES
7. **Tiled integration**: tmx2snes

### Where OpenSNES is Better
1. **Modern compiler**: C11, 13 optimization phases, -22% cycles vs PVSnesLib+816-opt
2. **Testing**: 60+25+37 automated tests vs zero
3. **Documentation**: multi-layered, in-repo, example READMEs
4. **Modularity**: link only what you need, clear dependency graph
5. **Evolvability**: contributor docs, CI, structured compiler backend
6. **Tooling**: memory analysis, debugging library, cycle counting
7. **HDMA helpers**: high-level wave/ripple/iris/gradient API
8. **Debug module**: Mesen2 breakpoints, nocash, assertions from C
9. **Type safety**: C11 `_Static_assert`, const correctness, volatile
10. **Cross-platform CI**: every push tested on 3 platforms

---

## 8. Roadmap to v1.0

OpenSNES's claim of "production quality" is **justified for the SDK itself** (build system,
tests, documentation, tooling) but **not yet for shipping games** (no production track
record, some missing features).

### Must-Have for v1.0

| Item | Status | Impact |
|------|--------|--------|
| Pre-built binary releases (download & use) | Missing | Blocks adoption |
| Hardware verification (real SNES testing) | Not documented | Credibility |
| At least 1 complete game demo (not port) | In progress (RPG) | Showcase |
| Performance benchmark published | Tool exists, not published | Marketing |

### Nice-to-Have

| Item | Status | Impact |
|------|--------|--------|
| Pixel mode (Mode 3 direct drawing) | Missing | Niche feature |
| Tiled map editor integration | Missing | Workflow convenience |
| VSCode extension / project template | Missing | DX improvement |
| Video tutorials | Missing | Wider audience |
| More ported examples | In progress | Breadth |

### Already Exceeds PVSnesLib

| Area | OpenSNES | PVSnesLib |
|------|----------|-----------|
| Automated testing | 60+25+37 | 0 |
| Documentation depth | 8 guides + 37 READMEs | Wiki + 4 READMEs |
| Compiler quality | C11, 13 opt phases | C89, 38 regex peepholes |
| Cross-platform CI | 3 platforms/push | Build-only, no validation |
| Developer tooling | 8 tools | 3 tools |
| Library modularity | Selective linking | Monolithic |

---

## Conclusion

OpenSNES v0.6.0 is a **technically superior SDK** to PVSnesLib in compiler quality,
testing, documentation, and tooling. It is **production-ready as an SDK** (the tools
work reliably across platforms). It is **not yet production-proven for shipping games**
(no commercial releases, short track record).

**Positioning**: A modern, well-tested SNES SDK ready for serious hobby development
and game jams, building toward commercial-grade maturity.

The path to v1.0 is clear and achievable: pre-built binaries, hardware verification
documentation, and a showcase game.
