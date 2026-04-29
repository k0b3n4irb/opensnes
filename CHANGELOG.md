# Changelog

All notable changes to OpenSNES are documented in this file.

OpenSNES is forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib). This changelog
covers changes made since the fork.

## [Unreleased]

Audit-driven maintenance cycle (see `~/opensnes_audit_2026-04-26.md` and
`~/opensnes_remediation_plan_2026-04-26.md`). 19 of 23 plan items completed
across P0-P4. The SDK is now end-to-end CI-enforced: build green no longer
just means "it compiles" — it means visual regression, lag detection, runtime,
compiler patterns, bank $00 layout and submodule pins all hold.

### Added

- **`KNOWN_LIMITATIONS.md`** — public catalog of silent failures with
  severity tags (🔴 silent corruption / 🟠 silent build / 🟡 toolchain quirk /
  🟢 mitigated). 14 entries covering VBlank, DMA budget, WRAM port, bank $00,
  push order, `volatile` + QBE, `.ACCU` tracking, SA-1 SIWP, SuperFX/Mesen2,
  compiler optimisation gaps, type sizes, sprite-palette CGRAM offset.
- **`compiler/ABI.md`** — empirically verified calling-convention reference:
  LEFT-TO-RIGHT push, frame layout, return values, direct-page layout
  (`tcc__r*`), type sizes, calling C↔ASM templates, port-from-PVSnesLib
  checklist.
- **`compiler/PINS.md`** — pinned SHAs for `compiler/{cproc,qbe,wla-dx}` with
  a per-submodule list of carried-forward local patches.
- **`tools/opensnes-emu/test/BASELINES.md`** — visual-regression baseline
  schema and regen protocol.
- **`lib/contrib/` directory** — non-core engine modules. Currently houses
  `object.asm` (3 124 LOC game-entity engine) relocated from `lib/source/`
  so `lib/source/` only contains hardware-wrapping code.
- **Section "Who is OpenSNES for?"** in README — explicit prerequisites
  (65816 ASM, NMI/VBlank model, hex addresses) and non-targets, plus an
  enhancement-chip maturity table.
- **Branching policy** in `CONTRIBUTING.md` — invariants for `main` vs
  `develop`.
- **2 compiler regression guards** — `test_arg_push_order` (LEFT-TO-RIGHT
  ABI) and `test_section_directives` (`.ACCU 16` / `.INDEX 16` markers).
  Compiler test suite: 60 → 62.

### Changed

- **`tools/opensnes-emu/` is now a public submodule** at
  `github.com/k0b3n4irb/opensnes-emu` (was a local-only directory in the
  user's tree). History was rewritten with `git filter-repo` to strip 275
  generated build artifacts (.sfc, .sym, .o, .obj, etc.) — repo size went
  from 218 MB to 11 MB tracked source.
- **CI runs the full ~390-check test suite** (was build-only). Drop of
  `--quick` adds the 60-test compiler-pattern phase. Added
  `--allow-known-bugs` so the 5 documented compiler-optimisation gaps
  (TCO + A-cache-through-pha + stale `leaf_opt` marker) report as
  `[KNOWN_BUG]` rather than failing the build.
- **`ROADMAP.md` resync** — corrects 52 → 53 examples, 212 → ~390 checks,
  60 → 62 compiler tests, 7 → 8 phases. Drops removed modules
  (animation, entity). Adds the contrib status for `object`. Pivots to
  the "post-v0.13.0 toward v0.14.0" snapshot.
- **`.claude/` is now tracked** (was entirely gitignored except one file).
  Hooks, rules, skills and shared settings ship with the repo so any
  Claude Code user gets the same gates. Personal `settings.local.json`
  remains gitignored.
- **`setMainScreen()` instead of `REG_TM = ...`** in 22 examples (style
  consistency; identical bytes generated). `short` → `s16` in three more.
- **Sprite API simplification (P3.2 chantier B)** — public sprite surface
  collapses from 28 to 18 functions:
  - `oamSet`/`oamSetEx`/`oamInit*` consolidations (B.A);
  - `oamDynamicDraw(id)` size-aware dispatcher replaces
    `oamDynamic{8,16,32}Draw` (B.1+B.2);
  - `OamDynamicConfig` struct + `oamDynamicInit(&cfg)` replace the
    5-arg positional `oamInitDynamicSprite` (B.3);
  - migrated 9 call sites across 5 example games + reverted a
    Y-1 over-compensation in the dynamic engine that was lifting
    Mario 1–2 px off the ground on flat terrain (B.4);
  - the NMI handler auto-flushes the dynamic engine via a new
    `dynamic_flush_hook` indirect call — main loops drop the
    explicit `oamInitDynamicSpriteEndFrame` / `oamVramQueueUpdate`
    pair (B.5);
  - legacy `oamInitDynamicSprite` and `oamDynamic{8,16,32}Draw`
    retire from the public header; their ASM symbols stay as the
    internal mechanism (B.6 modest);
  - `oamMetaDrawDyn(id, x, y, meta, gfx, OBJ_SMALL/OBJ_LARGE)`
    replaces `oamMetaDrawDyn{8,16,32}` — engine resolves pixel size
    from the size pair set at init (B.6 aggressive part 1);
  - simple init flushes (likemario, slopemario, dynamic_sprite)
    migrate to a single `WaitForVBlank()` that fires the auto-flush
    hook under force blank (B.6 aggressive part 2 partial);
  - `oamDynamicDrainQueue()` lets the multi-VBlank init-time drain
    happen via the same NMI auto-flush, with a draining flag that
    inhibits the end-frame hide step during the drain — which lets
    the legacy `oamInitDynamicSpriteEndFrame` / `oamVramQueueUpdate`
    retire from the public header for good (B.6 aggressive part 2
    complete).
- **NMI handler gains a `dynamic_flush_hook` 24-bit function pointer**
  in the bank-$00 register area. Defaults to a single-`rtl` no-op
  stub; `oamInitDynamicSprite` repoints it at `oamDynamicNmiFlush`
  which calls `oamInitDynamicSpriteEndFrame` + `oamVramQueueUpdate`
  every VBlank. Cost for ROMs that don't use the dynamic engine: one
  PEA + JML indirect + RTL ≈ 25 cycles per frame.

### Fixed

- **`lib/source/hdma.c:89`** — `sine_quarter[63] = 256` truncated to 0 in
  `u8`, leaving a 1-pixel notch at sin(90°) on every HDMA sine effect.
  Cap the table at 255.
- **`examples/games/tetris/render.c:346`** — `u8 tile` couldn't hold
  `TILE_BORDER_H | PAL1 = 0x409`, dropping the palette bits silently and
  rendering the line-clear flash with the wrong palette. Promote to `u16`.
- **`examples/memory/sa1_starfield/`** — `USE_FASTROM := 1` eliminates
  the 50 % VBlank lag (150/300 frames → 0/300). The 128-bird OAM-update
  loop was running into the next NMI on SlowROM.
- **Hooks `pre-commit-check.sh` / `mark-tests-passed.sh`** — outputted
  `{"decision": "allow"}` (invalid for the Claude Code schema) and
  pointed contributors to non-existent test scripts. Now run silently
  on the pass path and reference the real
  `node test/run-all-tests.mjs --quick` runner.
- **`.gitignore` cleanup** — `.claude/` entry was a blanket gitignore that
  hid project-wide policies. Replaced with the precise
  `.claude/settings.local.json` exclusion.
- **3 hardcoded `/home/kobenairb/...` paths** in `.claude/rules/` and
  skills replaced with `$PVSNESLIB_HOME`.
- **Two dead unit fixtures** (`unit/animation`, `unit/entity`) referenced
  modules removed from `lib/`. Build phase passed from 76/78 to 76/76.
- **2 stale visual baselines** (`graphics/sprites/dynamic_metasprite`,
  `maps/slopemario`) regenerated after recent ASM/c-bit fixes; baseline
  for `memory/sa1_starfield` captured for the first time after the
  FastROM bump.
- **12 latent C warnings** surfaced by the new clang lint pipeline —
  unused locals, dead helper functions (`writestring_bg2`, `refresh`),
  unused parameters in callback ABI signatures.

### Build

- **Bank $00 ROM overflow check** runs on every link
  (`devtools/symmap/symmap.py --check-bank0-overflow` invoked from
  `make/common.mk`). String-literal spills now fail the build instead of
  producing garbage at runtime. `SKIP_BANK0_CHECK=1` escapes if needed.
- **`make verify-toolchain`** compares each compiler-submodule HEAD
  against `compiler/PINS.md`. CI calls it before every build — drift
  fails fast with a fix-it message naming the right remediation.
- **clang `-fsyntax-only -Wall -Wextra -Werror`** runs in parallel with
  cc65816 in the `%.c.o` rule (cproc ignores `-W*`). The SDK is
  warning-clean and stays that way. `SKIP_LINT=1` for environments
  without clang.
- **Sed QBE→WLA-DX transform removed.** Empirical audit showed 9 of 10
  patterns matched nothing in current QBE output (QBE emits `.db`/`.dw`/
  `.dl` natively). The 10th pattern only deleted `/* end */` comments
  that WLA-DX accepts. `lib/Makefile` lost 12 lines of fragile sed.
- **Memmap dependency tracking** — `wrap_asm` consumers now list
  `$(MEMMAP_INC)` as a real prerequisite. `touch templates/memmap.inc`
  triggers a rebuild instead of producing stale objects.
- **FastROM Makefile flag** documented; SA-1 Starfield uses it.

### CI

- **Functional-tests job** — opensnes-emu test suite gates PR merges
  on Linux. Caches WASM core build by `bridge.cpp` hash; cache hit
  brings full-suite runtime to ~3 minutes.
- **Tag-on-main guard** — `release.yml` rejects a tag whose commit is
  not reachable from `origin/main`. Tags can only be cut from the
  release-PR merge.
- **Lint workflow** (`lint.yml`) — runs `lint_commits.py` on every PR
  and direct push to `main`/`develop`.

### Testing

- **Visual-regression baselines** carry provenance metadata
  (`rom_sha256`, `snes9x_commit`, `captured_at`). The runner emits
  `[WARN]` on drift instead of silently producing a misleading diff.
- All 53 baselines re-captured against the current `make`-built ROMs.
- **Compiler test runner** marks 5 unimplemented optimisations as
  `knownBug()` instead of `fail()` — TCO (3 tests), A-cache through
  pha (1), and the stale `leaf_opt=1` marker for non-leaf functions
  (1). Without `--allow-known-bugs` they still fail; with the flag
  they report `[KNOWN_BUG]` so a contributor implementing one sees
  green immediately.

### Tooling

- **`devtools/verify_toolchain.py`** — parses `compiler/PINS.md` and
  enforces submodule-pin invariants.
- **`devtools/lint_commits.py`** — Conventional-Commits subject
  validation + `Co-Authored-By:` rejection.

### Documentation

- **`compiler/ABI.md`**, **`KNOWN_LIMITATIONS.md`**,
  **`tools/opensnes-emu/test/BASELINES.md`**, **`lib/contrib/README.md`**
  added as canonical references.
- **`README.md`** gains the audience-explicit section, chip maturity
  table, and a prominent link to `KNOWN_LIMITATIONS.md`.
- **`ROADMAP.md`** resynchronised against current state.
- **`CONTRIBUTING.md`** documents the branching policy.
- **`.claude/rules/release.md`** has the full release flow with ASCII
  diagram and `make verify-toolchain` in the pre-release checklist.
- **5 stale "212 checks" / "52 examples"** references corrected across
  `.claude/rules/*.md` and the README.

## [0.13.0] - 2026-03-26

### SuperFX Phase 3-4 — Mandelbrot + Wireframe Cube

- **FMULT validated**: 4.12 fixed-point multiply confirmed (2.0×2.0=4.0, 1.5×3.0=4.5).
  Formula: `result_4_12 = FMULT_output << 4` (4× ADD R0).
- **Mandelbrot fractal**: 256×128 computed by GSU via FMULT+PLOT, 16-color palette,
  4.12 fixed-point iteration (z=z²+c, 15 max iterations). LOOP instruction for
  Y iteration (body exceeds BNE 8-bit range). CACHE for ~3× speedup.
- **Wireframe cube**: 12-edge Bresenham line drawing via PLOT, Y+X axis rotation
  computed in C (sin/cos table), orthographic projection. Chunked 4×4KB VBlank DMA
  (no flicker, ~15 FPS). Function splitting to avoid framesize>255.
- **WRAM stub overflow fix**: stub grew to 90 bytes with parameterized SCBR/R8,
  gsu_wram_area increased from 64 to 96 bytes.
- **Mesen2 backward branch bug**: confirmed via bsnes comparison. BNE/BRA+STW
  works on bsnes (cycle-accurate) but corrupts on Mesen2. bsnes is the reference
  emulator for SuperFX testing.

### Documentation

- **READMEs + screenshots**: all 4 SuperFX examples fully documented with
  emulator compatibility table (bsnes/Mesen2/snes9x).
- **EXAMPLES_BY_CATEGORY.md**: added Enhancement Chips section (SA-1 + SuperFX).
- **LEARNING_PATH.md**: added Level 6 (Enhancement Chips).
- **REGISTERS.md**: added SA-1 ($2200-$230E) and GSU ($3000-$303F) register tables.
- **MEMORY_MAP.md**: added SA-1 I-RAM/BW-RAM and SuperFX SRAM/cache layouts.

### Examples (56 total, +4 new SuperFX)

- **superfx_hello**: boot diagnostic + SRAM byte/word + FMULT 4.12 validation
- **superfx_bitmap**: 16-color gradient via PLOT hardware
- **superfx_mandelbrot**: fractal set via FMULT + PLOT (4.12 fixed-point)
- **superfx_3d**: rotating wireframe cube (Bresenham + PLOT, 2-axis rotation)

## [0.12.0] - 2026-03-23

### SuperFX (GSU) Enhancement Chip Support (NEW)

- **SuperFX coprocessor**: full build system support (`USE_SUPERFX=1`).
  Two-stage GSU assembly pipeline: `.sfx` -> `wla-superfx` -> `.sfx.bin` -> `.incbin`.
- **PLOT bitmap rendering**: GSU renders 16-color gradient via hardware PLOT
  instruction at 21.47 MHz with CACHE optimization. Column-major tile layout,
  pixel cache flush via RPIX.
- **Three critical GSU rules discovered**:
  1. Branch delay slot -- NOP after every BNE/BRA (instruction always executes)
  2. STOP pre-fetch -- NOP padding before STOP (pipeline halts prematurely)
  3. RPIX pixel cache flush -- last 8 pixels per row stay in internal cache
- **superfx_hello**: boot diagnostic with STB/STW SRAM write tests
- **superfx_bitmap**: 16-color rainbow gradient rendered by GSU PLOT hardware
- **superfx.h**: complete GSU register definitions ($3000-$303F)
- **WRAM execution**: mandatory for all GSU launches (ROM bus exclusive)
- **21.47 MHz + CACHE**: ~6x speedup over uncached 10.74 MHz baseline

### SA-1 Enhancement Chip

- **SA-1 murmuration demo**: 128 dots in Lissajous sine patterns at 10.74 MHz.
  Uses `lda.l sine_table,x` (opcode $BF) for DB-independent ROM reads.
  4 brightness palettes for depth illusion on dark blue background.

### Build System

- **Unified memmap files**: deleted 3 duplicate `lib/source/lib_memmap*.inc`,
  single source of truth in `templates/memmap*.inc`.
- **runtime.asm moved to lib/**: compiled once per config instead of per-example.
- **Per-example SA-1/SuperFX boot**: `project_sa1_boot.asm` generated from
  local override or template default.

### Documentation

- **SuperFX tutorial**: `docs/tutorials/superfx.md` covering architecture,
  PLOT rendering, assembly rules, WRAM execution, emulator compatibility.
- **SA-1 tutorial**: `docs/tutorials/sa1.md` with I-RAM patterns and debugging.
- **GETTING_STARTED.md rewritten**: Game Developer vs SDK Developer paths.
- **All examples documented**: 54 READMEs + screenshots (opensnes-emu).
- **Documentation audit**: fixed broken links, updated example count, added
  tutorials index.

## [0.11.0] - 2026-03-21

### SA-1 Enhancement Chip Support (NEW)

- **SA-1 coprocessor**: full build system support for SA-1 cartridges
  (`USE_SA1 := 1`). Includes ROM header (cart type $35), memory maps,
  post-link $FFD5 patching, and per-example boot stub override.
- **SA-1 boot infrastructure**: crt0.asm initializes SA-1 (reset vector,
  I-RAM write protection SIWP=$FF, release from reset, magic byte handshake).
- **Per-example SA-1 boot**: each SA-1 example provides its own `sa1_boot.asm`.
  crt0.asm includes `project_sa1_boot.asm` (local override or template default).
- **sa1.h / sa1.c**: register definitions ($2200-$230E), `sa1Init()` function.
- **Key discovery**: SIWP/CIWP bit=1 means WRITABLE (not protected despite
  the register name). Documented in `.claude/SA-1.md`.

### Build System

- **Unified memmap files**: deleted 3 duplicate `lib/source/lib_memmap*.inc`,
  all 18 library ASM files now reference `templates/memmap*.inc` via
  `-I ../templates`. Single source of truth for memory maps.
- **runtime.asm moved to lib/**: compiled once per config (lorom/hirom/sa1)
  instead of 46 times per example. `RUNTIME_OBJ` always linked from library.

### Compiler

- **WLA-DX .ACCU/.INDEX override warning**: detects when `rep`/`sep` tracking
  diverges from explicit `.ACCU`/`.INDEX` directives after branch merges.
  Flags reset at `.ENDS` boundaries to avoid false positives.
- **Leaf optimization fix**: `leaf_opt = fn->leaf && !fn->dynalloc` (was
  `!fn->dynalloc`). Prevents non-leaf functions from corrupting JSL return
  addresses with SSA temporaries.

### Runtime

- **Dynamic sprite state moved to bank $00**: RAMSECTION relocated from
  bank $7E slot 2 to bank 0 slot 1 — C code can now access variables
  via `lda.l $xxxx` (WRAM mirror, below $2000).

### Examples (52 total, +3 new)

- **sa1_hello**: SA-1 boot diagnostic — displays coprocessor status codes.
- **sa1_speed**: SA-1 32-bit counter at 10.74 MHz — shows 69K increments/frame.
- **sa1_starfield**: 128-dot murmuration with Lissajous sine patterns.
  4 sine lookups per bird using `lda.l sine_table,x` (DB-independent ROM reads),
  4 brightness palettes for depth illusion on dark blue background.

### Documentation

- **SA-1 tutorial**: `docs/tutorials/sa1.md` — architecture, setup, I-RAM
  communication patterns, assembly tips, Mesen2 debugging guide.
- **GETTING_STARTED.md rewritten**: two clear paths — "Game Developer" (download
  release, just needs `make`) vs "SDK Developer" (clone, build from source).
- **6 missing READMEs added**: all 52 examples now have README.md + screenshots
  (generated via opensnes-emu headless API).
- **Documentation audit**: fixed 4 broken links, updated example count 41→52,
  added SA-1 to all doc indexes, added tutorials table to docs/README.md.

## [0.10.0] - 2026-03-21

### Library

- **Dynamic metasprite engine**: `oamMetaDrawDyn32()`, `oamMetaDrawDyn16()`,
  `oamMetaDrawDyn8()` — multi-tile sprite characters with per-frame VRAM streaming.
  Iterates MetaspriteItem arrays and calls the proven oamDynamic*Draw functions
  per sub-sprite. oamMetaDrawDyn16 includes `sprsize` parameter for correct
  name table bit handling in SMALL mode.

### Build System

- **Eliminated `combined.asm`**: each ASM source (crt0, runtime, data_init_start,
  user ASMSRC) is now compiled as a separate object file. Fixes HiROM ROMBANKMAP
  linker errors and removes all `cat >>` file concatenation.
- **common.mk refactoring**: 529 → 303 lines (-43%), 32 → 15 conditionals (-53%).
  New `wrap_asm` macro factors the duplicated memmap-include-and-assemble pattern.
  `_HAS_SOUNDBANK` computed once replaces 6 nested conditional blocks. `$(if)`
  one-liners replace multi-line ifeq/else/endif blocks.
- **smconv patched**: generates FORCE sections with `.ORG 0` and `SOUNDBANK_BANK`
  in header directly — eliminates 3 sed post-processing calls.
- **Soundbank multi-bank support**: soundbanks >32KB correctly split across
  multiple ROM banks with FORCE placement. Tested with 56KB (LoROM, 2 banks)
  and 59KB (HiROM, 2 banks) soundbanks.
- **HiROM soundbank support**: soundbank assembled as separate object to avoid
  ROMBANKMAP conflicts. HiROM `.ORG $8000` override for bank mirror access.
- **HiROM text module fix**: `text.asm` and `text4bpp.asm` now include the correct
  HiROM memmap conditionally (was hardcoded to LoROM, blocking all HiROM+text builds).
- **HiROM ROMBANKS**: increased from 4 to 8 (512KB) to accommodate large soundbanks.
- **macOS compatibility**: `sed -i.bak` instead of `sed -i` for BSD sed portability.
- **Header template**: ROM_NAME via single sed, numeric values (CARTRIDGETYPE,
  ROMSIZE, SRAMSIZE) via `.DEFINE` in project_config.inc.
- **Fixed `make clean`**: removed stale `tests/` directory reference.

### Compiler

- **Zero warnings**: 72 Clang warnings fixed across cproc (67) and QBE (5).
  Strict prototypes, switch default cases, operator precedence parentheses,
  assignment-as-condition, sign comparison, unused variables.

### Tools

- **Zero warnings**: gfx4snes, smconv, img2snes, tmx2snes — strict prototypes,
  uninitialized variables, const qualifiers, missing newlines, duplicate
  instrument name handling in smconv (appends `_N` suffix).

### Examples (49 total, +3 new)

- **dynamic_metasprite**: port of PVSnesLib DynamicEngineMetaSprite — 3 OBJSEL
  configurations (8/16, 8/32, 16/32) selectable via D-PAD, glitch-free transitions.
- **snesmod_music_large**: "What Is Love" 108KB IT module — validates multi-bank
  LoROM soundbank (56KB across 2 banks).
- **snesmod_music_hirom**: "What Is Love" 210KB IT module — validates HiROM
  soundbank support with 64KB banks.
- **likemario**: renamed ACT_STAND/WALK/JUMP/FALL to MARIO_ACT_* to avoid
  conflict with map.h bitmask definitions.

## [0.8.0] - 2026-03-15

### opensnes-emu — Debug Emulator (NEW)

- **SNES debug emulator** powered by snes9x (WASM): load ROMs, run frames, capture
  screenshots, inspect VRAM/CGRAM/OAM/CPU/PPU state programmatically.
- **MCP server** with 14 tools for Claude Code integration (stdio transport).
- **Single source of truth**: `tests/` directory removed. All testing handled by
  `node tools/opensnes-emu/test/run-all-tests.mjs` — 212 checks across 7 phases.
- **Visual regression**: pixel-exact screenshot baselines for all 41 examples.
- **Lag frame detection**: measures steady-state lag over 300 frames per example.
- **Compiler benchmark**: cycle count comparison via `run-benchmark.mjs`.

### Compiler

- **fixMul/fixLerp**: rewritten in assembly using SNES hardware multiplier ($4202/$4203).
  C version overflowed because `(s32)a*(s32)b` compiled to 16-bit `__mul16`.
- **Alloc TSA**: alloc instruction now computes absolute stack address via `TSA+offset`,
  preventing aliasing with tcc compiler registers at $0000-$001F.
- **Benchmark baseline**: 33 functions, 1318 total cycles.

### Library

- **BG_4COLORS0 palette banking**: `bgInitTileSet` now computes CGRAM offset as
  `bgNumber*32 + paletteEntry*4` for Mode 0 (matches PVSnesLib).
- **HDMA channel 7 warning**: NMI handler uses DMA channel 7 for OAM — documented
  in hdma.h, window/transparent_window examples fixed to use channels 4+5.
- **fixMul/fixLerp/fixDiv**: hardware multiplier assembly (lib/source/math.asm).
- **objUpdateXY**: header corrected — parameter is raw index, not handle.
- **MultiPlayer5**: new mp5 module for multitap adapter support.

### Runtime

- **NMI handler**: MP5/mouse/scope extracted to SUPERFREE sections. Auto-joypad wait
  before all input reading. OAM DMA revert (preconfigured channel optimization caused
  Mesen2 regression — snes9x didn't catch it).

### Examples (41 total, +4 new)

- **Mode 0**: 4-layer 2bpp Kirby parallax demo (ported from PVSnesLib).
- **Mode 3**: 256-color 8bpp static display with split DMA loader.
- **Mode 5**: Hi-res 512×256 16-color display.
- **Tetris**: Korobeiniki music (SNESMOD), multi-line clear fix, RNG fix, gravity fix.
- **All 41 examples** now have README.md + screenshot.png.

### Documentation

- Streamlined root README (328→150 lines).
- Removed MATURITY_REVIEW.md (replaced by opensnes-emu real metrics).
- Updated all docs for 41 examples and opensnes-emu workflow.

### Removed

- `tests/` directory (17,701 lines) — fully migrated to opensnes-emu.
- `devtools/benchmark/` and `devtools/check_mvn/` — handled by opensnes-emu.
- `examples/benchmark/` — opensnes-emu handles benchmarking.

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
