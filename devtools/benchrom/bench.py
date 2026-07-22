#!/usr/bin/env python3
"""benchrom runner — frames -> ~cycles/call table for the C1 audit.

Runs benchrom.sfc in luna, reads the per-loop frame counts by symbol,
subtracts the empty-loop calibration and converts to approximate CPU
cycles per call (NTSC: ~59 561 CPU cycles per frame at 3.58 MHz —
approximate on purpose; the audit's ±10 % rule compares two numbers
from this same harness, so only the RATIO matters).

Usage: python3 devtools/benchrom/bench.py
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools" / "luna-test" / "probes"))
from lib import find_luna  # noqa: E402
import lib as probelib  # noqa: E402

ROM = HERE / "benchrom.sfc"
STEPS = 200_000_000          # generous: past every loop (r_bench_done gates)
N_ITER = 20_000             # must match main.c
CYCLES_PER_FRAME = 59_561.0

RESULTS = [
    ("r_m7_setangle",  "mode7SetAngle"),
    ("r_m7_setscale",  "mode7SetScale"),
    ("r_m7_setcenter", "mode7SetCenter"),
    ("r_m7_setmatrix", "mode7SetMatrix"),
    ("r_m7_transform", "mode7Transform"),
    ("r_map_getmeta",  "mapGetMetaTile"),
    ("r_map_getprop",  "mapGetMetaTilesProp"),
    ("r_map_camera",   "mapUpdateCamera"),
    ("r_map_update",   "camera+mapUpdate"),
    ("r_map_vblankf",  "camera+update+mapVblank"),
    ("r_map_getmeta_c", "C far-model getmetatile"),
    ("r_sd_draw",      "draw+EndFrame (steady)"),
    ("r_sd_draw_rf",   "refresh+draw+EndFrame+flush"),
    ("r_sd_flush_idle", "NmiFlush (idle floor)"),
    ("r_sd_draw_c",    "C-model draw+EndFrame"),
]


def peek16(luna, sym):
    b = probelib.peek(luna, ROM, STEPS, sym, 2)
    return b[0] | (b[1] << 8)


def main() -> int:
    luna = find_luna()
    if not ROM.is_file():
        print(f"build first: make -C {HERE}")
        return 1
    done = peek16(luna, "r_bench_done")
    if done != 0xBEEF:
        print(f"FAIL: fixture incomplete (r_bench_done={done:#x}) — raise STEPS?")
        return 1
    cal = peek16(luna, "r_cal_empty")
    cal_cyc = cal * CYCLES_PER_FRAME / N_ITER
    print(f"calibration: {cal} frames ({cal_cyc:.1f} cyc/iter loop overhead)\n")
    print(f"{'function':<18} {'frames':>6} {'~cycles/call':>12}")
    for sym, name in RESULTS:
        f = peek16(luna, sym)
        cyc = (f - cal) * CYCLES_PER_FRAME / N_ITER
        print(f"{name:<18} {f:>6} {cyc:>12.0f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
