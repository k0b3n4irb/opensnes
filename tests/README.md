# OpenSNES Test Suite

Automated testing infrastructure for OpenSNES SDK.

## Quick Reference

```bash
# Before every commit — the 3 mandatory scripts
./tests/compiler/run_tests.sh --allow-known-bugs        # 53/54 compiler tests
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick  # 25/25 ROM validation
./tests/unit/run_unit_tests.sh                           # 19 unit test modules
```

## Test Categories

| Category | Script | What it does |
|----------|--------|-------------|
| **Compiler** | `tests/compiler/run_tests.sh` | 54 regression tests (C → ASM → grep patterns) |
| **Examples** | `tests/examples/validate_examples.sh` | Memory overlap + ROM size for all 25 examples |
| **Unit** | `tests/unit/run_unit_tests.sh` | 19 test modules (~385 runtime + 119 compile-time assertions) |
| **CI** | `tests/ci/check_memory_overlaps.sh` | CI-specific overlap checking |

## Compiler Tests (`tests/compiler/`)

```bash
./tests/compiler/run_tests.sh [-v] [--allow-known-bugs]
```

54 tests that compile C files and verify the generated assembly (grep on .asm output).
Covers: codegen, types, operators, control flow, variables, promotions, optimizations.

**Status:** 53/54 pass. 1 known bug (`comparisons: test_s16_shift_right`).

## Example Validation (`tests/examples/`)

```bash
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh [--quick] [--verbose]
```

Validates all 25 example ROMs:
- `--quick`: skip rebuild, validate existing .sfc files only
- Without flag: clean + rebuild each example before validation

**Checks:** build success, memory overlaps (symmap.py), ROM size (32KB-4MB, power of 2).

## Unit Tests (`tests/unit/`)

```bash
./tests/unit/run_unit_tests.sh [-v]
```

19 modules. Each is a standalone .sfc ROM that runs on SNES hardware.
The runner builds each test and checks memory overlaps via `symmap.py`.

See `tests/CLAUDE.md` for the full module breakdown (assertions vs smoke tests).

## Writing Tests

Unit tests use inline `TEST()` macros:

```c
#include <snes.h>
#include <snes/console.h>
#include <snes/text.h>

static u8 tests_passed;
static u8 tests_failed;
static u8 test_line;

#define TEST(name, condition) do { \
    if (condition) { tests_passed++; } \
    else { tests_failed++; textPrintAt(1, test_line, "FAIL:"); \
           textPrintAt(7, test_line, name); test_line++; } \
} while(0)

int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    TEST("basic", my_feature(5) == 10);

    // Display results...
    setScreenOn();
    while (1) { WaitForVBlank(); }
}
```

## Mesen2 Lua Tests

Mesen2-based tests that validate ROM behavior at runtime.

| Script | Purpose |
|--------|---------|
| `tests/black_screen_test.lua` | Black screen smoke test (PNG size proxy) |
| `tests/run_black_screen_tests.sh` | Runner: tests all 25 ROMs for black screens |
| `tests/test_snes_compliance.sh` | 8 static checks on headers and assembly |
| `tests/examples/hello_world/test_hello_world.lua` | Mesen2 test for hello_world example |
| `tests/examples/breakout_transition_test.lua` | Mesen2 test for breakout transitions |
| `tests/examples/compare_screenshots.sh` + `.lua` | PVSnesLib vs OpenSNES visual comparison |

All Mesen2 Lua scripts use the **event-driven** pattern (register callbacks, return
control). See `tests/black_screen_test.lua` for the canonical example.

## Debugging Tools (snesdbg)

`tools/snesdbg/` provides a Lua debugging library for Mesen2:
- Symbol-aware memory reads (`dbg.read("player_x")`)
- OAM inspection and comparison (shadow buffer vs hardware)
- Breakpoints and variable watches by symbol name
- BDD-style test DSL (`describe`/`it` with event-driven `done()` callbacks)
- Non-blocking frame waits (`afterFrames`) and symbol waits (`onSymbolReached`)

See `tools/snesdbg/README.md` for the full API.

## Manual Validation (7 Reference Examples)

After automated tests pass, these must be tested in Mesen2 before committing
changes to compiler, library, or examples:

| # | Example | What to check |
|---|---------|---------------|
| 1 | `examples/games/breakout/` | Ball bounces, paddle moves, blocks break |
| 2 | `examples/games/likemario/` | Mario walks/jumps, camera scrolls |
| 3 | `examples/graphics/effects/hdma_wave/` | Wavy distortion visible |
| 4 | `examples/graphics/sprites/dynamic_sprite/` | Animated sprite at center |
| 5 | `examples/graphics/backgrounds/continuous_scroll/` | BG scrolls with D-PAD |
| 6 | `examples/audio/snesmod_music/` | Music plays |
| 7 | `examples/memory/save_game/` | Save/load works |

## CI Pipeline

GitHub Actions (`.github/workflows/opensnes_build.yml`) runs on push/PR to `main`/`develop`:
- Builds toolchain, library, and all 25 examples on Linux, Windows, macOS
- Runs compiler tests and example validation
- Uploads build artifacts (ROMs, docs)
