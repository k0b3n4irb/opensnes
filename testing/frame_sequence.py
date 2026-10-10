#!/usr/bin/env python3
"""Same pictures, another timing? — TRANSITORY PROTOTYPE (luna_tooling.md).

`luna diff A B --frames F --tolerance N` answers "does B show at F±N what A
shows at F". Two cases of 2026-10-08 fell outside it: a boot that got 20
frames longer than the tolerance we had asked for, and a ROM whose
free-running loop went from one picture every two frames to almost one per
frame (no single offset matches). Both were benign, and saying so took this
script: it hashes every frame of a range for both ROMs (`luna run
--until-frame F --print-fbhash`), collapses each run of equal hashes into
one picture, and reports

  * how many distinct pictures each ROM shows and for how many frames each,
  * the longest run of pictures the two ROMs show IN THE SAME ORDER.

A long common run with different counts = same animation, another cadence
or offset. A common run of 0 or 1 = different pictures (the negative
control: two unrelated ROMs).

This is the specification of a capability asked of luna (report of
2026-10-08, `luna diff --sequence`); it is deleted when luna ships it.

    python3 testing/frame_sequence.py A.sfc B.sfc --first 1 --last 200
"""
from __future__ import annotations

import argparse
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lib import find_luna  # noqa: E402
import luna_runner  # noqa: E402


def pictures(hashes: list[str]) -> list[tuple[str, int, int]]:
    """Runs of equal hashes as (hash, first index, length)."""
    out: list[tuple[str, int, int]] = []
    for n, h in enumerate(hashes):
        if out and out[-1][0] == h:
            out[-1] = (h, out[-1][1], out[-1][2] + 1)
        else:
            out.append((h, n, 1))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("a", type=Path)
    ap.add_argument("b", type=Path)
    ap.add_argument("--first", type=int, default=1)
    ap.add_argument("--last", type=int, default=200)
    args = ap.parse_args()
    luna = find_luna()
    frames = range(args.first, args.last + 1)
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor() as ex:
        grab = lambda rom, tag: list(ex.map(
            lambda f: luna_runner.render(luna, rom, f, Path(tmp) / f"{tag}{f}.png")[0], frames))
        ha, hb = grab(args.a, "a"), grab(args.b, "b")
    pa, pb = pictures(ha), pictures(hb)
    sa, sb = [p[0] for p in pa], [p[0] for p in pb]
    best, at = 0, (0, 0)
    for i in range(len(sa)):
        for j in range(len(sb)):
            k = 0
            while i + k < len(sa) and j + k < len(sb) and sa[i + k] == sb[j + k]:
                k += 1
            if k > best:
                best, at = k, (i, j)
    length = lambda p: sorted({n for _, _, n in p[1:-1]})
    print(f"frames {args.first}-{args.last}")
    print(f"A: {len(pa)} pictures, frames per picture {length(pa)}")
    print(f"B: {len(pb)} pictures, frames per picture {length(pb)}")
    if best:
        fa, fb = pa[at[0]][1] + args.first, pb[at[1]][1] + args.first
        print(f"longest common run: {best} pictures in the same order "
              f"(from frame {fa} in A, frame {fb} in B, offset {fb - fa:+d})")
    else:
        print("longest common run: 0 pictures")
    return 0


if __name__ == "__main__":
    sys.exit(main())
