# Debug Test Fixtures

Test fixtures for validating OpenSNES debugging tools.

## Structure

```
debug-fixtures/
├── clean/                  # Working examples (should all pass)
│   └── hello_world/        # Minimal working example
│
└── broken/                 # Intentional bugs (MUST be caught)
    └── wram_overlap/       # oamMemory at $7E:0000 overlapping Bank $00 vars
```

## Fixture Format

Each fixture directory contains:
- `*.sym` - WLA-DX symbol file
- `expected_error.txt` - (broken fixtures only) Text that MUST appear in tool output

## Running Validation

```bash
./tools/validate_debug_tools.sh
```

## Adding New Fixtures

### Clean Fixtures
1. Copy a working `.sym` file from `examples/`
2. Verify: `python3 tools/symmap/symmap.py --check-overlap your_fixture.sym`
3. Should exit with code 0 (success)

### Broken Fixtures
1. Create a `.sym` file that triggers the bug
2. Create `expected_error.txt` with the error message substring
3. Verify: `python3 tools/symmap/symmap.py --check-overlap your_fixture.sym`
4. Should exit with code 1 and output should contain expected error

## Known Bug Types

### wram_overlap

**Bug:** RAMSECTION in Bank $7E placed at $0000-$1FFF overlaps with Bank $00 variables due to WRAM mirroring.

**Real-world example:** The 2_animation example had oamMemory at $7E:0000, which overwrote monster_x at $00:0022 when the OAM buffer was cleared.

**Fix:** Use `ORGA $0300 FORCE` to place Bank $7E sections above the mirror range.

**Learn more:** See `.claude/KNOWLEDGE.md` Section 3 (WRAM Mirroring)
