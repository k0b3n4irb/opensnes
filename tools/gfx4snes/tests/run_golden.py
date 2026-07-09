#!/usr/bin/env python3
"""Golden-output tests for gfx4snes.

Each case copies a fixture PNG into a temp dir, runs gfx4snes with the
canonical flags used by the example Makefiles, and byte-compares every
produced file against the committed golden. gfx4snes output is
deterministic (no timestamps in generated files), so any diff is a real
behaviour change: either a regression, or an intentional change that
must be re-goldened (re-run the case and copy the outputs over golden/,
reviewing the diff).

Fixture provenance (both PVSnesLib-derived assets already in the repo,
see ATTRIBUTION.md):
  fixtures/bg.png  = examples/graphics/backgrounds/mode1/res/opensnes.png
  fixtures/spr.png = examples/games/likemario/res/mario_sprite.png

Run:  python3 tools/gfx4snes/tests/run_golden.py
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
TOOL = REPO / "bin" / "gfx4snes"

# (fixture, flags, expected outputs — all byte-compared against golden/)
CASES = [
    ("bg.png", ["-s", "8", "-o", "16", "-u", "16", "-p", "-m"],
     ["bg.pic", "bg.pal", "bg.map", "bg.inc", "bg_data.as"]),
    ("spr.png", ["-s", "16", "-p"],
     ["spr.pic", "spr.pal", "spr.inc", "spr_data.as"]),
    # Metasprite path (-T) with flip-aware dedup (-F): flip.png holds one
    # asymmetric 16x16 tile and its exact H-mirror, so the golden pins the
    # OBJ_FLIPX emission and the canonical-tile reference (issue #97).
    ("flip.png", ["-s", "16", "-o", "16", "-u", "16", "-p", "-T",
                  "-X", "32", "-Y", "16", "-P", "2", "-F"],
     ["flip.pic", "flip.pal", "flip.inc", "flip_data.as", "flip_meta.inc"]),
    # Metasprite tile field is an 8x8 CHARACTER NAME, not a block index
    # (issue #100). two.png = two distinct 16x16 blocks -> the second block's
    # tiles live at char name 2 (2 columns of 8x8), not index 1. Pins the
    # positional conversion in metasprites.c.
    ("two.png", ["-s", "16", "-o", "16", "-u", "16", "-p", "-T",
                 "-X", "32", "-Y", "16", "-P", "2"],
     ["two_meta.inc"]),
    # three.png = [A, H-mirror of A, distinct B]. The mirror makes the
    # compacted tile counter lag the reading position, so a naive index would
    # emit B at 2 instead of its true char name 4. Pins the position-based
    # canonical reference in maps.c (issue #100 multi-mirror drift).
    ("three.png", ["-s", "16", "-o", "16", "-u", "16", "-p", "-T",
                   "-X", "48", "-Y", "16", "-P", "2", "-F"],
     ["three_meta.inc"]),
]


def run_case(fixture: str, flags: list[str], outputs: list[str]) -> list[str]:
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / fixture, work / fixture)
        proc = subprocess.run([str(TOOL), *flags, "-i", fixture],
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
        name = f"{fixture} [{' '.join(flags)}]"
        if errs:
            print(f"  FAIL {name}: " + "; ".join(errs))
            fails += 1
        else:
            print(f"  PASS {name} ({len(outputs)} outputs match)")
    print(f"\ngfx4snes golden: {len(CASES) - fails}/{len(CASES)} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
