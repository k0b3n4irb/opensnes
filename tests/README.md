# OpenSNES Test Suite

Automated testing infrastructure for OpenSNES SDK.

## Philosophy

**Every feature must have tests.** We follow a test-first development approach:
1. Write test specification
2. Implement feature
3. Verify tests pass
4. Document the feature

## Test Categories

| Category | Description | Location |
|----------|-------------|----------|
| `black_screen` | Smoke test - detects broken ROMs | `tests/black_screen_test.lua` |
| `examples/` | Example ROM tests (snesdbg-based) | `tests/examples/` |
| `unit/` | Low-level function tests (math, memory) | `tests/unit/` |
| `hardware/` | SNES hardware feature tests | `tests/hardware/` |
| `integration/` | Full system tests | `tests/integration/` |
| `compiler/` | Compiler regression tests | `tests/compiler/` |

## Quick Smoke Test: Black Screen Detection

The fastest way to check if all ROMs are working is the **black screen test**. A black screen after initialization typically means the ROM is broken.

```bash
# Run black screen test on all examples
./tests/run_black_screen_tests.sh /path/to/Mesen

# With verbose output
./tests/run_black_screen_tests.sh /path/to/Mesen --verbose

# Save screenshots for inspection
./tests/run_black_screen_tests.sh /path/to/Mesen --save-screenshots
```

**How it works:**
1. Loads each ROM in Mesen
2. Waits 90 frames (1.5 seconds)
3. Analyzes screen buffer for non-black pixels
4. PASS if >0.5% of pixels are non-black
5. FAIL if screen is black (likely broken ROM)

**Output:**
- Screenshots of failed ROMs saved to `tests/screenshots/FAIL_*.png`
- Exit code 0 = all pass, 1 = failures detected

## Example Tests

The `examples/` category tests the working example ROMs using the `snesdbg` Lua library:

| Test | ROM | What it tests |
|------|-----|---------------|
| hello_world | text/hello_world | Basic boot, hardware init, VBlank |
| custom_font | text/custom_font | Font loading, message display |
| animation | graphics/animation | **WRAM mirroring fix**, sprite movement, OAM transfer |
| tone | audio/tone | SPC700 init, audio playback |

Run example tests:
```bash
./run_tests.sh examples
```

## Prerequisites

- **Mesen2 Emulator**: Required for running ROM-based tests
  - macOS: Build from source or use release
  - Path: Set `MESEN_PATH` environment variable

```bash
export MESEN_PATH=/path/to/Mesen
```

## Running Tests

```bash
# Run all tests
./run_tests.sh

# Run specific category
./run_tests.sh unit
./run_tests.sh hardware

# Run single test
./run_tests.sh unit/math

# Verbose output
./run_tests.sh -v
```

## Test Structure

Each test is a directory containing:

```
tests/unit/math/
├── README.md           # Test documentation
├── Makefile           # Build configuration
├── test_math.c        # Test source code
├── test_math.lua      # Mesen2 test runner script
└── expected.txt       # Expected results (optional)
```

## Writing Tests

### 1. Create Test Directory

```bash
mkdir -p tests/unit/my_feature
```

### 2. Write Test Documentation (README.md)

```markdown
# my_feature Test

## Purpose
Tests the my_feature functionality.

## What is Tested
- Feature behavior A
- Edge case B
- Error condition C

## Expected Results
- All assertions pass
- No crashes or hangs
```

### 3. Write Test Code

Unit tests use a simple inline pattern with `TEST()` macros (see `tests/unit/` for examples):

```c
// test_my_feature.c
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

> **Note:** `tests/harness/test_harness.h` exists but is **deprecated** — its labels
> use a `_` prefix that breaks WLA-DX cross-object resolution. All unit tests use
> the inline `TEST()` macro pattern shown above instead.

## Continuous Integration

The CI pipeline runs automatically on:
- Every push to `main` or `develop` branches
- Every pull request to `main` or `develop`

### What CI Checks

| Check | Description |
|-------|-------------|
| Build toolchain | Compiles cc65816, WLA-DX, gfx4snes, smconv |
| Build examples | Builds all 25 example ROMs |
| Build tests | Compiles test ROMs |
| Validate examples | Checks for memory overlaps (symmap.py) |
| Compiler tests | Runs compiler regression tests |
| Example count | Verifies all 25 examples built successfully |

### CI Workflow

GitHub Actions workflow: `.github/workflows/opensnes_build.yml`

Runs on 3 platforms:
- **Linux** (ubuntu-latest)
- **Windows** (windows-latest with MSYS2)
- **macOS** (macos-latest)

### Local CI Validation

Before pushing, you can run the same checks locally:

```bash
# Full build
make clean && make

# Validate all examples (checks for memory overlaps, ROM sizes)
./tests/examples/validate_examples.sh --quick

# Run compiler tests
./tests/compiler/run_tests.sh

# Verify example count
echo "Examples built: $(find examples -name '*.sfc' | wc -l)"
```

### Artifacts

CI uploads the following artifacts for debugging:
- **Build logs**: Full build output for each platform
- **Example ROMs**: All compiled .sfc files
- **Documentation**: Generated API docs (from Linux build)

## Coverage Goals

| Component | Target Coverage |
|-----------|----------------|
| Math functions | 100% |
| Memory management | 100% |
| Input handling | 90% |
| Sprite system | 80% |
| Background system | 80% |
| Audio system | 70% |

## Adding New Test Categories

1. Create directory under `tests/`
2. Add to `run_tests.sh` category list
3. Document in this README
4. Add to CI workflow
