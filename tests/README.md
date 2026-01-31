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
| hello_world | text/1_hello_world | Basic boot, hardware init, VBlank |
| custom_font | text/2_custom_font | Font loading, message display |
| animation | graphics/2_animation | **WRAM mirroring fix**, sprite movement, OAM transfer |
| tone | audio/1_tone | SPC700 init, audio playback |

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

```c
// test_my_feature.c
#include <snes.h>
#include "test_harness.h"

void test_feature_basic(void) {
    // Arrange
    int input = 5;

    // Act
    int result = my_feature(input);

    // Assert
    TEST_ASSERT_EQUAL(10, result);
}

int main(void) {
    test_init();

    RUN_TEST(test_feature_basic);

    test_report();
    return 0;
}
```

### 4. Write Mesen2 Test Script

```lua
-- test_my_feature.lua
local test = require("test_harness")

test.init()
test.run_until_complete(5000)  -- 5 second timeout
test.check_results()
test.exit()
```

## Test Harness API

### C API (test_harness.h)

```c
// Initialize test system
void test_init(void);

// Run a test function
void RUN_TEST(void (*test_func)(void));

// Assertions
void TEST_ASSERT(int condition);
void TEST_ASSERT_EQUAL(int expected, int actual);
void TEST_ASSERT_EQUAL_U16(u16 expected, u16 actual);
void TEST_ASSERT_EQUAL_PTR(void* expected, void* actual);
void TEST_ASSERT_MEM_EQUAL(void* expected, void* actual, u16 size);

// Mark test result
void TEST_PASS(void);
void TEST_FAIL(const char* message);

// Report results (writes to known memory address for Mesen2)
void test_report(void);
```

### Lua API (test_harness.lua)

```lua
-- Initialize test runner
test.init()

-- Run emulator until test completes or timeout
test.run_until_complete(timeout_ms)

-- Check test results from memory
test.check_results()

-- Read memory
test.read_byte(address)
test.read_word(address)

-- Exit emulator with result code
test.exit()
```

## Memory Layout for Test Results

Tests report results to fixed memory addresses:

| Address | Size | Description |
|---------|------|-------------|
| $7F0000 | 1 | Test status: 0=running, 1=pass, 2=fail |
| $7F0001 | 2 | Tests run count |
| $7F0003 | 2 | Tests passed count |
| $7F0005 | 2 | Tests failed count |
| $7F0010 | 64 | Failure message (null-terminated) |

## Continuous Integration

The CI pipeline runs automatically on:
- Every push to `main` or `develop` branches
- Every pull request to `main` or `develop`

### What CI Checks

| Check | Description |
|-------|-------------|
| Build toolchain | Compiles cc65816, WLA-DX, gfx4snes, smconv |
| Build examples | Builds all 27 example ROMs |
| Build tests | Compiles test ROMs |
| Validate examples | Checks for memory overlaps (symmap.py) |
| Compiler tests | Runs compiler regression tests |
| Example count | Verifies all 27 examples built successfully |

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
