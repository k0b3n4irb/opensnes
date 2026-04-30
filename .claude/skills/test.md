---
name: test
description: Run OpenSNES test suite (compiler tests, example validation, visual regression)
argument-hint: "[all|compiler|validate|<example-name>]"
allowed-tools: Bash(*), Read
---

# /test - Run Tests

Run tests for OpenSNES SDK.

## Usage
```
/test                    # Run all tests
/test black-screen       # Run black screen tests (mandatory before commit)
/test compiler           # Run compiler tests
/test validate           # Validate all examples build
/test <example>          # Test specific example in Mesen2
```

## Requirements
- Mesen2 emulator for ROM testing

## Test Scripts

| Script | Purpose |
|--------|---------|
| `tests/run_black_screen_tests.sh` | Detect broken ROMs (mandatory) |
| `tests/run_tests.sh` | General ROM tests with Mesen2 |
| `tests/compiler/run_tests.sh` | Compiler regression tests |
| `tests/examples/validate_examples.sh` | Build validation |

## Implementation

### Black Screen Test (Mandatory)
```bash
./tests/run_black_screen_tests.sh /path/to/Mesen
```
Detects broken ROMs by checking if they display content after 90 frames.

### Compiler Tests
```bash
./tests/compiler/run_tests.sh
```

### Example Validation
```bash
./tests/examples/validate_examples.sh --quick
```

### Single ROM Test
```bash
/path/to/Mesen examples/$1/*.sfc --testrunner --lua tests/mesen/test.lua
```

## Reporting
After testing, report:
1. Tests passed/failed
2. Any build errors
3. Screenshots saved for failed tests
