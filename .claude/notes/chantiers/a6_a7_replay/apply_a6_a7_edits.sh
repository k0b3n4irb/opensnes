#!/usr/bin/env bash
# Re-apply 2026-05-11's 9-site atomic patch (A6.1/4/6/9/10/11 + A7.3/4/5 +
# Kl-opt guards) to compiler/cproc and compiler/qbe submodules.
#
# Intended for use at the START of the next focused session. Saves ~30 min
# of mechanical re-typing. Captures EXACTLY the edits made on 2026-05-11
# session, including the post-mortem additions (A6.11 + Kl-opt guards).
#
# Usage (from repo root):
#   bash .claude/notes/chantiers/a6_a7_replay/apply_a6_a7_edits.sh
#
# Exits 0 on success, non-zero with a clear error message on any guard.

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

PATCH_DIR=".claude/notes/chantiers/a6_a7_replay"
QBE_PATCH="$PATCH_DIR/qbe.patch"
CPROC_PATCH="$PATCH_DIR/cproc.patch"
EXPECTED_BRANCH="wip/a6-a7-atomic-v3"

red()   { printf "\033[31m%s\033[0m\n" "$*" >&2; }
green() { printf "\033[32m%s\033[0m\n" "$*"; }
blue()  { printf "\033[34m%s\033[0m\n" "$*"; }

blue "=== A6+A7 atomic patch replay ==="

# Guard 1: branch
CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [ "$CURRENT_BRANCH" != "$EXPECTED_BRANCH" ]; then
    red "ERROR: must run on branch '$EXPECTED_BRANCH', currently on '$CURRENT_BRANCH'"
    red "Run: git checkout $EXPECTED_BRANCH"
    exit 1
fi

# Guard 2: submodule worktrees clean (only persistent wla-dx mod allowed)
QBE_DIRTY="$(cd compiler/qbe && git status -s | wc -l | tr -d ' ')"
CPROC_DIRTY="$(cd compiler/cproc && git status -s | wc -l | tr -d ' ')"
if [ "$QBE_DIRTY" != "0" ]; then
    red "ERROR: compiler/qbe has uncommitted changes:"
    (cd compiler/qbe && git status -s) >&2
    red "Reset with: (cd compiler/qbe && git stash)"
    exit 1
fi
if [ "$CPROC_DIRTY" != "0" ]; then
    red "ERROR: compiler/cproc has uncommitted changes"
    (cd compiler/cproc && git status -s) >&2
    exit 1
fi

# Guard 3: patches exist
[ -f "$QBE_PATCH" ]   || { red "ERROR: missing $QBE_PATCH"; exit 1; }
[ -f "$CPROC_PATCH" ] || { red "ERROR: missing $CPROC_PATCH"; exit 1; }

# Apply
blue "Applying cproc patch (A6.1)..."
(cd compiler/cproc && git apply "$REPO_ROOT/$CPROC_PATCH")
green "  cproc patch applied"

blue "Applying qbe patch (A6.4 + A6.6 + A6.9 + A6.10 + A6.11 + A7.3 + A7.4 + A7.5 + Kl-opt guards)..."
(cd compiler/qbe && git apply "$REPO_ROOT/$QBE_PATCH")
green "  qbe patch applied"

# Build
blue "Building cproc..."
make -C compiler/cproc 2>&1 | tail -3

blue "Building qbe..."
make -C compiler/qbe CC=clang 2>&1 | tail -3

# Install
mkdir -p bin
cp compiler/cproc/cproc-qbe bin/cproc-qbe
cp compiler/qbe/qbe bin/qbe
green "  binaries installed to bin/"

# Smoke build (1 example) to confirm the toolchain produces output
blue "Smoke build: text/hello_world..."
(cd examples/text/hello_world && make clean >/dev/null 2>&1 && make 2>&1 | tail -2)

green ""
green "=== Replay complete ==="
green "Next steps (Phase 1 — diagnostic, ~2-3h):"
green "  1. Open examples/text/hello_world/hello_world.sfc in Mesen2"
green "  2. Step through main() until first visible wrong behavior"
green "  3. Cross-reference with examples/text/hello_world/main.c.asm"
green "  4. Document mechanism in .claude/notes/chantiers/a6_a7_unified_audit.md"
green ""
green "Abort conditions (Phase 1):"
green "  - No clear mechanism after 3h → escalate, don't widen scope further"
green "  - Lib ASM rework needed → split into A6.12+ chantier, don't bundle"
green ""
green "Test command for full validation (after fixes):"
green "  make clean && make && cd tools/opensnes-emu && node test/run-all-tests.mjs --quick"
