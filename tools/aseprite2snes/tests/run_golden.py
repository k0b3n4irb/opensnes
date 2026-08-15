#!/usr/bin/env python3
"""Golden-output tests for aseprite2snes (Aseprite export -> anim.h clips).

Runs the built tool on a committed Aseprite JSON fixture and byte-compares the
generated C header against a committed golden. aseprite2snes is fully
deterministic (frames and tags are emitted in export order), so any diff is a
real behaviour change: a regression, or an intentional change that must be
re-goldened by re-running the tool here.

A second case asserts the range hard-fail: a tag whose frame range runs past
the frame count must exit non-zero.

Fixture provenance: tests/fixtures/*.json are hand-authored exports in the
exact shape Aseprite writes with `--data --list-tags` (json-array frames,
meta.frameTags with name/from/to/direction/repeat) — not third-party assets.
hero.json exercises a non-uniform-duration clip (walk), two uniform clips
(idle, hurt) and a one-shot pingpong (hurt, repeat=1).

Run:  python3 tools/aseprite2snes/tests/run_golden.py
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "aseprite2snes"

FIXTURES = HERE / "fixtures"
GOLDEN = HERE / "golden"


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — run `make tools` first")

    errs = []
    # Run from the fixtures dir so the "Source export:" path embedded in the
    # header ("hero.json") is stable across machines.
    proc = subprocess.run(
        [str(TOOL), "-p", "hero", "hero.json"],
        cwd=str(FIXTURES), capture_output=True, text=True, timeout=60,
    )
    if proc.returncode != 0:
        errs.append(f"hero.json: exit {proc.returncode}: "
                    f"{(proc.stderr or proc.stdout).strip()[:200]}")
    else:
        got = proc.stdout
        want = (GOLDEN / "hero_anim.h").read_text()
        if got != want:
            errs.append(f"hero_anim.h: {len(got)} bytes differ from golden "
                        f"({len(want)} bytes)")

    # An out-of-range tag range must hard-fail (2 frames, tag asks for 0..9).
    over = subprocess.run(
        [str(TOOL), "-p", "bad", "bad_range.json"],
        cwd=str(FIXTURES), capture_output=True, text=True, timeout=60,
    )
    if over.returncode == 0:
        errs.append("bad_range.json: expected non-zero exit (range overflow) "
                    "but got 0")

    if errs:
        print("aseprite2snes golden tests FAILED:")
        for e in errs:
            print(f"  - {e}")
        return 1
    print("aseprite2snes golden tests OK (2 checks: header, range overflow)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
