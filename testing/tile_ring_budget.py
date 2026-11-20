#!/usr/bin/env python3
"""The tilering module's frames against the vertical blank, NTSC and PAL.

scrolling/tile_ring flies one lap of a fixed course (straight legs at 3 and
7 pixels a frame, diagonals at 5 + 5) over a map of 1,433 distinct
characters. Each frame, tileRingUpdate() queues the characters and the
tilemap the move needs, and vramQueueFlush() sends them right after the NMI
handler. Both run in the vertical blank, so their worst frames added must
fit in it:

    NTSC  lines 225-261:  37 x 1364 =  50,468 master cycles
    PAL   lines 225-311:  87 x 1364 = 118,668 master cycles

`luna profile --budget SYMBOL=MCLK` gives each symbol's worst completed
frame; the `.sym` is handed to luna without the routines' child labels
(`vramQueueFlush@entry`, `NmiHandler@…`), as testing/nmi_budget.py does, so
that each symbol is measured whole. The same run gates the frame itself:
no lag frame (`--max-lag-frames 0`), the module's work outside the blank
included. The two ways of sending the characters are measured: staging (the
example's default: copied to WRAM, one or two transfers) and direct (one
transfer per run of characters, from ROM; tr_direct poked before main).

This script asks luna and compares; it computes nothing luna does not give.

Run:  python3 testing/tile_ring_budget.py            # gate (in `make tests`)
      python3 testing/tile_ring_budget.py --report   # the figures only
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from lib import REPO_ROOT, find_luna  # noqa: E402

ROM = REPO_ROOT / "examples" / "scrolling" / "tile_ring" / "tile_ring.sfc"
LINE_MCLK = 1364
BLANK = {"ntsc": 37 * LINE_MCLK, "pal": 87 * LINE_MCLK}
FROM, UNTIL = 30, 1082          # one lap: the course is 1,052 frames
MODES = {"staging": [], "direct": ["--poke-at", "main", "--poke", "tr_direct=0100"]}
BUDGET_RE = re.compile(r"budget: (\S+) max (\d+) mclk \(frame (\d+)\)")
FRAMES_RE = re.compile(r"active mean (\d+) mclk of (\d+) a frame, max (\d+)")


def folded_sym(out: Path) -> Path:
    src = ROM.with_suffix(".sym")
    dst = out / src.name
    dst.write_text("".join(l for l in src.read_text(encoding="utf-8").splitlines(keepends=True)
                           if "NmiHandler@" not in l and "vramQueueFlush@" not in l))
    return dst


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--report", action="store_true", help="print the figures without gating")
    args = ap.parse_args()
    if not ROM.is_file():
        sys.exit(f"tile-ring-budget: {ROM.relative_to(REPO_ROOT)} missing — build the examples first")
    luna = find_luna()
    bad = 0
    with tempfile.TemporaryDirectory() as td:
        sym = folded_sym(Path(td))
        for region, blank in BLANK.items():
            for mode, extra in MODES.items():
                cmd = [luna, "profile", str(ROM), "--sym", str(sym), "--no-render", "--top", "0",
                       "--from-frame", str(FROM), "--until-frame", str(UNTIL), "--force-region", region,
                       "--budget", f"vramQueueFlush={10**9}", "--budget", f"NmiHandler={10**9}",
                       *([] if args.report else ["--max-lag-frames", "0"]), *extra]
                proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
                worst = {m.group(1): int(m.group(2)) for m in BUDGET_RE.finditer(proc.stdout)}
                frames = FRAMES_RE.search(proc.stdout)
                if len(worst) != 2 or not frames:
                    print(f"  FAIL  {region} {mode}: luna said {(proc.stdout + proc.stderr).strip()[-300:]}")
                    bad += 1
                    continue
                flush, nmi = worst["vramQueueFlush"], worst["NmiHandler"]
                total = flush + nmi
                over = total > blank or proc.returncode != 0
                verdict = "OVER" if over else "ok"
                if over and not args.report:
                    bad += 1
                mean, frame, peak = (int(g) for g in frames.groups())
                print(f"  {verdict:4}  {region:4} {mode:7}  flush {flush:6} + NMI {nmi:5} = {total:6} mclk"
                      f" of {blank:6} ({100.0 * total / blank:4.1f} % of the blank);"
                      f" frame: mean {mean:6}, worst {peak:6} of {frame} mclk"
                      + ("" if proc.returncode == 0 else f"  [luna exit {proc.returncode}: a lag frame]"))
    print(f"\ntile ring VBlank budget: {4 - bad}/4 within the blank, no lag frame"
          + (f", {bad} OVER" if bad else ""))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
