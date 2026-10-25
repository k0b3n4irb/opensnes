#!/usr/bin/env python3
"""The argument checks of the public headers (OPENSNES_CHECK_ARGS, snes/types.h)
refuse what they must and nothing else.

They are clang's diagnose_if, so they act in the clang pre-pass every C file of
a build goes through (make/common.mk, CLANG_LINT_FLAGS). This runs that pass on
calls the hardware would take wrongly (a tile base that is not a multiple of
0x1000 words, a tilemap base not a multiple of 0x400) and on calls it takes as
they are, with the flags read from common.mk. Skipped where clang is absent, as
the pre-pass is.
"""
from __future__ import annotations

import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

REFUSED = [
    ("bgSetGfxPtr(1, 0x0400);", "bgSetGfxPtr: the tile address must be a multiple of 0x1000 words"),
    ("bgSetMapPtr(1, 0x0200, 0);", "bgSetMapPtr: the tilemap address must be a multiple of 0x400 words"),
    ("bgInitTileSet(0, 0, 0, 0, 0, 0, 0, 0x1800);", "bgInitTileSet: the tile address must be a multiple of 0x1000 words"),
    ("bgInitTileSetData(2, 0, 0, 0x0C00);", "bgInitTileSetData: the tile address must be a multiple of 0x1000 words"),
]
ACCEPTED = [
    "bgSetGfxPtr(0, 0x2000);",
    "bgSetGfxPtr(0, v);",                       # known at run time only: not checked
    "bgSetMapPtr(0, 0x0800, 0);",
    "bgInitTileSet(0, 0, 0, 0, 0, 0, 0, 0x4000);",
    "bgInitTileSetData(0xFF, 0, 0, 0x0400);",   # 0xFF: no pointer is set
]


def lint_flags() -> list[str]:
    text = (ROOT / "make" / "common.mk").read_text(encoding="utf-8")
    m = re.search(r"^CLANG_LINT_FLAGS\s*:=\s*((?:.*\\\n)*.*)$", text, re.M)
    if not m:
        sys.exit("make/common.mk: no CLANG_LINT_FLAGS")
    return m.group(1).replace("\\\n", " ").split()


def lint(body: str, flags: list[str], work: Path) -> subprocess.CompletedProcess:
    src = work / "t.c"
    src.write_text("#include <snes.h>\nvoid f(u16 v) {\n    (void)v;\n    " + body + "\n}\n", encoding="utf-8")
    return subprocess.run(["clang", *flags, "-I", str(ROOT / "lib" / "include"), str(src)],
                          capture_output=True, text=True)


def main() -> int:
    if not shutil.which("clang"):
        print("test_header_arg_checks: no clang, skipped (the build skips its pre-pass too)")
        return 0
    flags = lint_flags()
    bad = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        for call, needle in REFUSED:
            p = lint(call, flags, work)
            if p.returncode == 0 or needle not in p.stderr:
                bad.append(f"not refused as expected: {call}\n{p.stderr.strip()}")
        for call in ACCEPTED:
            p = lint(call, flags, work)
            if p.returncode != 0:
                bad.append(f"refused, should pass: {call}\n{p.stderr.strip()}")
    for b in bad:
        print("FAIL", b)
    print(f"header argument checks: {len(REFUSED) + len(ACCEPTED) - len(bad)}/{len(REFUSED) + len(ACCEPTED)} as expected")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
