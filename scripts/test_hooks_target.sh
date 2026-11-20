#!/bin/sh
# make hooks: sets an empty core.hooksPath, leaves another value, FORCE=1 replaces it.
# Usage: test_hooks_target.sh SCRATCH_DIR (a new, empty directory; the caller removes it)
set -e
root=$(cd "$(dirname "$0")/.." && pwd)
t=${1:?scratch dir}
git init -q "$t"
cp "$root/Makefile" "$t/Makefile"
cd "$t"
make -s hooks >/dev/null
[ "$(git config core.hooksPath)" = scripts/githooks ] || { echo "FAIL empty"; exit 1; }
git config core.hooksPath other
make -s hooks | grep -q "left unchanged" || { echo "FAIL message"; exit 1; }
[ "$(git config core.hooksPath)" = other ] || { echo "FAIL kept"; exit 1; }
make -s hooks FORCE=1 >/dev/null
[ "$(git config core.hooksPath)" = scripts/githooks ] || { echo "FAIL force"; exit 1; }
echo "hooks target: OK"
