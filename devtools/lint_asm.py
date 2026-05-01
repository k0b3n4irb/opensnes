#!/usr/bin/env python3
"""lint_asm.py — enforce the .ACCU/.INDEX marker policy in hand-written ASM.

Implements the rule documented in `.claude/rules/`: every `rep #$nn` or
`sep #$nn` instruction in a hand-written `.asm` file must be followed
(immediately, modulo blank lines and comments) by the matching
`.ACCU 8|16` and/or `.INDEX 8|16` directive(s).

The bug this prevents is silent: WLA-DX's per-object-file mode tracking
loses precision at branch merges and at section boundaries, and the
assembler has shipped 2-byte `cpx #0` (8-bit immediate) where 3 bytes
were needed, producing a one-byte misalignment that cascades through
every following instruction. Explicit markers force WLA-DX to use the
declared mode regardless of its inferred state.

Bit semantics (operand of `rep`/`sep`):
  bit 5 (mask $20) → M (memory/A) — 16-bit when M=0, 8-bit when M=1
  bit 4 (mask $10) → X (index)    — 16-bit when X=0, 8-bit when X=1
  rep clears bits, sep sets bits.

Operand-to-marker mapping:
  rep #$20 → .ACCU 16
  rep #$10 → .INDEX 16
  rep #$30 → .ACCU 16 + .INDEX 16
  sep #$20 → .ACCU 8
  sep #$10 → .INDEX 8
  sep #$30 → .ACCU 8 + .INDEX 8

Usage:
    devtools/lint_asm.py [--fix] [PATH ...]

With no PATH given, lints every `.asm` under `lib/source/` and
`templates/`. With `--fix`, the missing markers are inserted on the
line immediately after the rep/sep (preserving indentation). The
generated `.c.asm` files are NOT scanned — those come from cproc/qbe
and the compiler emits the markers itself.

Exit codes:
    0 — every rep/sep is properly followed by markers (or --fix succeeded)
    1 — one or more violations
    2 — bad invocation
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# Default search roots when no explicit paths are given.
DEFAULT_ROOTS = ["lib/source", "templates"]

# Locate hand-written rep/sep. The operand is a hex constant; we capture
# its numeric value to know which markers to require / inject.
RE_REP_SEP = re.compile(
    r"^(?P<indent>\s*)(?P<op>rep|sep)\s+#\$(?P<hex>[0-9a-fA-F]+)\b",
    re.IGNORECASE,
)
RE_ACCU = re.compile(r"^\s*\.ACCU\s+(?P<size>8|16)\b", re.IGNORECASE)
RE_INDEX = re.compile(r"^\s*\.INDEX\s+(?P<size>8|16)\b", re.IGNORECASE)
RE_BLANK_OR_COMMENT = re.compile(r"^\s*$|^\s*;")


def required_markers(op: str, operand: int) -> tuple[str | None, str | None]:
    """Return the (.ACCU value, .INDEX value) the rep/sep mandates.

    `rep` clears P bits → 16-bit mode for the affected register.
    `sep` sets P bits → 8-bit mode.

    Bit 5 (mask 0x20) governs M (.ACCU); bit 4 (mask 0x10) governs X
    (.INDEX). Each is independent — `rep #$20` only touches A, leaves
    X/Y alone.
    """
    is_rep = op.lower() == "rep"
    val_when_set = "16" if is_rep else "8"

    accu = val_when_set if (operand & 0x20) else None
    index = val_when_set if (operand & 0x10) else None
    return accu, index


def find_next_code_line(lines: list[str], start: int) -> int:
    """Return the index of the next non-blank, non-comment line after
    `start`, or len(lines) if none exists."""
    j = start + 1
    while j < len(lines) and RE_BLANK_OR_COMMENT.match(lines[j]):
        j += 1
    return j


def has_required_markers(
    lines: list[str], rep_sep_line: int, want_accu: str | None, want_index: str | None
) -> bool:
    """Check whether the lines immediately following `rep_sep_line`
    declare the required `.ACCU` and/or `.INDEX` values. Markers may
    appear in either order, possibly with one blank/comment line in
    between, but must precede any other instruction."""
    j = find_next_code_line(lines, rep_sep_line)
    accu_ok = want_accu is None
    index_ok = want_index is None
    # Look at up to two consecutive directive lines.
    for _ in range(2):
        if j >= len(lines):
            break
        line = lines[j]
        m_accu = RE_ACCU.match(line)
        m_index = RE_INDEX.match(line)
        if m_accu and not accu_ok and m_accu.group("size") == want_accu:
            accu_ok = True
        elif m_index and not index_ok and m_index.group("size") == want_index:
            index_ok = True
        else:
            break
        j = find_next_code_line(lines, j)
    return accu_ok and index_ok


def lint_file(path: Path) -> list[tuple[int, str, str]]:
    """Return a list of (line_number, rep/sep_text, what_is_missing)
    for every violation found in `path`."""
    lines = path.read_text().splitlines()
    violations: list[tuple[int, str, str]] = []
    for i, line in enumerate(lines):
        m = RE_REP_SEP.match(line)
        if not m:
            continue
        try:
            operand = int(m.group("hex"), 16)
        except ValueError:
            continue
        want_accu, want_index = required_markers(m.group("op"), operand)
        if want_accu is None and want_index is None:
            continue  # operand affected neither M nor X
        if has_required_markers(lines, i, want_accu, want_index):
            continue
        missing = []
        if want_accu is not None:
            missing.append(f".ACCU {want_accu}")
        if want_index is not None:
            missing.append(f".INDEX {want_index}")
        violations.append((i + 1, line.strip(), " + ".join(missing)))
    return violations


def fix_file(path: Path) -> int:
    """Insert missing markers into `path` in-place. Returns the number
    of insertions made."""
    lines = path.read_text().splitlines(keepends=True)
    inserted = 0
    # Walk from the bottom so insertions don't shift indices we still
    # need to inspect.
    i = len(lines) - 1
    while i >= 0:
        m = RE_REP_SEP.match(lines[i])
        if not m:
            i -= 1
            continue
        try:
            operand = int(m.group("hex"), 16)
        except ValueError:
            i -= 1
            continue
        want_accu, want_index = required_markers(m.group("op"), operand)
        if want_accu is None and want_index is None:
            i -= 1
            continue
        # Build a stripped-newline view around line i for the check.
        stripped = [ln.rstrip("\n") for ln in lines]
        if has_required_markers(stripped, i, want_accu, want_index):
            i -= 1
            continue
        indent = m.group("indent")
        # Convention in the existing files is a tab indent for body
        # instructions. Match the rep/sep's own indent so the directive
        # stays grouped.
        new_lines = []
        if want_accu is not None:
            new_lines.append(f"{indent}.ACCU {want_accu}\n")
        if want_index is not None:
            new_lines.append(f"{indent}.INDEX {want_index}\n")
        for offset, nl in enumerate(new_lines):
            lines.insert(i + 1 + offset, nl)
            inserted += 1
        i -= 1
    if inserted:
        path.write_text("".join(lines))
    return inserted


def discover_files(args_paths: list[str], repo_root: Path) -> list[Path]:
    """Resolve the list of `.asm` files to lint."""
    if args_paths:
        out = []
        for p in args_paths:
            path = Path(p)
            if not path.is_absolute():
                path = repo_root / path
            if path.is_dir():
                out.extend(sorted(path.rglob("*.asm")))
            elif path.is_file():
                out.append(path)
            else:
                sys.stderr.write(f"warning: {path} not found, skipped\n")
        return out
    out = []
    for root in DEFAULT_ROOTS:
        for p in sorted((repo_root / root).rglob("*.asm")):
            out.append(p)
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument(
        "--fix",
        action="store_true",
        help="insert missing .ACCU/.INDEX markers in-place",
    )
    parser.add_argument(
        "paths",
        nargs="*",
        help="files or directories to lint (default: lib/source + templates)",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    files = discover_files(args.paths, repo_root)
    if not files:
        sys.stderr.write("error: no .asm files found\n")
        return 2

    if args.fix:
        total_inserted = 0
        files_changed = 0
        for f in files:
            n = fix_file(f)
            if n:
                files_changed += 1
                total_inserted += n
                print(f"{f.relative_to(repo_root)}: inserted {n} marker(s)")
        if total_inserted:
            print(f"\nFixed {total_inserted} marker(s) across {files_changed} file(s).")
        else:
            print("OK: nothing to fix")
        return 0

    failed = 0
    for f in files:
        violations = lint_file(f)
        if not violations:
            continue
        failed += len(violations)
        rel = f.relative_to(repo_root)
        for line_no, text, missing in violations:
            print(f"{rel}:{line_no}: missing {missing} after `{text}`")

    if failed:
        print(
            f"\n{failed} violation(s). Run `devtools/lint_asm.py --fix` "
            f"to insert the markers automatically."
        )
        return 1

    print(f"OK: {len(files)} file(s) checked, no rep/sep without marker")
    return 0


if __name__ == "__main__":
    sys.exit(main())
