---
name: test
description: Run the OpenSNES test suite on luna (corpus liveness, visual regression, manifests, WRAM oracle, coverage ratchet, compiler fixtures)
argument-hint: "[all|compiler|manifests|<example-path>]"
allowed-tools: Bash(*), Read
---

# /test - Run Tests

Every runtime check goes through **luna**, the project's only emulator
backend (`.claude/rules/testing.md`, `.claude/rules/luna_tooling.md`).
Install the pinned binary once with `scripts/install-luna.sh`.

## Usage
```
/test                        # the full suite: make tests
/test compiler               # compiler C→ASM checks: make test-compiler
/test manifests              # scripted-input probes: make test-manifests
/test <category>/<example>   # one example: liveness + fbhash
```

## Commands

```bash
# Everything CI runs (about 15 minutes)
make tests

# Compiler fixtures only
make test-compiler

# luna test manifests (input → WRAM asserts)
make test-manifests

# One example
python3 tools/luna-test/luna_runner.py --coverage --only $1
python3 tools/luna-test/luna_runner.py --compare  --only $1

# Class A changes: the A/B proof against ROMs built before the change
python3 tools/luna-test/diff_corpus.py --ref /tmp/examples_before
```

## Report
1. Pass/fail per pillar, with the failing example and luna's message.
2. For a visual DIFF: the frame, and whether `luna diff` explains it.
3. Never re-baseline (`--update`) before the cause of a change is known.
