# OpenSNES Professional Development Workflow

## Philosophy

> **"CI green means nothing. Local verification means everything."**

This workflow exists because bugs went undetected for months due to CI silently masking failures. Every rule here prevents a real bug that caused real problems.

---

## Phase 0: Pre-Flight Checks (Before ANY Work)

**MANDATORY before starting any development session:**

```bash
# 1. Ensure clean state
git status  # Must show clean working tree or known changes

# 2. Pull latest changes
git pull origin develop

# 3. Verify toolchain works
make clean && make

# 4. Run baseline tests (MUST ALL PASS)
./tests/compiler/run_tests.sh
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

**If ANY command fails:** STOP. Fix the issue before proceeding.

---

## Phase 1: Development

### 1.1 Understand Before Changing

**Before modifying ANY file:**

| File Type | Required Reading |
|-----------|------------------|
| Assembly (`.asm`) | `.claude/65816_ASSEMBLY_GUIDE.md`, `.claude/SNES_HARDWARE_REFERENCE.md` |
| Library code | `.claude/SNES_ROSETTA_STONE.md` |
| Memory layout (`crt0.asm`, `hdr.asm`) | WRAM mirror documentation in `CLAUDE.md` |
| Compiler (`qbe/`, `cproc/`) | Section 24 in `.claude/KNOWLEDGE.md` |
| CI workflows | Section 23 in `.claude/KNOWLEDGE.md` |

### 1.2 Create Test First (If None Exists)

```bash
# Check if tests exist for the feature
ls tests/unit/           # Unit tests
ls tests/compiler/cases/ # Compiler tests

# If no test exists: CREATE ONE FIRST
# Tests go in:
#   - tests/unit/<feature>/     for library features
#   - tests/compiler/cases/     for compiler features
```

### 1.3 Development Rules

#### Shell Scripts / CI Workflows
```bash
# EVERY script MUST start with:
set -o pipefail

# NEVER write:
cmd | tee log      # Exit code is lost!

# ALWAYS write:
set -o pipefail
cmd | tee log      # Exit code preserved
```

#### Memory Layout Changes
```bash
# After ANY change to:
#   - templates/common/crt0.asm
#   - templates/common/hdr.asm
#   - Any RAMSECTION directive

# MUST run:
make clean && make lib && make examples
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick

# If "COLLISION" appears: DO NOT COMMIT
```

#### Compiler Changes
```bash
# After ANY change to compiler/qbe or compiler/cproc:

# 1. Create minimal test case
cat > /tmp/test.c << 'EOF'
// Your test code
EOF

# 2. Verify assembly output
./bin/cc65816 /tmp/test.c -o /tmp/test.asm
cat /tmp/test.asm  # Manually verify correctness

# 3. Run ALL compiler tests
./tests/compiler/run_tests.sh

# 4. Rebuild and test examples
make clean && make
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

---

## Phase 2: Testing Gate

### 2.1 Test Pyramid

```
                    ┌─────────────┐
                    │  Emulator   │  ← Black screen test (Mesen2)
                    │   Tests     │
                   ─┴─────────────┴─
                  ┌─────────────────┐
                  │   Integration   │  ← Example validation
                  │     Tests       │     (memory overlaps, sizes)
                 ─┴─────────────────┴─
                ┌─────────────────────┐
                │     Unit Tests      │  ← Compiler tests,
                │                     │     library unit tests
               ─┴─────────────────────┴─
```

### 2.2 Mandatory Test Commands

**ALL must pass before proceeding to commit:**

```bash
# Level 1: Unit Tests
./tests/compiler/run_tests.sh
# Expected: "Results: 16/16 passed" (or current count)

# Level 2: Integration Tests
make examples
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
# Expected: "ALL VALIDATIONS PASSED"

# Level 3: Emulator Tests (if available)
./tests/run_black_screen_tests.sh /path/to/Mesen
# Expected: All ROMs display content (no black screens)
```

### 2.3 Test Failure Protocol

**If ANY test fails:**

1. **DO NOT COMMIT**
2. Investigate the failure
3. Fix the issue
4. Re-run ALL tests from the beginning
5. Only proceed when ALL pass

---

## Phase 3: Commit Gate

### 3.1 Pre-Commit Checklist

```bash
# Run this EXACT sequence before EVERY commit:

echo "=== Pre-Commit Verification ==="

# 1. Clean build
echo "[1/5] Clean build..."
make clean && make || { echo "FAIL: Build failed"; exit 1; }

# 2. Unit tests
echo "[2/5] Compiler tests..."
./tests/compiler/run_tests.sh || { echo "FAIL: Compiler tests failed"; exit 1; }

# 3. Build examples
echo "[3/5] Building examples..."
make examples || { echo "FAIL: Examples failed to build"; exit 1; }

# 4. Validate examples
echo "[4/5] Validating examples..."
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick || { echo "FAIL: Validation failed"; exit 1; }

# 5. Check for uncommitted test files
echo "[5/5] Checking for forgotten files..."
git status

echo "=== All checks passed ==="
```

### 3.2 Commit Format

```bash
# Format: type(scope): description
# Types: feat, fix, chore, docs, refactor, test

# Example:
git commit -m "$(cat <<'EOF'
fix(compiler): resolve struct pointer initialization bug

Detailed description of what was fixed and why.

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

### 3.3 Submodule Commits

**For changes to `compiler/qbe` or `compiler/cproc`:**

```bash
# 1. Commit in submodule
cd compiler/qbe
git add .
git commit -m "fix(emit): description

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>"

# 2. Push submodule
export GH_TOKEN=$(grep GH_TOKEN ../../.env | cut -d'=' -f2)
git push https://${GH_TOKEN}@github.com/k0b3n4irb/qbe.git HEAD:main

# 3. Update reference in main repo
cd ../..
git add compiler/qbe
git commit -m "chore: update qbe submodule

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>"
```

---

## Phase 4: Push Gate

### 4.1 Pre-Push Verification

```bash
# Before pushing, verify one more time:
git log --oneline -5  # Review commits
git diff origin/develop..HEAD --stat  # Review changes
```

### 4.2 Push Command

```bash
export GH_TOKEN=$(grep GH_TOKEN .env | cut -d'=' -f2)
git push https://${GH_TOKEN}@github.com/k0b3n4irb/opensnes.git develop
```

---

## Phase 5: CI Verification (CRITICAL)

### 5.1 NEVER Trust Green

**After pushing, you MUST verify CI actually ran the tests:**

```bash
# Wait for CI to complete
export GH_TOKEN=$(grep GH_TOKEN .env | cut -d'=' -f2)
gh run list --repo k0b3n4irb/opensnes --limit 1

# Get run ID from output, then:
gh run view <RUN_ID> --repo k0b3n4irb/opensnes

# Check that these steps show as "success":
#   ✓ Build tests
#   ✓ Run compiler tests
#   ✓ Build examples
#   ✓ Validate examples
```

### 5.2 Download and Verify Logs

```bash
# Even if CI is green, download logs and verify:
gh run download <RUN_ID> --repo k0b3n4irb/opensnes -n "OpenSNES Logs (linux)"

# Search for actual results:
grep -E "(PASS|FAIL|Error|error:)" opensnes_release_linux.log

# Expected: Only "PASS" messages, no "FAIL" or "Error"
```

### 5.3 CI Failure Protocol

**If CI fails (or logs show hidden failures):**

1. Download ALL platform logs
2. Identify the failure
3. Fix locally
4. Re-run local verification (Phase 2 + 3)
5. Push fix
6. Repeat CI verification

---

## Emergency Procedures

### Reverting a Bad Push

```bash
# If a push breaks the build:
git revert HEAD
git push origin develop

# Then investigate and fix properly
```

### Investigating CI "Green" Failures

```bash
# If CI shows green but something is broken:

# 1. Download all logs
gh run download <RUN_ID> -n "OpenSNES Logs (linux)"
gh run download <RUN_ID> -n "OpenSNES Logs (windows)"
gh run download <RUN_ID> -n "OpenSNES Logs (darwin)"

# 2. Search for hidden errors
grep -r "Error\|FAIL\|error:\|failed" *.log

# 3. Check for missing pipefail
grep -L "pipefail" .github/workflows/*.yml
```

### Memory Collision Debug

```bash
# If validate_examples shows COLLISION:

# 1. Identify the collision
python3 tools/symmap/symmap.py --check-overlap examples/path/to/*.sym

# 2. Check which symbols collide
# Bank $00 symbols should NOT be in $0300-$1FFF range
# Bank $7E buffers should be at $2000+ (above mirror range)

# 3. If compiler-generated variables collide:
#    - Check templates/common/crt0.asm reserved sections
#    - Verify Bank $7E buffers are above $2000
```

---

## Quick Reference Card

```
┌────────────────────────────────────────────────────────────┐
│                  BEFORE EVERY COMMIT                       │
├────────────────────────────────────────────────────────────┤
│  make clean && make                              [BUILD]   │
│  ./tests/compiler/run_tests.sh                   [UNIT]    │
│  make examples                                   [BUILD]   │
│  OPENSNES_HOME=$(pwd) ./tests/examples/          [VALID]   │
│      validate_examples.sh --quick                          │
├────────────────────────────────────────────────────────────┤
│                   AFTER EVERY PUSH                         │
├────────────────────────────────────────────────────────────┤
│  gh run view <ID> --repo k0b3n4irb/opensnes      [STATUS]  │
│  Download logs and grep for errors               [VERIFY]  │
├────────────────────────────────────────────────────────────┤
│                     NEVER FORGET                           │
├────────────────────────────────────────────────────────────┤
│  • CI green ≠ tests passed                                 │
│  • Always use set -o pipefail in scripts                   │
│  • Bank $00 $0000-$1FFF mirrors Bank $7E                   │
│  • Copy strings from parser buffers, don't store pointers  │
└────────────────────────────────────────────────────────────┘
```

---

## Documentation References

| Document | When to Consult |
|----------|-----------------|
| `CLAUDE.md` | Critical rules, WRAM mirror layout |
| `.claude/KNOWLEDGE.md` | Bug history, lessons learned |
| `.claude/SNES_HARDWARE_REFERENCE.md` | PPU, DMA, timing |
| `.claude/65816_ASSEMBLY_GUIDE.md` | Assembly programming |
| `.claude/SNES_ROSETTA_STONE.md` | API reference, PVSnesLib comparison |

---

## Git Configuration

```bash
# Author for commits
git config user.name "K0b3"
git config user.email "K0b3@nowhere.zz"

# GitHub token for push
export GH_TOKEN=$(grep GH_TOKEN .env | cut -d'=' -f2)
```

## Submodule Rules

| Submodule | Can Modify? | Push To |
|-----------|-------------|---------|
| `compiler/qbe` | YES | k0b3n4irb/qbe |
| `compiler/cproc` | YES | k0b3n4irb/cproc |
| `compiler/wla-dx` | NO | Never (read-only) |

---

*Last updated: February 2026 - After discovering CI masking and memory corruption bugs*
