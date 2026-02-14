# OpenSNES Change Validation Protocol

> **Golden Rule: One atomic change -> build -> test 7 examples -> commit. No exceptions.**
> **TDD Rule: No change without a test. Write the test FIRST. It must fail before the fix and pass after.**

This protocol MUST be followed for EVERY change to compiler, library, runtime, or examples.
It exists because a single memory layout commit (`db8e977`) broke all dynamic sprite examples
for 2 weeks, and chaotic multi-change debugging made it worse.

---

## 1. Reference Test Suite (7 Examples)

These 7 examples cover all critical subsystems. **ALL must work after every change.**

| # | Example | Path | Tests | Expected Behavior |
|---|---------|------|-------|-------------------|
| 1 | **Breakout** | `examples/games/breakout/` | Sprites, input, game logic | Ball bounces, paddle moves with D-PAD, blocks break |
| 2 | **LikeMario** | `examples/games/likemario/` | Scrolling, sprites, input, tilemaps | Mario walks/jumps, camera scrolls, tiles display |
| 3 | **HDMA Wave** | `examples/graphics/effects/hdma_wave/` | HDMA, PPU effects | Wavy distortion effect on background |
| 4 | **Dynamic Sprite** | `examples/graphics/sprites/dynamic_sprite/` | Dynamic sprites, OAM buffer | 4 animated 16x16 sprites (red/green) at y=100 |
| 5 | **Continuous Scroll** | `examples/graphics/backgrounds/continuous_scroll/` | BG scrolling, input | Background scrolls smoothly with D-PAD |
| 6 | **SFX Demo** | `examples/audio/sfx_demo/` | Audio, input | Sound effects play on button press |
| 7 | **HiROM Demo** | `examples/audio/snesmod_hirom/` | HiROM mode, audio | Music plays, HiROM mapping works |

### Failure Symptoms to Watch For

| Symptom | Likely Cause |
|---------|-------------|
| Black screen | Crash in init, bad memory layout, missing data |
| Garbled sprites | VRAM write outside VBlank, wrong OAM buffer address |
| D-PAD not working | Compiler bug (return value, unsigned compare), NMI handler issue |
| No audio | SPC700 init failure, missing audio data |
| Corrupted tiles | Wrong BG/tilemap VRAM addresses, DMA source bank issue |

---

## 2. Test-Driven Development (TDD) — MANDATORY

**Every change MUST be accompanied by a test that would catch a regression.**

No exceptions. If the oambuffer move (`db8e977`) had come with a test verifying the symbol
was in the bank $00 mirror range, it would have been caught on the same day, not 2 weeks later.

### 2.1 The TDD Cycle

```
1. WRITE THE TEST  → test fails (proves it detects the problem)
2. MAKE THE CHANGE → test passes (proves the change works)
3. RUN ALL TESTS   → no regressions (proves nothing else broke)
4. COMMIT          → test + change together (atomic, reversible)
```

### 2.2 Test Type by Change Category

| Change Type | Test To Write | Test Location | Runs In |
|-------------|--------------|---------------|---------|
| **Compiler optimization** | C file + expected ASM pattern | `tests/compiler/run_tests.sh` | `run_tests.sh` |
| **Compiler bug fix** | C file that triggers the bug + expected correct output | `tests/compiler/run_tests.sh` | `run_tests.sh` |
| **Memory layout** (crt0, RAMSECTION) | Symbol address range check | `tests/examples/validate_examples.sh` | `validate_examples.sh` |
| **Library function** | Example or unit test that calls the function | `tests/unit/<module>/` | `make tests` |
| **DMA / PPU / NMI** | Example that exercises the hardware path | Reference example suite | Manual in emulator |
| **New example** | The example itself must build + validate | `examples/` | `validate_examples.sh` |

### 2.3 Compiler Test Pattern

For every compiler change (optimization, fix, ABI change), add a test function
in `run_tests.sh`:

```bash
test_my_new_feature() {
    local name="my_new_feature"
    local src="$BUILD/test_my_new_feature.c"
    local out="$BUILD/test_my_new_feature.c.asm"
    ((TESTS_RUN++))

    # 1. Write a minimal C program that exercises the feature
    cat > "$src" << 'CEOF'
void my_function(void) {
    // ... minimal code that triggers the codegen path
}
CEOF

    # 2. Compile it
    compile_test "$name" "$src" "$out" || return

    # 3. Verify the expected assembly pattern
    if ! grep -q 'expected_instruction' "$out"; then
        log_fail "$name: missing 'expected_instruction' in output"
        ((TESTS_FAILED++))
        return
    fi

    # 4. Verify absence of the old (wrong) pattern
    if grep -q 'wrong_instruction' "$out"; then
        log_fail "$name: still emitting 'wrong_instruction'"
        ((TESTS_FAILED++))
        return
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}
```

### 2.4 Memory Layout Invariant Tests

Critical symbols MUST be verified for correct placement after every build.
These invariants are checked by `validate_examples.sh`:

| Symbol | Must Be In | Address Range | Why |
|--------|-----------|---------------|-----|
| `oambuffer` | Bank $00 | `$0000-$1FFF` | C compiler generates `sta.l $0000,x` (bank $00 only) |
| `oamMemory` | Bank $00 | `$0300-$051F` | NMI handler DMA source, must be in WRAM mirror |
| Static mutable vars | RAM | `$0000-$1FFF` | Must be writable, not ROM |
| `const` data | ROM | `$8000-$FFFF` | Must not waste RAM |

**To add a new invariant**, add a check to `validate_examples.sh` that:
1. Parses the `.sym` file for the symbol
2. Verifies the address is in the expected range
3. Fails with a clear message if violated

### 2.5 Concrete Example: The Test That Would Have Prevented the oambuffer Regression

The commit `db8e977` moved `oambuffer` from `$7E:0520` to `$7E:2000`. This test would catch it:

```bash
test_oambuffer_bank00_accessible() {
    local name="oambuffer_in_bank00_mirror"
    local sym_file="examples/graphics/sprites/dynamic_sprite/dynamic_sprite.sym"
    ((TESTS_RUN++))

    if [[ ! -f "$sym_file" ]]; then
        log_fail "$name: symbol file not found (build dynamic_sprite first)"
        ((TESTS_FAILED++))
        return
    fi

    # oambuffer MUST be in $0000-$1FFF (bank $00 WRAM mirror range)
    local addr
    addr=$(grep -E '^\s*[0-9a-fA-F]+:[0-9a-fA-F]+\s+oambuffer\b' "$sym_file" \
           | awk '{print $1}' | head -1)

    if [[ -z "$addr" ]]; then
        log_fail "$name: oambuffer symbol not found in $sym_file"
        ((TESTS_FAILED++))
        return
    fi

    # Extract the offset (after the colon)
    local offset
    offset=$(echo "$addr" | cut -d: -f2)
    local offset_dec=$((16#$offset))

    if [[ $offset_dec -ge 8192 ]]; then  # 0x2000 = 8192
        log_fail "$name: oambuffer at $addr (above $2000, not C-accessible!)"
        ((TESTS_FAILED++))
        return
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}
```

This test:
- **Before db8e977**: PASS (oambuffer at `$7E:0520`, in mirror range)
- **After db8e977**: FAIL (oambuffer at `$7E:2000`, above mirror range)
- **After our fix**: PASS (oambuffer back in bank $00, `$00:xxxx < $2000`)

### 2.6 When You Think "This Change Is Too Small for a Test"

Ask yourself: **"If someone reverts this in 6 months, what breaks?"**

- If the answer is "nothing visible immediately" → it NEEDS a test (silent regressions are the worst)
- If the answer is "build fails" → the build system IS the test (OK to skip)
- If the answer is "I don't know" → it DEFINITELY needs a test

---

## 3. Change Classification

Before making any change, classify it:

| Class | Scope | Risk | Examples |
|-------|-------|------|----------|
| **A - Compiler** | `compiler/qbe/`, `compiler/cproc/` | HIGH | Optimization phases, codegen fixes, ABI changes |
| **B - Library/Runtime** | `lib/`, `templates/common/` | HIGH | crt0.asm, sprite.c, dma, NMI handler |
| **C - Example** | `examples/` | LOW | New example, example bug fix |
| **D - Tools/Build** | `tools/`, `make/`, `.github/` | MEDIUM | Build system, asset tools, CI |

**Risk rules:**
- Class A or B: MUST run full protocol (all 7 examples)
- Class C: MUST run protocol for the modified example + 2 neighbors
- Class D: MUST run full build + validate_examples.sh

---

## 4. Validation Steps

### Step 1: Establish Baseline (Before ANY Change)

```bash
# Verify current state works
make clean && make -k
./tests/compiler/run_tests.sh
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

Record results. If baseline fails, FIX IT before proceeding.

### Step 2: Make ONE Atomic Change

- Change ONE thing only (one file, one function, one optimization)
- If a change requires touching 2+ files, that's OK if they form one logical unit
- NEVER combine: compiler change + library change + example fix

### Step 3: Automated Tests

```bash
# Rebuild
make clean && make -k

# Compiler regression tests
./tests/compiler/run_tests.sh

# Memory overlap validation
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

**If any automated test fails: STOP. Revert the change. Investigate.**

### Step 4: Manual Test — 7 Reference Examples

Test each example in Mesen2 emulator. Record results:

```
| Example          | Build | Runs | Visual | Input | Audio | Status |
|------------------|-------|------|--------|-------|-------|--------|
| breakout         | OK    | OK   | OK     | OK    | N/A   | PASS   |
| likemario        | OK    | OK   | OK     | OK    | N/A   | PASS   |
| hdma_wave        | OK    | OK   | OK     | N/A   | N/A   | PASS   |
| dynamic_sprite   | OK    | OK   | OK     | N/A   | N/A   | PASS   |
| continuous_scroll| OK    | OK   | OK     | OK    | N/A   | PASS   |
| sfx_demo         | OK    | OK   | OK     | OK    | OK    | PASS   |
| hirom_demo       | OK    | OK   | OK     | N/A   | OK    | PASS   |
```

**ALL 7 must PASS. If any fails: STOP. Revert. Investigate.**

### Step 5: Commit

Only after Steps 3 AND 4 pass:

```bash
# Commit with test results in message
git commit -m "$(cat <<'EOF'
type(scope): description

Tested: 7/7 reference examples pass (breakout, likemario, hdma_wave,
dynamic_sprite, continuous_scroll, sfx_demo, hirom_demo)

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
EOF
)"
```

### Step 6: Verify Post-Commit

```bash
# Sanity check — rebuild from committed state
make clean && make -k
./tests/compiler/run_tests.sh
```

---

## 5. Regression Analysis Protocol

When a test fails after a change:

### 5.1 Immediate Actions

1. **DO NOT make another change** — you now have TWO problems
2. **Record the exact failure** — screenshot, error message, symptoms
3. **Revert the change**: `git checkout -- <file>` or `git stash`
4. **Verify revert fixes it**: rebuild and retest the failing example

### 5.2 Bisection (If Revert Doesn't Fix)

If the problem existed before your change (pre-existing), use binary search:

```bash
# Find the range of commits
git log --oneline -20

# Test middle commit
git checkout <mid-commit> -- compiler/qbe/w65816/emit.c
make clean && make -k
# Test the failing example only

# Narrow down: older half or newer half?
# Repeat until the breaking commit is found
```

### 5.3 Root Cause Classification

| Symptom Pattern | Check First |
|-----------------|-------------|
| ALL examples broken | crt0.asm, memory layout, calling convention |
| Only C-heavy examples | Compiler codegen (emit.c), ABI |
| Only sprite examples | sprite.c, sprite_dynamic.asm, OAM buffer address |
| Only audio examples | SPC700 init, audio module |
| Only one example | That example's specific code/assets |

### 5.4 The "Never Again" Rule

After fixing a regression, MANDATORY actions:
1. **Write a regression test** that reproduces the bug (see Section 2)
2. Verify the test FAILS without the fix, PASSES with the fix
3. Add the test to the automated suite (`run_tests.sh` or `validate_examples.sh`)
4. A note in `.claude/KNOWLEDGE.md` explaining the root cause
5. Update MEMORY.md if it's a pattern that could recur

The regression test is the most important step. Without it, the same bug WILL come back.

---

## 6. Submodule Change Protocol

Submodule changes (compiler/qbe, compiler/cproc) require extra care because
they affect ALL projects.

### Order of Operations

1. Make change in submodule working tree
2. Run full validation (Steps 1-4 above)
3. Commit INSIDE the submodule
4. Push the submodule to its remote
5. `git add compiler/<sub>` in main repo
6. Commit main repo with submodule reference update
7. Run validation AGAIN from the main repo commit

### Recovery from Submodule Mess

```bash
# If submodule state is confused:
cd compiler/qbe
git status          # Check working tree
git log --oneline -5  # Where are we?
git stash           # Save work if needed (NOTE: stash is per-submodule)
git checkout main   # Reset to known state
git stash pop       # Restore work if needed
```

**CRITICAL**: `git stash` in the parent repo does NOT affect submodule working trees!

---

## 7. Known Traps

| Trap | Prevention |
|------|------------|
| Stale .asm files after compiler change | Always `make clean && make` |
| cproc uncommitted changes appear "clean" | Check `cd compiler/cproc && git status` explicitly |
| QBE submodule detached HEAD | `cd compiler/qbe && git checkout main` before committing |
| Multiple changes mask each other | ONE change at a time, always |
| "It was working before" without a commit | Commit working states immediately |
| Bank $00 vs $7E for C-accessible RAM | All C buffers must be in bank $00, addr < $2000 |

---

## 8. Quick Checklist (Copy-Paste for Each Session)

```
[ ] Baseline: make clean && make -k passes
[ ] Baseline: compiler tests pass
[ ] Baseline: validate_examples.sh passes
[ ] TEST FIRST: regression test written (Section 2)
[ ] TEST FIRST: test fails before the change (proves detection)
[ ] Change: ONE atomic change made
[ ] Build: make clean && make -k passes
[ ] Tests: new test passes
[ ] Tests: compiler tests pass (no regressions)
[ ] Tests: validate_examples.sh passes (no regressions)
[ ] Manual: 7 reference examples tested in emulator
[ ] Commit: test + change together, with test results in message
[ ] Verify: post-commit rebuild passes
```

---

*Created: February 2026 — After the oambuffer regression that broke 2 weeks of development.*
*Root cause: commit db8e977 moved oambuffer from bank $00 mirror ($7E:0520) to $7E:2000,
making it inaccessible to C code (compiler generates sta.l $0000,x = always bank $00).*
