#!/usr/bin/env python3
"""Golden-output tests for tmx2snes.

Each case copies the fixture map into a temp dir, runs tmx2snes, and
byte-compares every produced file against the committed golden. The
output is deterministic (no timestamps), so any diff is a real behaviour
change: either a regression, or an intentional change that must be
re-goldened after reviewing the diff.

What the cases pin:

  - the `-Q` quadrant tilemap. A SNES 64x64 background is four 32x32
    pages; the golden is byte-identical to the hand-written Python
    converter `examples/games/rpg/gen_assets.py` used before this flag
    existed, which is what makes it a meaningful reference and not just
    "whatever the code does today".
  - the `-e` entity header. Object types become macro prefixes, custom
    properties become tables, and a lone object of its type also gets
    scalar forms.
  - that a pretty-printed .tmj parses at all (issue #125): the fixture is
    indented, which cute_tiled cannot read without the minify pass.

Fixture provenance: fixtures/town.tmj and fixtures/tileset.map are the
RPG template's own map (`examples/games/rpg/res/`), original work.

Run:  python3 tools/tmx2snes/tests/run_golden.py
"""
from __future__ import annotations

import filecmp
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "tmx2snes"

# (map fixture, extra flags, expected outputs — all byte-compared)
CASES = [
    ("town.tmj", ["-e", "-Q"],
     ["BG1.q16", "town.inc", "BG1.m16", "town.b16", "town.o16"]),
    # no flags: the historical outputs must be untouched by the additions
    ("town.tmj", [], ["BG1.m16", "town.b16", "town.o16"]),
]


def run_case(fixture: str, flags: list[str], outputs: list[str]) -> list[str]:
    errs: list[str] = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / fixture, work / fixture)
        shutil.copy(HERE / "fixtures" / "tileset.map", work / "tileset.map")
        proc = subprocess.run([str(TOOL), *flags, fixture, "tileset.map"],
                              cwd=work, capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            return [f"exit {proc.returncode}: {(proc.stderr or proc.stdout).strip()[:200]}"]
        for out in outputs:
            got = work / out
            want = HERE / "golden" / out
            if not got.is_file():
                errs.append(f"{out}: not produced")
            elif not filecmp.cmp(got, want, shallow=False):
                errs.append(f"{out}: differs from golden ({got.stat().st_size} vs "
                            f"{want.stat().st_size} bytes)")
    return errs


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — run `make tools` first")
    fails = 0
    for fixture, flags, outputs in CASES:
        errs = run_case(fixture, flags, outputs)
        name = f"{fixture} [{' '.join(flags) or 'no flags'}]"
        if errs:
            print(f"  FAIL {name}: " + "; ".join(errs))
            fails += 1
        else:
            print(f"  PASS {name} ({len(outputs)} outputs match)")
    total = len(CASES)
    print(f"\ntmx2snes golden: {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
