#!/usr/bin/env python3
"""run.py — the same C, built by OpenSNES and by PVSnesLib, timed on luna.

`workloads.c` holds twenty small workloads; this script builds one ROM
per workload (plus a baseline that runs none), runs each on luna for the
same number of frames, and reports for every workload:

  cycles  the master cycles it cost (`luna profile`),
  size    the bytes of code of its functions,
  stack   how deep the stack went while it ran.

How a workload is timed. `luna profile` credits every master cycle to the
symbol being executed. A ROM that runs a workload and the baseline ROM of
the same SDK run for the same number of frames, so they differ only by the
workload: the symbols it executes gain cycles, the idle loop loses as many,
the NMI handler stays equal. The cost of the workload is the sum of the
gains. Everything is counted — the function, what it calls, the runtime's
multiply and divide — and nothing of the SDK's boot or per-frame handler.

The two ROMs must leave the same checksum in `res`: a workload on which the
SDKs disagree is reported as such and not compared.

    python3 devtools/sdkbench/run.py              # OpenSNES against the committed reference
    python3 devtools/sdkbench/run.py --check      # the CI gate: OpenSNES against baseline.json
    python3 devtools/sdkbench/run.py --update     # rewrite baseline.json (and the reference
                                                  # when PVSNESLIB_HOME is set)
    python3 devtools/sdkbench/run.py --only sort,crc

Two files are committed beside this script. `baseline.json` holds the
OpenSNES figures and is what --check compares with (CI has no PVSnesLib).
`pvsneslib_reference.json` holds PVSnesLib's figures, with the date and the
commit they were measured at; with PVSNESLIB_HOME set to a PVSnesLib tree
built for this machine they are measured again instead of read.

--check fails when the total of the cycles grows by more than 5 %, or one
workload by more than 25 %, or a checksum changes. Both ROMs are LoROM,
SlowROM (each SDK's default).
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
         "long", "bytes", "calls", "switch", "crc", "list",
         "tilemap", "grid", "entities", "copy", "strings", "state", "place", "dist", "depot"]
WHAT = {"sieve": "sieve of 1024, byte array", "sort": "insertion sort, 64 words",
        "physics": "32 entities x 60 steps", "collide": "496 box pairs x 8",
        "mul": "2304 variable multiplies", "decimal": "200 numbers to digits (/10, %10)",
        "long": "300 steps of 32-bit math", "bytes": "512-byte fill, copy, compare x 4",
        "calls": "recursive fib(17)", "switch": "1280 interpreted ops",
        "crc": "CRC-16 of 256 bytes, bitwise", "list": "linked list, 40 walks",
        "tilemap": "32x16 tilemap, write + 1200 lookups", "grid": "16x32 grid, 4 neighbours",
        "entities": "32 entities x 60, by pointer", "copy": "word and byte copy loops",
        "strings": "strlen / strcmp / strcpy by hand", "state": "600 steps, switch + fn table",
        "place": "19 sprites x 200, parallel tables (issue #166)",
        "dist": "81 distances x 40, nested loop and a call (issue #166)",
        "depot": "18 agents, 30 bursts of 16 decisions (issue #166)"}
FRAMES = 900
DONE = 0x600D
BASELINE = HERE / "baseline.json"
REFERENCE = HERE / "pvsneslib_reference.json"

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


# The functions of each workload, for the size column.
FUNCS = {"sieve": ["w_sieve"], "sort": ["w_sort", "rnd16"],
         "physics": ["w_physics", "ent_init", "rnd16"], "collide": ["w_collide", "ent_init", "rnd16"],
         "mul": ["w_mul"], "decimal": ["w_decimal"], "long": ["w_long"], "bytes": ["w_bytes"],
         "calls": ["w_calls", "fib"], "switch": ["w_switch"], "crc": ["w_crc"], "list": ["w_list"],
         "tilemap": ["w_tilemap"], "grid": ["w_grid", "rnd16"],
         "entities": ["w_entities", "ent_init", "rnd16"], "copy": ["w_copy"],
         "strings": ["w_strings", "slen", "scmp", "scpy"],
         "state": ["w_state", "op_add", "op_xor", "op_rot", "op_dec"],
         "place": ["w_place", "place"],
         "dist": ["w_dist", "distances", "vectorLength"],
         "depot": ["w_depot", "gap", "shake", "tally", "clampX", "clampY", "bearing", "halt", "steer", "depart", "forecast", "sidestep", "crowd", "intruder", "slotFor", "decide", "setup"]}
STACK_TOP = 0x1FFF          # both SDKs start their stack there


def code_size(sdk: str, rom: Path, funcs: list) -> int:
    """Bytes of code of these functions in this ROM."""
    if sdk == "opensnes":
        # WLA-DX [sections]: `offset bank:addr addr size name`, one per function
        size, on = {}, False
        for line in rom.with_suffix(".sym").read_text().splitlines():
            if line.startswith("["):
                on = line.strip() == "[sections]"
                continue
            parts = line.split()
            if on and len(parts) == 5 and parts[4].startswith(".text."):
                size[parts[4][6:].split(".")[0]] = int(parts[3], 16)
        # a helper that is no longer a function was inlined into its one
        # caller (cc65816, 2026-10-10): its bytes are counted there
        if funcs[0] not in size:
            sys.exit(f"sdkbench: {funcs[0]} is not a function of {rom.name}")
        return sum(size.get(f, 0) for f in funcs)
    # PVSnesLib: no section sizes. A function runs to the next function label
    # of its bank; the last one of a bank runs to its `rtl` ($6B), looked for
    # after the function's last local label so that an operand byte is not
    # taken for it.
    labels = []
    for line in rom.with_suffix(".sym").read_text().splitlines():
        parts = line.split(None, 1)
        if len(parts) == 2 and ":" in parts[0]:
            bank, addr = parts[0].split(":")
            labels.append((int(bank, 16), int(addr, 16), parts[1].strip()))
    labels.sort()
    image = rom.read_bytes()
    total = 0
    for f in funcs:
        # a static function is `<file>.asm_<name>` to 816-tcc, a global one `<name>`
        k = next(n for n, lab in enumerate(labels) if lab[2] == f or lab[2].endswith(".asm_" + f))
        bank, start = labels[k][0], labels[k][1]
        last, end = start, None
        for b2, a2, name in labels[k + 1:]:
            if b2 != bank:
                break
            if not name.startswith("__local"):
                end = a2
                break
            last = a2
        if end is None:                               # LoROM: bank * $8000 + (addr - $8000)
            off = bank * 0x8000 + (last - 0x8000)
            end = last + image.index(b"\x6b", off) - off + 1
        total += end - start
    return total


def measure(rom: Path, luna: str):
    """(symbol -> master cycles, res, done, lowest stack pointer) after FRAMES frames."""
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
    return cycles, word(peeks[0]), word(peeks[1]), prof["stack"]["low"]["sp"]


def one(job):
    sdk, k, pvs, luna = job
    rom = build(sdk, k, pvs)
    if isinstance(rom, str):
        return sdk, k, rom
    size = code_size(sdk, rom, FUNCS[NAMES[k]]) if k else 0
    return sdk, k, measure(rom, luna) + (size,)


def run_sdk(sdk: str, ks: list, pvs, luna: str, jobs: int) -> dict:
    """{workload: {cycles, size, stack, res}} for one SDK."""
    got = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        for _, k, r in pool.map(one, [(sdk, k, pvs, luna) for k in ks]):
            if isinstance(r, str):
                sys.exit("sdkbench: " + r)
            got[k] = r
    base = got[0][0]
    out = {}
    for k in ks[1:]:
        cycles, res, done, low, size = got[k]
        if done != DONE:
            sys.exit(f"sdkbench: {NAMES[k]}: the {sdk} ROM did not finish in {FRAMES} frames")
        out[NAMES[k]] = {"cycles": sum(max(0, m - base.get(sym, 0)) for sym, m in cycles.items()),
                         "size": size, "stack": STACK_TOP - low, "res": res}
    return out


DOC = REPO / "docs" / "BENCHMARK.md"
DESCRIPTION = {
    "sieve": "sieve of 1024 in a byte array", "sort": "insertion sort of 64 words",
    "physics": "32 entities bouncing, 60 steps, by index", "collide": "496 box pairs tested, 8 rounds",
    "mul": "2304 multiplies of two variables", "decimal": "200 numbers to decimal digits (`/ 10`, `% 10`)",
    "long": "300 steps of a 32-bit generator and hash", "bytes": "512-byte fill, copy and compare, 4 passes",
    "calls": "recursive `fib(17)`", "switch": "1280 operations of a `switch` interpreter",
    "crc": "CRC-16 of 256 bytes, bit by bit", "list": "a 64-node linked list walked 40 times",
    "tilemap": "a 32×16 tilemap written, then 1200 lookups",
    "grid": "a 16×32 byte grid, four neighbours of each cell",
    "entities": "the 32 entities again, through a pointer", "copy": "word and byte copies as index loops",
    "strings": "`strlen`, `strcmp`, `strcpy` written by hand",
    "state": "600 steps of a `switch` state machine and a table of functions",
    "place": "19 sprites placed 200 times: a loop over parallel tables with an on-screen test (issue #166)",
    "dist": "81 distances between two teams, 40 times: a nested loop, two absolute values and a call (issue #166)",
    "depot": "eighteen agents deciding where to go, 30 bursts of 16 decisions: parallel arrays, helpers called from loops, values live across calls (issue #166)"}


def write_doc() -> None:
    """Rewrite the table of docs/BENCHMARK.md between its two markers from
    the committed JSON files, so the page cannot drift from them."""
    b = json.loads(BASELINE.read_text())["workloads"]
    t = json.loads(REFERENCE.read_text())["workloads"]
    pct = lambda o, p: f"{100 * (o - p) / p:+.1f} %"
    out = ["| Workload | What it does | Cycles: PVSnesLib | OpenSNES | | Size: PVS | OSN | | Stack: PVS | OSN |",
           "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for n, o in b.items():
        p = t[n]
        out.append(f"| `{n}` | {DESCRIPTION[n]} | {p['cycles']:,} | {o['cycles']:,} | {pct(o['cycles'], p['cycles'])} "
                   f"| {p['size']} | {o['size']} | {pct(o['size'], p['size'])} | {p['stack']} | {o['stack']} |")
    tot = lambda d, k: sum(v[k] for v in d.values())
    out.append(f"| **Total** | | **{tot(t, 'cycles'):,}** | **{tot(b, 'cycles'):,}** | "
               f"**{pct(tot(b, 'cycles'), tot(t, 'cycles'))}** | **{tot(t, 'size')}** | **{tot(b, 'size')}** | "
               f"**{pct(tot(b, 'size'), tot(t, 'size'))}** | | |")
    out.append("")
    out.append(f"Of the {len(b)} workloads OpenSNES is **faster on {sum(b[n]['cycles'] < t[n]['cycles'] for n in b)}**, "
               f"**no larger on {sum(b[n]['size'] <= t[n]['size'] for n in b)}**, and **no deeper in stack on "
               f"{sum(b[n]['stack'] <= t[n]['stack'] for n in b)}**. Both ROMs leave the same checksum for every "
               "workload, so they computed the same thing.")
    text = DOC.read_text()
    a, z = "<!-- sdkbench:begin -->", "<!-- sdkbench:end -->"
    i, j = text.index(a) + len(a), text.index(z)
    DOC.write_text(text[:i] + "\n" + "\n".join(out) + "\n" + text[j:])


def git_head(path: Path) -> str:
    r = subprocess.run(["git", "-C", str(path), "log", "-1", "--format=%h %cs"], capture_output=True, text=True)
    return r.stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--only", help="comma-separated workload names")
    ap.add_argument("--check", action="store_true", help="the gate: compare OpenSNES with baseline.json")
    ap.add_argument("--update", action="store_true", help="rewrite baseline.json (and the reference if PVSNESLIB_HOME is set)")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    args = ap.parse_args()
    luna = find_luna()
    wanted = [n for n in NAMES[1:] if not args.only or n in args.only.split(",")]
    ks = [0] + [NAMES.index(n) for n in wanted]
    ours = run_sdk("opensnes", ks, None, luna, args.jobs)

    pvs = os.environ.get("PVSNESLIB_HOME")
    if pvs and (Path(pvs) / "devkitsnes" / "snes_rules").is_file() and not args.check:
        theirs = run_sdk("pvsneslib", ks, Path(pvs), luna, args.jobs)
        origin = f"PVSnesLib measured now ({git_head(Path(pvs))})"
        if args.update and not args.only:
            REFERENCE.write_text(json.dumps({"pvsneslib": git_head(Path(pvs)), "frames": FRAMES,
                                             "workloads": theirs}, indent=2) + "\n")
    else:
        ref = json.loads(REFERENCE.read_text())
        theirs = {n: v for n, v in ref["workloads"].items() if n in wanted}
        origin = f"PVSnesLib from pvsneslib_reference.json ({ref['pvsneslib']})"
    if args.update and not args.only:
        BASELINE.write_text(json.dumps({"frames": FRAMES, "workloads": ours}, indent=2) + "\n")
        write_doc()
        print(f"sdkbench: {BASELINE.name} and the table of docs/BENCHMARK.md rewritten")

    problems = []
    if args.check:
        base = json.loads(BASELINE.read_text())["workloads"]
        tb = sum(base[n]["cycles"] for n in wanted)
        tn = sum(ours[n]["cycles"] for n in wanted)
        if tn > tb * 1.05:
            problems.append(f"total cycles {tb:,} -> {tn:,} ({100 * (tn - tb) / tb:+.1f} %, limit +5 %)")
        for n in wanted:
            if ours[n]["cycles"] > base[n]["cycles"] * 1.25:
                problems.append(f"{n}: cycles {base[n]['cycles']:,} -> {ours[n]['cycles']:,} (limit +25 %)")
            if ours[n]["res"] != base[n]["res"]:
                problems.append(f"{n}: checksum 0x{base[n]['res']:04X} -> 0x{ours[n]['res']:04X} — the workload computes something else")

    pct = lambda o, p: f"{100 * (o - p) / p:+.1f} %" if p else "-"
    print(f"{'workload':9} {'cycles PVS':>11} {'cycles OSN':>11} {'':>8}  {'size PVS':>8} {'OSN':>5} {'':>8}  {'stack PVS':>9} {'OSN':>4}")
    rows = []
    for n in wanted:
        o, p = ours[n], theirs.get(n)
        if not p:
            continue
        if o["res"] != p["res"]:
            problems.append(f"{n}: the SDKs disagree on the result (OpenSNES 0x{o['res']:04X}, "
                            f"PVSnesLib 0x{p['res']:04X}) — not compared")
            continue
        rows.append((n, o, p))
        print(f"{n:9} {p['cycles']:>11,} {o['cycles']:>11,} {pct(o['cycles'], p['cycles']):>8}  "
              f"{p['size']:>8} {o['size']:>5} {pct(o['size'], p['size']):>8}  {p['stack']:>9} {o['stack']:>4}")
    if rows:
        tot = lambda who, key: sum((o if who == "o" else p)[key] for _, o, p in rows)
        print(f"{'TOTAL':9} {tot('p', 'cycles'):>11,} {tot('o', 'cycles'):>11,} "
              f"{pct(tot('o', 'cycles'), tot('p', 'cycles')):>8}  {tot('p', 'size'):>8} {tot('o', 'size'):>5} "
              f"{pct(tot('o', 'size'), tot('p', 'size')):>8}")
        faster = sum(o["cycles"] < p["cycles"] for _, o, p in rows)
        smaller = sum(o["size"] <= p["size"] for _, o, p in rows)
        shallower = sum(o["stack"] <= p["stack"] for _, o, p in rows)
        print(f"\n{origin}. Of {len(rows)} workloads OpenSNES is faster on {faster}, "
              f"no larger on {smaller}, no deeper in stack on {shallower}.")
        print("cycles: master cycles (an NTSC frame is about 357,370); size: bytes of code; "
              "stack: bytes below the initial stack pointer at the deepest point.")
    for p in problems:
        print("PROBLEM:", p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
