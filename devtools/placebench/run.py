#!/usr/bin/env python3
"""run.py — oamPlaceWorld (the library's) against place_old, on luna.

place_old.asm is the library's body before LIB place prod, renamed. Builds
the bench ROM (bench.c, kernels.asm, place_old.asm) under build/placebench/,
SlowROM and FastROM, and for each row reads from luna the master cycles
between bench_mark_a and bench_mark_b (16 calls, interrupts off), minus the
same for the empty kernel (the same pushes and a call to an rtl): a figure is
the routine's body. A row whose old and new checksums differ gives no figure
(the whole OAM shadow, visible[], oam_max_id and oam_update_flag are in the
checksum).

    python3 devtools/placebench/run.py             # the table, the pass
    python3 devtools/placebench/run.py --json out  # and the figures

The pass: the `p` rows are the calls one consumer makes every frame (two
always, p18 and p20; up to five of the others); its one-sprite batch is
weighed 88 % placed, 12 % hidden, as that consumer measured.
"""
import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna  # noqa: E402

WORK = REPO / "build" / "placebench"
SOURCES = ("Makefile", "bench.c", "kernels.asm", "place_old.asm")
SPEEDS = {"slow": 0, "fast": 1}
FRAME = 357368      # master cycles of a 60 fps frame

# the consumer's pass: (row, weight); p18 and p20 are there every pass
ALWAYS = (("p18", 1), ("p20", 1))
OTHERS = (("pball_in", 0.88), ("pball_out", 0.12), ("p5", 1), ("p1", 1),
          ("p4a", 1), ("p7", 1), ("p4b", 1))


def table():
    asm = (HERE / "kernels.asm").read_text()
    calls = int(re.search(r"^\.DEFINE CALLS (\d+)", asm, re.M).group(1))
    rows = re.findall(r"^\s+\.dl\s+\w+,\s*\w+,\s*\w+\s*;\s*(\w+)\s+(\w+)", asm, re.M)
    return calls, rows


def build(speed):
    d = WORK / speed
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    for f in SOURCES:
        shutil.copy(HERE / f, d)
    r = subprocess.run(["make", f"OPENSNES={REPO}", f"USE_FASTROM={SPEEDS[speed]}"],
                       cwd=d, capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"placebench ({speed}): build failed\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return d / "placebench.sfc"


def mclk_at(luna, rom, mark, hit):
    out = rom.parent / f"{mark}.{hit}.json"
    subprocess.run([luna, "profile", str(rom), "--no-render", "--until-pc", mark,
                    "--hit", str(hit), "--out", str(out)], check=True, capture_output=True)
    total = json.loads(out.read_text())["total_mclk"]
    out.unlink()
    return total


def checksums(luna, rom, n):
    err = subprocess.run([luna, "state", str(rom), "--no-render", "--until-pc", "bench_done",
                          "--peek", f"bench_sum:#{n * 2}"],
                         capture_output=True, text=True, check=True).stderr
    data = []
    for line in err.split("peek ", 1)[1].splitlines()[1:]:
        m = re.match(r"\s+\$[0-9A-Fa-f]+\s+((?:[0-9A-Fa-f]{2}\s*)+)$", line)
        if m:
            data += [int(b, 16) for b in m.group(1).split()]
    assert len(data) == n * 2, len(data)
    return [data[i] | data[i + 1] << 8 for i in range(0, n * 2, 2)]


def measure(luna, speed):
    calls, rows = table()
    rom = build(speed)
    jobs = [(mark, k + 1) for k in range(len(rows)) for mark in ("bench_mark_a", "bench_mark_b")]
    with concurrent.futures.ThreadPoolExecutor(int(os.environ.get("LUNA_JOBS", os.cpu_count() or 2))) as ex:
        totals = dict(zip(jobs, ex.map(lambda j: mclk_at(luna, rom, *j), jobs)))
    seg = [totals[("bench_mark_b", k + 1)] - totals[("bench_mark_a", k + 1)] for k in range(len(rows))]
    sums = checksums(luna, rom, len(rows))
    assert rows[0] == ("empty", "empty")
    res = {}
    for (row, variant), s, c in zip(rows[1:], seg[1:], sums[1:]):
        r = res.setdefault(row, {})
        r[variant] = round((s - seg[0]) / calls)
        r[variant + "_sum"] = c
    for row, r in res.items():
        if "mix" in r:
            continue                # the mixed path alone: no old body
        if r["old_sum"] != r["new_sum"]:
            sys.exit(f"placebench ({speed}): {row}: the checksums differ "
                     f"(old {r['old_sum']:#06x}, new {r['new_sum']:#06x}): no figure")
        r["margin"] = round(100 * (1 - r["new"] / r["old"]), 1)
    return res


def per_sprite(res, shape, variant):
    def g(k):
        return res[f"{k}_{shape}"][variant]
    placed = round((g("vis32") - g("vis1")) / 31)
    hidden = round((g("hid32") - g("hid1")) / 31)
    return {"placed": placed, "hidden": hidden, "fixed": round(g("vis1") - placed),
            "one_placed": g("vis1"), "one_hidden": g("hid1")}


def the_pass(res, rows):
    old = sum(res[r]["old"] * w for r, w in rows)
    new = sum(res[r]["new"] * w for r, w in rows)
    return (round(old), round(new), round(old - new), round(100 * (old - new) / old, 1),
            round(100 * (old - new) / FRAME, 2))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--json")
    args = ap.parse_args()
    luna = find_luna()
    results = {speed: measure(luna, speed) for speed in SPEEDS}
    slow, fast = results["slow"], results["fast"]
    print(f"{'row':16} {'old':>8} {'new':>8} {'margin':>7}   {'old fast':>8} {'new fast':>9} {'margin':>7}")
    for row in slow:
        s, f = slow[row], fast[row]
        if "mix" in s:
            continue
        print(f"{row:16} {s['old']:8} {s['new']:8} {s['margin']:6.1f}%   {f['old']:8} {f['new']:9} {f['margin']:6.1f}%")
    print("\nper sprite (vis32 - vis1 and hid32 - hid1, / 31), fixed a call (vis1 - placed):")
    derived = {}
    for shape in ("plain", "order", "vis", "ordvis"):
        for speed in SPEEDS:
            for v in ("old", "new"):
                d = per_sprite(results[speed], shape, v)
                derived[f"{shape}/{speed}/{v}"] = d
                print(f"  {shape:6} {speed:4} {v:3} placed {d['placed']:5}  hidden {d['hidden']:5}"
                      f"  fixed {d['fixed']:5}  one placed {d['one_placed']:5}  one hidden {d['one_hidden']:5}")
    print("\nthe mixed path (`big`: size 16, size_big 32), the library alone:")
    print(f"{'row':16} {'slow':>8} {'fast':>8}   checksum")
    for row in slow:
        if "mix" in slow[row]:
            print(f"{row:16} {slow[row]['mix']:8} {fast[row]['mix']:8}   {slow[row]['mix_sum']:#06x}")
    for shape in ("plain", "order", "vis", "ordvis"):
        for speed in SPEEDS:
            r = results[speed]
            def g(k):
                return r[f"{k}_{shape}"]["mix"]
            d = {"placed_small": round((g("mvis32s") - g("mvis1s")) / 31),
                 "placed_large": round((g("mvis32b") - g("mvis1b")) / 31),
                 "hidden": round((g("mhid32") - g("mhid1")) / 31),
                 "one_small": g("mvis1s"), "one_large": g("mvis1b"), "one_hidden": g("mhid1")}
            derived[f"mixed/{shape}/{speed}"] = d
            print(f"  mixed {shape:6} {speed:4} placed small {d['placed_small']:5}  large {d['placed_large']:5}"
                  f"  hidden {d['hidden']:5}  one small {d['one_small']:5}  one large {d['one_large']:5}"
                  f"  one hidden {d['one_hidden']:5}")
    print("\nthe consumer's pass: old, new, saved, %, % of a frame")
    passes = {}
    for speed in SPEEDS:
        for name, rows in (("always", ALWAYS), ("all", ALWAYS + OTHERS)):
            passes[f"{name}/{speed}"] = p = the_pass(results[speed], rows)
            print(f"  {name:6} {speed:4} {p[0]:7} {p[1]:7} {p[2]:6} {p[3]:5.1f}% {p[4]:5.2f}%")
    if args.json:
        Path(args.json).write_text(json.dumps({"rows": results, "per_sprite": derived,
                                               "pass": passes}, indent=1) + "\n")


if __name__ == "__main__":
    main()
