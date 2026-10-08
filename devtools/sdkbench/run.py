#!/usr/bin/env python3
"""run.py — the same C, built by OpenSNES and by PVSnesLib, timed on luna.

`workloads.c` holds twelve small workloads; this script builds one ROM per
workload and per SDK (plus a baseline that runs none), runs each on luna
for the same number of frames with `luna profile`, and reports the master
cycles each workload cost.

How a workload is timed. `luna profile` credits every master cycle to the
symbol being executed. A ROM that runs a workload and the baseline ROM of
the same SDK run for the same number of frames, so they differ only by the
workload: the symbols it executes gain cycles, the idle loop loses as many,
the NMI handler stays equal. The cost of the workload is the sum of the
gains. Everything is counted — the function, what it calls, the runtime's
multiply and divide — and nothing of the SDK's boot or per-frame handler.

The two ROMs must leave the same checksum in `res`: a workload on which the
SDKs disagree is reported as such and not compared.

    python3 devtools/sdkbench/run.py            # needs PVSNESLIB_HOME
    python3 devtools/sdkbench/run.py --json out.json
    python3 devtools/sdkbench/run.py --only sort,crc

Both ROMs are LoROM, SlowROM (each SDK's default). Contributor tool: it
needs a PVSnesLib tree built for this machine and is not part of any gate.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna  # noqa: E402

WORK = REPO / "build" / "sdkbench"
NAMES = ["baseline", "sieve", "sort", "physics", "collide", "mul", "decimal",
         "long", "bytes", "calls", "switch", "crc", "list"]
WHAT = {"sieve": "sieve of 1024, byte array", "sort": "insertion sort, 64 words",
        "physics": "32 entities x 60 steps", "collide": "496 box pairs x 8",
        "mul": "2304 variable multiplies", "decimal": "200 numbers to digits (/10, %10)",
        "long": "300 steps of 32-bit math", "bytes": "512-byte fill, copy, compare x 4",
        "calls": "recursive fib(17)", "switch": "1280 interpreted ops",
        "crc": "CRC-16 of 256 bytes, bitwise", "list": "linked list, 40 walks"}
FRAMES = 900
DONE = 0x600D

OPENSNES_MAKEFILE = """OPENSNES := {repo}
TARGET   := bench.sfc
ROM_NAME := SDKBENCH
USE_LIB  := 1
LIB_MODULES := console sprite dma background
SKIP_LINT := 1
CSRC := workloads.c
include $(OPENSNES)/make/common.mk
"""


def prepare(sdk: str, k: int, pvs: Path) -> Path:
    d = WORK / sdk / NAMES[k]
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    which = f"#define WORKLOAD {k}\n"
    if sdk == "opensnes":
        shutil.copy(HERE / "workloads.c", d / "workloads.c")
        (d / "which.h").write_text(which)
        (d / "Makefile").write_text(OPENSNES_MAKEFILE.format(repo=REPO))
    else:
        # PVSnesLib's own hello_world is the skeleton: its header, its font
        # data and its Makefile, with our source in src/.
        skel = pvs / "snes-examples" / "hello_world"
        for name in ("Makefile", "hdr.asm", "data.asm", "pvsneslibfont.png"):
            shutil.copy(skel / name, d / name)
        (d / "src").mkdir()
        shutil.copy(HERE / "workloads.c", d / "src" / "hello_world.c")
        (d / "src" / "which.h").write_text(which)
    return d


def build(sdk: str, k: int, pvs: Path):
    d = prepare(sdk, k, pvs)
    env = {key: v for key, v in os.environ.items() if key not in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL")}
    if sdk == "pvsneslib":
        env["PVSNESLIB_HOME"] = str(pvs)
    proc = subprocess.run(["make", "-s", "-C", str(d)], capture_output=True, text=True, env=env)
    rom = d / ("bench.sfc" if sdk == "opensnes" else "hello_world.sfc")
    if proc.returncode != 0 or not rom.is_file():
        return f"{sdk}/{NAMES[k]}: build failed\n" + (proc.stdout + proc.stderr)[-1200:]
    if sdk == "pvsneslib":
        wla_sym(rom.with_suffix(".sym"))
    return rom


def wla_sym(path: Path) -> None:
    """PVSnesLib's link writes `007e2000 name`; luna reads WLA-DX's
    `7e:2000 name` under a [labels] header. Same information, rewritten."""
    out = ["[labels]"]
    for line in path.read_text(errors="replace").splitlines():
        parts = line.split(None, 1)
        if len(parts) == 2 and len(parts[0]) == 8:
            try:
                addr = int(parts[0], 16)
            except ValueError:
                continue
            out.append(f"{(addr >> 16) & 0xFF:02x}:{addr & 0xFFFF:04x} {parts[1].strip()}")
    path.with_suffix(".sym.orig").write_text(path.read_text(errors="replace"))
    path.write_text("\n".join(out) + "\n")


def measure(rom: Path, luna: str):
    """(symbol -> master cycles, res, done) after FRAMES frames."""
    out = rom.with_suffix(".profile.json")
    subprocess.run([luna, "profile", str(rom), "--until-frame", str(FRAMES), "--out", str(out)],
                   capture_output=True, text=True)
    prof = json.loads(out.read_text())
    st = subprocess.run([luna, "state", "--until-frame", str(FRAMES), "--peek", "res:2",
                         "--peek", "done:2", "--out", "-", str(rom)], capture_output=True, text=True)
    peeks = json.loads(st.stdout)["peeks"]
    word = lambda p: int.from_bytes(bytes.fromhex(p["bytes_hex"]), "little")
    cycles: dict = {}
    for e in prof["entries"]:
        cycles[e["symbol"]] = cycles.get(e["symbol"], 0) + e["mclk"]
    return cycles, word(peeks[0]), word(peeks[1])


def one(job):
    sdk, k, pvs, luna = job
    rom = build(sdk, k, pvs)
    if isinstance(rom, str):
        return sdk, k, rom
    return sdk, k, measure(rom, luna)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--only", help="comma-separated workload names")
    ap.add_argument("--json", help="write the figures to this file")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    args = ap.parse_args()
    pvs = os.environ.get("PVSNESLIB_HOME")
    if not pvs or not (Path(pvs) / "devkitsnes" / "snes_rules").is_file():
        sys.exit("sdkbench: set PVSNESLIB_HOME to a PVSnesLib tree built for this machine")
    pvs = Path(pvs)
    luna = find_luna()
    wanted = [n for n in NAMES[1:] if not args.only or n in args.only.split(",")]
    ks = [0] + [NAMES.index(n) for n in wanted]
    jobs = [(sdk, k, pvs, luna) for sdk in ("opensnes", "pvsneslib") for k in ks]
    got: dict = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        for sdk, k, r in pool.map(one, jobs):
            if isinstance(r, str):
                sys.exit("sdkbench: " + r)
            got[sdk, k] = r

    def cost(sdk, k):
        base, work = got[sdk, 0][0], got[sdk, k][0]
        return sum(max(0, m - base.get(sym, 0)) for sym, m in work.items())

    rows, problems = [], []
    for k in ks[1:]:
        name = NAMES[k]
        ro, rp = got["opensnes", k][1], got["pvsneslib", k][1]
        for sdk in ("opensnes", "pvsneslib"):
            if got[sdk, k][2] != DONE:
                problems.append(f"{name}: the {sdk} ROM did not finish in {FRAMES} frames")
        if ro != rp:
            problems.append(f"{name}: the SDKs disagree on the result "
                            f"(OpenSNES 0x{ro:04X}, PVSnesLib 0x{rp:04X}) — not compared")
            continue
        rows.append((name, cost("pvsneslib", k), cost("opensnes", k), ro))

    print(f"{'workload':10} {'what':34} {'PVSnesLib':>11} {'OpenSNES':>11} {'OpenSNES vs PVSnesLib':>22}")
    for name, p, o, _ in rows:
        print(f"{name:10} {WHAT[name]:34} {p:>11,} {o:>11,} {100 * (o - p) / p:>+20.1f} %")
    if rows:
        tp, to = sum(r[1] for r in rows), sum(r[2] for r in rows)
        print(f"{'TOTAL':10} {'':34} {tp:>11,} {to:>11,} {100 * (to - tp) / tp:>+20.1f} %")
        print(f"\nmaster cycles; one NTSC frame is about 357,370. OpenSNES is faster on "
              f"{sum(o < p for _, p, o, _ in rows)} of {len(rows)} workloads.")
    for p in problems:
        print("PROBLEM:", p)
    if args.json:
        Path(args.json).write_text(json.dumps(
            {"frames": FRAMES, "rows": [dict(name=n, pvsneslib=p, opensnes=o, res=r) for n, p, o, r in rows],
             "problems": problems}, indent=2) + "\n")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
