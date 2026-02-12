# Testing Rules (Auto-loaded)

These rules are MANDATORY for every session. No exceptions.

## Before ANY Code Change

1. **Read the full protocol**: Open and consult `.claude/TESTING_PROTOCOL.md`
2. **Classify the change**: A (compiler), B (library/runtime), C (example), D (tools/build)
3. **Write the test FIRST** (TDD): The test must FAIL before the fix and PASS after

## Test-First Workflow

```
1. WRITE THE TEST   -> test fails (proves detection)
2. MAKE THE CHANGE  -> test passes (proves the fix)
3. RUN ALL TESTS    -> no regressions
4. COMMIT           -> test + change together (atomic)
```

## Required Test Commands

```bash
make clean && make                                                      # Full rebuild
./tests/compiler/run_tests.sh                                           # Compiler tests
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick      # Example validation
```

**ALL THREE must pass before any commit.**

## 7 Reference Examples (Must ALL Work)

| # | Example | Path |
|---|---------|------|
| 1 | Breakout | `examples/games/2_breakout/` |
| 2 | LikeMario | `examples/games/1_likemario/` |
| 3 | HDMA Wave | `examples/graphics/effects/2_hdma_wave/` |
| 4 | Dynamic Sprite | `examples/graphics/sprites/4_dynamic_sprite/` |
| 5 | Continuous Scroll | `examples/graphics/backgrounds/2_continuous_scroll/` |
| 6 | SFX Demo | `examples/audio/1_sfx_demo/` |
| 7 | HiROM Demo | `examples/audio/4_snesmod_hirom/` |

## Atomic Changes Only

- ONE logical change per commit
- NEVER combine: compiler change + library change + example fix
- If a test fails after a change: STOP, REVERT, INVESTIGATE

## NEVER Commit Without Tests

- Every compiler change needs a test in `run_tests.sh`
- Every memory layout change needs a check in `validate_examples.sh`
- If you think "this is too small for a test" -- it NEEDS a test
