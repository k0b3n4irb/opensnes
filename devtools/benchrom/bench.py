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
    # const paths — the A9 instrument (reads through `const` pointers/tables;
    # per CALL of the loop body: animTick, or a 32-element walk/copy)
    ("r_c_anim_tick",  "animTick (const clip)"),
    ("r_c_anim_meta",  "animTickMeta (+const table)"),
    ("r_c_walk8",      "const u8 walk x32", 8),
    ("r_c_walk16",     "const u16 walk x32", 8),
    ("r_c_copy",       "const->RAM copy x32", 8),
    ("r_c_fields",     "const struct fields"),
    ("r_c_index",      "const tab[i] (fused)"),
]


_cache: dict[str, int] = {}


def peek16(luna, sym):
    """All results in ONE luna run (one --peek per symbol): a run is 200 M
    steps, so one per symbol made the table take a quarter of an hour."""
    if not _cache:
        import json
        import subprocess
        syms = ["r_bench_done", "r_cal_empty", "r_c_val"] + [r[0] for r in RESULTS]
        cmd = [luna, "state", "-n", str(STEPS), "--out", "-"]
        for s in syms:
            cmd += ["--peek", f"{s}:2"]
        cmd.append(str(ROM))
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=1200).stdout
        for p in json.loads(out)["peeks"]:
            b = bytes.fromhex(p["bytes_hex"])
            _cache[p["spec"].split(":")[0]] = b[0] | (b[1] << 8)
    return _cache[sym]


def main() -> int:
    luna = find_luna()
    if not ROM.is_file():
        print(f"build first: make -C {HERE}")
        return 1
    done = peek16(luna, "r_bench_done")
    if done != 0xBEEF:
        print(f"FAIL: fixture incomplete (r_bench_done={done:#x}) — raise STEPS?")
        return 1
    val = peek16(luna, "r_c_val")
    if val != 528:
        print(f"FAIL: const-path correctness (r_c_val={val}, expected 528)")
        return 1
    cal = peek16(luna, "r_cal_empty")
    cal_cyc = cal * CYCLES_PER_FRAME / N_ITER
    print(f"calibration: {cal} frames ({cal_cyc:.1f} cyc/iter loop overhead)\n")
    print(f"{'function':<28} {'frames':>6} {'~cycles/call':>12}")
    for row in RESULTS:
        sym, name = row[0], row[1]
        div = row[2] if len(row) > 2 else 1      # loops run N_ITER/div times
        f = peek16(luna, sym)
        cyc = (f - cal / div) * CYCLES_PER_FRAME / (N_ITER / div)
        print(f"{name:<28} {f:>6} {cyc:>12.0f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
