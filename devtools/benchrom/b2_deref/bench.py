#!/usr/bin/env python3
"""B2 far-deref cost — frames -> ~cycles per ACCESS (32 accesses per iteration).
Usage: make -C devtools/benchrom/b2_deref && python3 devtools/benchrom/b2_deref/bench.py
"""
from __future__ import annotations
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
sys.path.insert(0, str(REPO / "tools" / "luna-test" / "probes"))
from lib import find_luna  # noqa: E402
import lib as probelib  # noqa: E402
ROM = HERE / "b2_deref.sfc"
STEPS = 70_000_000
N_ITER = 2_000
INNER = 32
CYCLES_PER_FRAME = 59_561.0
RESULTS = [
    ("r_ld_ptr_near",   "byte load  ptr   near (lda.l $0000,x)"),
    ("r_ld_ptr_far",    "byte load  ptr   far  ([tcc__r9])"),
    ("r_ld_ptr16_near", "word load  ptr   near"),
    ("r_ld_ptr16_far",  "word load  ptr   far"),
    ("r_ld_idx_near",   "byte load  s[i]  near"),
    ("r_ld_idx_far",    "byte load  s[i]  far  (lda.l sym,x)"),
    ("r_st_ptr_near",   "byte store ptr   near"),
    ("r_st_idx_near",   "byte store s[i]  near"),
    ("r_ld_ptr_farram", "byte load  ptr   __far ([tcc__r9],y)"),
    ("r_ld_ptr16_farram", "word load  ptr   __far"),
    ("r_ld_idx_farram", "byte load  s[i]  __far (lda.l sym,x)"),
    ("r_st_ptr_farram", "byte store ptr   __far ([tcc__r9],y)"),
    ("r_st_idx_farram", "byte store s[i]  __far (sta.l sym,x)"),
]
import json, subprocess
_cache = {}
def peek16(luna, sym):
    if not _cache:
        syms = ["r_bench_done", "r_cal_empty"] + [r[0] for r in RESULTS]
        cmd = [luna, "state", "-n", str(STEPS), "--out", "-"]
        for s in syms:
            cmd += ["--peek", f"{s}:2"]
        cmd.append(str(ROM))
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=600).stdout
        for p in json.loads(out)["peeks"]:
            b = bytes.fromhex(p["bytes_hex"])
            _cache[p["spec"].split(":")[0]] = b[0] | (b[1] << 8)
    return _cache[sym]
def main() -> int:
    luna = find_luna()
    if peek16(luna, "r_bench_done") != 0xBEEF:
        print("FAIL: fixture incomplete — raise STEPS?"); return 1
    cal = peek16(luna, "r_cal_empty")
    print(f"calibration: {cal} frames\n{'access':<40} {'frames':>6} {'~cyc/access':>12}")
    for sym, name in RESULTS:
        f = peek16(luna, sym)
        cyc = (f - cal) * CYCLES_PER_FRAME / N_ITER / INNER
        print(f"{name:<40} {f:>6} {cyc:>12.1f}")
    return 0
if __name__ == "__main__":
    sys.exit(main())
