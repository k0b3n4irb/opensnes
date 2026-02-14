# Testing Rules (Auto-loaded)

These rules are MANDATORY for every session. No exceptions.

## Before ANY Code Change

1. **Read the full protocol**: Open and consult `.claude/TESTING_PROTOCOL.md`
2. **Classify the change**: A (compiler), B (library/runtime), C (example), D (tools/build)
3. **Write the test FIRST** (TDD): The test must FAIL before the fix and PASS after

## Post-Change Workflow — STRICT 3-STEP GATE

After EVERY modification (code, library, compiler, example, build system):

### Step 1: Run ALL automated tests (Claude does this)

**CRITICAL: NEVER use partial rebuilds** (`make lib`, `make -C examples/...`) before testing.
Partial rebuilds can leave stale artifacts that produce non-reproducible test results.
The ONLY allowed build command before manual testing is `make clean && make` from root.

```bash
make clean && make                                                      # Full rebuild FROM ROOT
./tests/compiler/run_tests.sh                                           # Compiler tests
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick      # Example validation
```

**ALL THREE must pass.** If any fails: STOP, REVERT, FIX.

### Step 2: Hand off to user for manual validation (BLOCKING)

After automated tests pass, **STOP and ask the user** to validate the 7 reference
examples in Mesen2. Do NOT commit. Do NOT proceed. Wait for explicit user approval.

Say something like:
> "Tous les tests automatisés passent (X/X compiler, Y/Y examples).
> Peux-tu valider les 7 exemples de référence dans Mesen2 ?
> Une fois validé, je committerai le checkpoint."

### Step 3: Commit checkpoint (only after user approval)

Once the user confirms validation, create an atomic checkpoint commit:
- Include ALL changes since the last checkpoint
- Include submodule updates if applicable
- Message format: `type(scope): description` with test results
- This creates a clean rollback point for regression analysis

```
CHANGE → auto tests → USER VALIDATES → commit checkpoint
                ↑              ↑              ↑
            Claude runs    User runs      Claude commits
            (mandatory)    (mandatory)    (after approval)
```

**NEVER skip Step 2. NEVER commit without user approval.**

## 7 Reference Examples (User validates in Mesen2)

| # | Example | Path | What to check |
|---|---------|------|---------------|
| 1 | Breakout | `examples/games/breakout/` | Ball bounces, paddle moves, blocks break |
| 2 | LikeMario | `examples/games/likemario/` | Mario walks/jumps, camera scrolls |
| 3 | HDMA Wave | `examples/graphics/effects/hdma_wave/` | Wavy distortion visible |
| 4 | Dynamic Sprite | `examples/graphics/sprites/dynamic_sprite/` | 4 animated sprites at y=100 |
| 5 | Continuous Scroll | `examples/graphics/backgrounds/continuous_scroll/` | BG scrolls with D-PAD |
| 6 | SFX Demo | `examples/audio/sfx_demo/` | Sound plays on button press |
| 7 | HiROM Demo | `examples/audio/snesmod_hirom/` | Music plays, HiROM works |

## Atomic Changes Only

- ONE logical change per commit
- NEVER combine: compiler change + library change + example fix
- If a test fails after a change: STOP, REVERT, INVESTIGATE

## Checkpoints = Regression Safety

Each checkpoint commit is a known-good state. If a future change breaks something:
1. `git log --oneline` to find the last checkpoint
2. `git diff <checkpoint>..HEAD` to see what changed
3. `git checkout <checkpoint>` to return to a working state
4. Bisect between checkpoints to find the breaking change

## NEVER Commit Without Tests

- Every compiler change needs a test in `run_tests.sh`
- Every memory layout change needs a check in `validate_examples.sh`
- If you think "this is too small for a test" -- it NEEDS a test
