# /test - Run Tests

Run tests for OpenSNES SDK.

## Usage
```
/test                    # Run all tests
/test build              # Build validation only (no emulator)
/test <example>          # Test specific example in Mesen2
/test audio/1_tone       # Example: test audio example
```

## Requirements
- Mesen2 emulator for ROM testing
- Default path: `/Users/k0b3/workspaces/github/Mesen2/bin/osx-arm64/Release/Mesen`

## Implementation

### Build Validation
```bash
cd /Users/k0b3/workspaces/github/opensnes

# Build all examples and check for errors
for dir in examples/*/; do
    for example in "$dir"*/; do
        if [ -f "$example/Makefile" ]; then
            echo "Building: $example"
            make -C "$example" clean
            make -C "$example" || echo "FAILED: $example"
        fi
    done
done
```

### ROM Testing
```bash
# Launch ROM in Mesen2
MESEN=/Users/k0b3/workspaces/github/Mesen2/bin/osx-arm64/Release/Mesen
ROM=/Users/k0b3/workspaces/github/opensnes/examples/$1/*.sfc

$MESEN $ROM &
```

### Lua Test Scripts (if available)
```bash
# Run Mesen2 with Lua test script
$MESEN --testrunner $ROM --lua tests/$1/test.lua
```

## Test Categories
- **Unit tests:** `tests/unit/` - C function tests
- **Integration tests:** `tests/integration/` - Full ROM tests
- **Example validation:** Build and run all examples

## Reporting
After testing, report:
1. Tests passed/failed
2. Any build errors
3. Any runtime issues observed
