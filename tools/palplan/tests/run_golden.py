#!/usr/bin/env python3
"""Golden-output tests for palplan (project shared-palette planner).

Runs the built tool on a committed manifest of BGR555 .pal fixtures and
byte-compares the generated C header and combined CGRAM image against
committed goldens. palplan is fully deterministic (slots are assigned in
manifest order), so any diff is a real behaviour change: a regression, or
an intentional change that must be re-goldened by re-running the tool here.

A second case asserts the over-subscription hard-fail: nine distinct sprite
palettes must exit non-zero (only eight slots exist).

Fixture provenance: tests/fixtures/*.pal are synthesized BGR555 palettes
generated deterministically in this repo (see the git history of this file
and tools/palplan/README.md) — not third-party assets. The manifest wires
in an identical pair (boss==hero), a two-colour-apart pair (hero~enemy), a
transparent-index-0-only pair (hero~ghost) and a 4-colour palette (hud).

Run:  python3 tools/palplan/tests/run_golden.py
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "palplan"

FIXTURES = HERE / "fixtures"
GOLDEN = HERE / "golden"


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — run `make tools` first")

    errs = []
    with tempfile.TemporaryDirectory() as td:
        header = Path(td) / "project.h"
        combined = Path(td) / "project.pal"
        # Run from the fixtures dir so the manifest path embedded in the
        # header ("project.txt") is stable across machines.
        proc = subprocess.run(
            [str(TOOL), "-o", str(header), "-b", str(combined), "project.txt"],
            cwd=str(FIXTURES), capture_output=True, text=True, timeout=60,
        )
        if proc.returncode != 0:
            errs.append(f"project.txt: exit {proc.returncode}: "
                        f"{(proc.stderr or proc.stdout).strip()[:200]}")
        else:
            for name, out in (("project.h", header), ("project.pal", combined)):
                got = out.read_bytes()
                want = (GOLDEN / name).read_bytes()
                if got != want:
                    errs.append(f"{name}: {len(got)} bytes differ from golden "
                                f"({len(want)} bytes)")

        # Over-subscription must hard-fail (9 distinct sprite palettes, 8 slots).
        over = subprocess.run(
            [str(TOOL), "over.txt"],
            cwd=str(FIXTURES), capture_output=True, text=True, timeout=60,
        )
        if over.returncode == 0:
            errs.append("over.txt: expected non-zero exit (over-subscription) "
                        "but got 0")

    if errs:
        print("palplan golden tests FAILED:")
        for e in errs:
            print(f"  - {e}")
        return 1
    print("palplan golden tests OK (3 checks: header, combined pal, overflow)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
