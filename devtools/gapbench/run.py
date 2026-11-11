#!/usr/bin/env python3
"""run.py — what C costs against assembly written by hand, routine by routine, on luna.

Builds the bench ROM (kernels.c, kernels.asm) under build/gapbench/, SlowROM
and FastROM, and for each row of the driver's table reads from luna the master
cycles between bench_mark_a and bench_mark_b (16 calls, interrupts off), minus
the same for an empty routine. A row whose C and assembly checksums differ is
refused: the two must have done the same work.

    python3 devtools/gapbench/run.py              # the table
    python3 devtools/gapbench/run.py --check      # fail if a C figure is above baseline.json
    python3 devtools/gapbench/run.py --update     # rewrite baseline.json
    python3 devtools/gapbench/run.py --markdown   # the table as the README has it
"""
import argparse
import concurrent.futures
import json
import math
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

WORK = REPO / "build" / "gapbench"
BASELINE = HERE / "baseline.json"
SOURCES = ("Makefile", "kernels.c", "kernels.asm")
SPEEDS = {"slow": 0, "fast": 1}
# A routine moved in the ROM meets the memory refresh at other instructions:
# a few master cycles in tens of thousands. Anything beyond is a real change.
TOLERANCE = 0.002
# The twelve rows the bench started with (2026-11-11): their plain mean is
# kept beside the weighted one so that the first figure quoted stays readable.
FIRST_ROWS = ("copy", "fill", "oam", "col", "row", "box", "fixed", "mul", "ents", "call", "soa", "glide")
# What a row weighs in the weighted mean: the share of two real games' busy
# time that the kind of work takes in their luna profiles (object logic 40 to
# 50 %, sprites 16 to 22 %, boxes and probes 6 to 10 %, tilemaps a few %,
# multiply under 1 %, block copies 0.2 %; calls spread over all of it, 6.6 %
# whole). README.md says how they were read. A row not listed weighs 1.
WEIGHTS = {
    "soa": 8, "ents": 6, "state": 8, "fptr": 4, "slots": 4, "chain": 10,   # object logic
    "oam": 6, "glide": 6, "sprites": 8,                                      # sprites
    "box": 6, "probe": 4,                                                    # collisions
    "col": 2, "row": 2, "blocks": 2,                                         # tilemaps
    "call": 6,                                                               # calls
    "mul": 1, "fixed": 1,                                                    # arithmetic
    "copy": 1, "fill": 1, "unpack": 1,                                       # memory
}


def table():
    """The driver's rows, in order: (routine symbol, kernel name, variant)."""
    asm = (HERE / "kernels.asm").read_text()
    calls = int(re.search(r"^\.DEFINE CALLS (\d+)", asm, re.M).group(1))
    rows = []
    for m in re.finditer(r"^\s+\.dl\s+\w+,\s+(\w+),\s+\w+", asm, re.M):
        sym = m.group(1)
        if sym == "k_nop":
            rows.append((sym, "nop", "nop"))
        else:
            kernel, variant = re.fullmatch(r"k_(\w+)_(c|asm|lib)", sym).groups()
            rows.append((sym, kernel, variant))
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
        sys.exit(f"gapbench ({speed}): build failed\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return d / "gapbench.sfc"


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


def sizes(rom):
    sym = rom.with_suffix(".sym").read_text()
    return {m.group(2): int(m.group(1), 16)
            for m in re.finditer(r"^([0-9a-fA-F]{8}) _sizeof_(k_\w+)$", sym, re.M)}


def measure(luna, speed):
    calls, rows = table()
    rom = build(speed)
    assert len(rows) <= 64, "bench_sum in kernels.c holds 64 rows"
    jobs = [(mark, k + 1) for k in range(len(rows)) for mark in ("bench_mark_a", "bench_mark_b")]
    with concurrent.futures.ThreadPoolExecutor(int(os.environ.get("LUNA_JOBS", os.cpu_count() or 2))) as ex:
        totals = dict(zip(jobs, ex.map(lambda j: mclk_at(luna, rom, *j), jobs)))
    seg = [totals[("bench_mark_b", k + 1)] - totals[("bench_mark_a", k + 1)] for k in range(len(rows))]
    sums = checksums(luna, rom, len(rows))
    size = sizes(rom)
    assert rows[0][1] == "nop"
    res = {}
    for (sym, kernel, variant), s, c in zip(rows[1:], seg[1:], sums[1:]):
        r = res.setdefault(kernel, {})
        r[variant] = round((s - seg[0]) / calls)
        r[variant + "_sum"] = c
        if variant != "lib":
            r[variant + "_bytes"] = size.get(sym) or size.get(re.sub(r"[ui]_asm$", "_asm", sym))
    for kernel, r in res.items():
        bad = [v for v in ("asm", "lib") if v in r and r[v + "_sum"] != r["c_sum"]]
        if bad or r["c_sum"] == 0:
            sys.exit(f"gapbench ({speed}): {kernel}: the checksums differ "
                     f"({ {v: hex(r[v + '_sum']) for v in ('c', 'asm', 'lib') if v in r} }): "
                     "the twins did not do the same work, no figure")
        r["ratio"] = round(r["c"] / r["asm"], 2)
    return res


def geomean(xs):
    return round(math.exp(sum(map(math.log, xs)) / len(xs)), 2)


def other_source(kernel, res):
    """fillu, coli, rowi, entsi: the same routine written another way in C,
    against the same assembly. Shown, not counted in the mean."""
    return kernel[-1] in "ui" and kernel[:-1] in res


def summary(res):
    rows = {k: r["ratio"] for k, r in res.items() if not other_source(k, res)}
    ratios = list(rows.values())
    total = sum(WEIGHTS.get(k, 1) for k in rows)
    weighted = math.exp(sum(WEIGHTS.get(k, 1) * math.log(v) for k, v in rows.items()) / total)
    first = [rows[k] for k in FIRST_ROWS if k in rows]
    return {"geomean": geomean(ratios), "weighted": round(weighted, 2), "first": geomean(first),
            "min": min(ratios), "max": max(ratios)}


def print_table(results, markdown):
    slow, fast = results["slow"], results["fast"]
    head = ("routine", "C", "asm", "C/asm", "C fast", "asm fast", "C/asm fast", "C bytes", "asm bytes")
    lines = []
    for k in slow:
        s, f = slow[k], fast[k]
        lines.append((k + (" *" if other_source(k, slow) else ""), s["c"], s["asm"], f"{s['ratio']:.2f}", f["c"], f["asm"], f"{f['ratio']:.2f}",
                      s["c_bytes"], s["asm_bytes"]))
    ss, sf = summary(slow), summary(fast)
    lines.append(("mean, weighted by real games' profiles", "", "", f"{ss['weighted']:.2f}", "", "", f"{sf['weighted']:.2f}", "", ""))
    lines.append(("geometric mean, every row equal", "", "", f"{ss['geomean']:.2f}", "", "", f"{sf['geomean']:.2f}", "", ""))
    lines.append(("geometric mean of the first twelve rows", "", "", f"{ss['first']:.2f}", "", "", f"{sf['first']:.2f}", "", ""))
    if markdown:
        print("| " + " | ".join(head) + " |")
        print("|---|" + "--:|" * (len(head) - 1))
        for row in lines:
            print("| " + " | ".join(str(c) for c in row) + " |")
    else:
        print(*(h.rjust(11) if i else h.ljust(40) for i, h in enumerate(head)))
        for row in lines:
            print(*(str(c).rjust(11) if i else str(c).ljust(40) for i, c in enumerate(row)))
    for k in slow:
        if "lib" in slow[k]:
            print(f"\n{k}: the library's routine, {slow[k]['lib']} master cycles a call SlowROM, "
                  f"{fast[k]['lib']} FastROM")
    print("\n* the same routine written another way in C, against the same assembly; not in the mean")
    print(f"master cycles a call; C/asm from {ss['min']:.2f} to {ss['max']:.2f} SlowROM, "
          f"{sf['min']:.2f} to {sf['max']:.2f} FastROM")


def check(results):
    base = json.loads(BASELINE.read_text())
    bad = []
    for speed, res in results.items():
        for k, r in res.items():
            b = base[speed].get(k)
            if b is None:
                bad.append(f"{speed}/{k}: not in baseline.json (--update)")
                continue
            for v in ("c", "asm"):
                if r[v] > b[v] * (1 + TOLERANCE):
                    bad.append(f"{speed}/{k}: {v} {r[v]} master cycles, baseline {b[v]}")
                elif r[v] < b[v] * (1 - TOLERANCE):
                    print(f"note: {speed}/{k}: {v} {r[v]} is under the baseline {b[v]} (--update to keep the gain)")
    if bad:
        sys.exit("gapbench: slower than baseline.json\n  " + "\n  ".join(bad))
    print("gapbench: OK, nothing above baseline.json")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("--markdown", action="store_true")
    ap.add_argument("--json")
    args = ap.parse_args()
    luna = find_luna()
    results = {speed: measure(luna, speed) for speed in SPEEDS}
    print_table(results, args.markdown)
    if args.json:
        Path(args.json).write_text(json.dumps(results, indent=1) + "\n")
    if args.update:
        BASELINE.write_text(json.dumps(results, indent=1) + "\n")
        print(f"wrote {BASELINE.relative_to(REPO)}")
    elif args.check:
        check(results)
