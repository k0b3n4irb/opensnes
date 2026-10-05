#!/usr/bin/env python3
"""Find, in a project's C sources, the OpenSNES names that 1.0 removes and
the two calls that change meaning (docs/UPGRADING.md).

    python3 devtools/check_upgrade.py <folder-or-file>...   # or: make check-upgrade SRC=<folder>

The list of names comes from two places: the SDK headers themselves — every
function or variable declared with OPENSNES_DEPRECATED("...") and every
constant named in a `#pragma clang deprecated(NAME, "...")`, the names that
still compile with a warning — and devtools/removed_api.txt, the names
already gone from the headers (since 2026-10-05), which the compiler can only
report as unknown identifiers. Two calls are reported even
though they keep their name: hdmaEnable() / hdmaDisable(), which took a
mask until 0.48 and a channel number from 1.0, dmaTransfer(), whose source
is one far pointer from 1.0, and mode7SetScale() /
mode7Transform(), whose 1:1 value changed in 0.47. Exit 0 when nothing is
found, 1 otherwise; a line per hit: file:line, the name, what to use.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
HEADERS = HERE.parent / "lib" / "include" / "snes"

DEPRECATED_FN = re.compile(
    r'OPENSNES_DEPRECATED\("([^"]*)"\)\s*\n\s*[^\n;]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', re.S)
# Anchored to the line start: types.h's doc comment quotes the generic form
# `#pragma clang deprecated(NAME, "...")`, which is not a name.
DEPRECATED_MACRO = re.compile(r'^\s*#pragma clang deprecated\(([A-Z0-9_]+), "([^"]*)"\)', re.M)

# The calls that keep their name and change what they mean.
MEANING = {
    "hdmaEnable": "takes a channel number 0-7 since 1.0 (a bit mask until 0.48; "
                  "above 7 it is refused) — 1 << n becomes n, or hdmaEnableMask(1 << n)",
    "hdmaDisable": "same as hdmaEnable: n, or hdmaDisableMask(1 << n)",
    "dmaTransfer": "takes the source as one far pointer since 1.0 (bank + address until 0.48): "
                   "dmaTransfer(ch, mode, src, destReg, size)",
    "mode7SetScale": "0x0100 is 1:1 since 0.47 (it was 0x0200): check the value",
    "mode7Transform": "100 is 1:1 since 0.47: check the percentage",
}

SOURCE_SUFFIXES = {".c", ".h", ".asm", ".s", ".inc"}


REMOVED_LIST = HERE / "removed_api.txt"


def gone_names() -> dict[str, str]:
    """Names already removed from the headers: removed_api.txt."""
    names: dict[str, str] = {}
    if REMOVED_LIST.is_file():
        for line in REMOVED_LIST.read_text(encoding="utf-8").splitlines():
            if not line or line.startswith("#"):
                continue
            name, what, header = (line.split("\t") + ["", ""])[:3]
            names[name] = f"use {what} (removed from {header})"
    return names


def removed_names() -> dict[str, str]:
    names: dict[str, str] = {}
    for header in sorted(HEADERS.glob("*.h")):
        text = header.read_text(encoding="utf-8")
        for msg, name in DEPRECATED_FN.findall(text):
            if name != "OPENSNES_DEPRECATED":
                names[name] = msg
        for name, msg in DEPRECATED_MACRO.findall(text):
            names[name] = msg
    return names


def sources(paths: list[str]):
    for p in paths:
        path = Path(p)
        if path.is_file():
            yield path
        else:
            for f in sorted(path.rglob("*")):
                if f.suffix in SOURCE_SUFFIXES and f.is_file():
                    yield f


def main(argv: list[str]) -> int:
    quiet = "-q" in argv or "--quiet" in argv
    argv = [a for a in argv if a not in ("-q", "--quiet")]
    if not argv:
        print(__doc__.strip().splitlines()[0])
        print("usage: check_upgrade.py [-q] <folder-or-file>...   (-q: hits only, no summary —"
              " the form make/common.mk runs per source when clang is absent)")
        return 2
    names = removed_names()
    gone = gone_names()
    names.update(gone)
    word = re.compile(r"\b(" + "|".join(map(re.escape, list(names) + list(MEANING))) + r")\b")
    hits = 0
    for f in sources(argv):
        if HEADERS in f.resolve().parents:
            continue  # the SDK's own headers name them on purpose
        for n, line in enumerate(f.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            code = line.split("//")[0]
            for m in word.finditer(code):
                name = m.group(1)
                what = names.get(name) or MEANING[name]
                kind = ("removed" if name in gone else
                        "removed at 1.0" if name in names else "changes meaning")
                print(f"{f}:{n}: {name} — {kind}: {what}")
                hits += 1
    if not quiet:
        print(f"\ncheck-upgrade: {hits} hit(s) in {len(list(sources(argv)))} file(s); "
              f"{len(names) - len(gone)} deprecated names from the headers, "
              f"{len(gone)} removed ones from removed_api.txt")
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
