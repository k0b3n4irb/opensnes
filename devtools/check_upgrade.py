#!/usr/bin/env python3
"""Find, in a project's C sources, the OpenSNES names that 1.0 removes and
the two calls that change meaning (docs/UPGRADING.md).

    python3 devtools/check_upgrade.py <folder-or-file>...   # or: make check-upgrade SRC=<folder>

The list of removed names is read from the SDK headers themselves — every
function or variable declared with OPENSNES_DEPRECATED("...") and every
constant named in a `#pragma clang deprecated(NAME, "...")` — so it cannot
drift from what the compiler warns about. Two calls are reported even
though they keep their name: hdmaEnable() / hdmaDisable(), which take a
mask until 1.0 and a channel number from 1.0, and mode7SetScale() /
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
    "hdmaEnable": "takes a bit mask until 1.0 and a channel number from 1.0 — "
                  "use hdmaEnableMask() now, hdmaEnable(channel) at 1.0",
    "hdmaDisable": "same as hdmaEnable: hdmaDisableMask() now",
    "mode7SetScale": "0x0100 is 1:1 since 0.47 (it was 0x0200): check the value",
    "mode7Transform": "100 is 1:1 since 0.47: check the percentage",
}

SOURCE_SUFFIXES = {".c", ".h", ".asm", ".s", ".inc"}


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
    if not argv:
        print(__doc__.strip().splitlines()[0])
        print("usage: check_upgrade.py <folder-or-file>...")
        return 2
    names = removed_names()
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
                kind = "removed at 1.0" if name in names else "changes meaning"
                print(f"{f}:{n}: {name} — {kind}: {what}")
                hits += 1
    print(f"\ncheck-upgrade: {hits} hit(s) in {len(list(sources(argv)))} file(s); "
          f"{len(names)} removed names known from the headers")
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
