#!/usr/bin/env python3
"""run.py — the same library calls through OpenSNES and PVSnesLib, timed on luna.

`devtools/sdkbench` compares the compilers on C that calls no library. This
one compares the libraries: scene.c asks each SDK for what a game asks every
frame — read the pad, scroll, place sprites, send data to VRAM, print — and
this script reports what each request costs.

How a row is timed. A row is built twice per SDK: with its library calls
(CALLS = 1) and with the same loops around nothing (CALLS = 0). Both run for
the same number of frames on luna, and `luna profile` credits every master
cycle to the symbol being executed; the cost of the row is the sum of what
each symbol gained from the second ROM to the first. The loop is in both and
cancels; the call, its arguments, the library function, what it calls and
what the per-frame handler does because of it are all counted.

The last row is the frame itself: the master cycles per frame that are not
spent waiting, in a ROM that does nothing but wait for the next frame —
the SDK's vblank handler. It is the difference between two run lengths, so
that the boot does not count.

    python3 devtools/libbench/run.py              # OpenSNES against the committed reference
    python3 devtools/libbench/run.py --check      # the CI gate: OpenSNES against baseline.json
    python3 devtools/libbench/run.py --update     # rewrite baseline.json, the table of
                                                  # docs/PERF.md, and the reference when
                                                  # PVSNESLIB_HOME is set
    python3 devtools/libbench/run.py --only pad,text --detail

`baseline.json` holds the OpenSNES figures and is what --check compares with
(CI has no PVSnesLib); `pvsneslib_reference.json` holds PVSnesLib's, with the
commit they were measured at. With PVSNESLIB_HOME set to a PVSnesLib tree
built for this machine they are measured again instead of read.
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
sys.path.insert(0, str(HERE.parent / "sdkbench"))
from lib import find_luna  # noqa: E402
import run as sdkbench  # noqa: E402  (wla_sym, git_head)

WORK = REPO / "build" / "libbench"
ROWS = ["idle", "pad", "scroll", "oamset", "oamxy", "oamsize", "dma", "text", "frame", "worldc", "world", "vramc", "vramq", "worldo", "hideeach", "hiderange", "mapcol", "maprow"]
# what one repetition asks for, and how many calls that is
WHAT = {"pad": ("held, pressed, released of pad 0, ten times", 30),
        "scroll": ("`bgSetScroll` on three backgrounds, ten times", 30),
        "oamset": ("`oamSet` on 32 sprites", 32),
        "oamxy": ("`oamSetXY` on 32 sprites", 32),
        "oamsize": ("the size of 32 sprites", 32),
        "dma": ("2 KB to VRAM, one `dmaCopyVram`", 1),
        "text": ("20 characters printed and shown", 1),
        "frame": ("one frame: pad, three scrolls, 32 sprites, 20 characters", 1),
        "worldc": ("19 world-space sprites placed by a loop in C (the same source)", 19),
        "world": ("the same 19 sprites: `oamPlaceWorld` here, the C loop there", 19),
        "vramc": ("six 128-byte VRAM transfers, six `dmaCopyVram` calls", 6),
        "vramq": ("the same six, the part paid in VBlank: one `vramQueueFlush` here, the calls there", 6),
        "hideeach": ("32 sprites parked off screen, one call each", 32),
        "hiderange": ("the same 32 sprites in one call: `oamHideRange` here, `oamClear` there", 1),
        "mapcol": ("the map engine streaming one new column (29 entries), one `mapUpdate`", 29),
        "maprow": ("the map engine streaming one new row (33 entries), one `mapUpdate`", 33),
        "worldo": ("the 19 sprites sorted by depth (an order array): `oamPlaceWorld` here, the C loop there", 19)}
REPS = 20
FRAMES = 300
DONE = 0x600D
BASELINE = HERE / "baseline.json"
REFERENCE = HERE / "pvsneslib_reference.json"
# vblank_wai is the label of the `wai` inside WaitForVBlank (crt0.asm): luna folds the
# cycles onto the nearest label, so the halted cycles of a frame that only waits are
# credited to it, not to WaitForVBlank (the 2026-11-08 CI read 357 226 for "idle").
IDLE_SYMBOLS = ("main", "WaitForVBlank", "vblank_wai")

OPENSNES_MAKEFILE = """OPENSNES := {repo}
TARGET   := bench.sfc
ROM_NAME := LIBBENCH
USE_LIB  := 1
LIB_MODULES := console sprite dma background text input vramqueue map
SKIP_LINT := 1
CSRC := scene.c
include $(OPENSNES)/make/common.mk
"""


def build(sdk: str, row: int, calls: int, pvs):
    d = WORK / sdk / f"{ROWS[row]}_{calls}"
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    which = f"#define ROW {row}\n#define CALLS {calls}\n#define PVS {int(sdk == 'pvsneslib')}\n"
    env = {k: v for k, v in os.environ.items() if k not in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL")}
    if sdk == "opensnes":
        shutil.copy(HERE / "scene.c", d / "scene.c")
        (d / "which.h").write_text(which)
        (d / "Makefile").write_text(OPENSNES_MAKEFILE.format(repo=REPO))
        rom = d / "bench.sfc"
    else:
        skel = pvs / "snes-examples" / "hello_world"
        # 4.5 shipped hdr.asm, data.asm and the font in the example; 4.6
        # generates them at build time (AUTOHDR, built-in font). Copy the
        # Makefile, and the others only when the example still has them.
        for name in ("Makefile", "hdr.asm", "data.asm", "pvsneslibfont.png"):
            if name == "Makefile" or ((skel / name).is_file() and not (skel / "pvsneslibfont.inc").exists()):
                shutil.copy(skel / name, d / name)
        (d / "src").mkdir()
        shutil.copy(HERE / "scene.c", d / "src" / "hello_world.c")
        (d / "src" / "which.h").write_text(which)
        env["PVSNESLIB_HOME"] = str(pvs)
        rom = d / "hello_world.sfc"
    proc = subprocess.run(["make", "-s", "-C", str(d)], capture_output=True, text=True, env=env)
    if proc.returncode != 0 or not rom.is_file():
        sys.exit(f"libbench: {sdk}/{ROWS[row]} (CALLS={calls}): build failed\n" + (proc.stdout + proc.stderr)[-1500:])
    if sdk == "pvsneslib":
        sdkbench.wla_sym(rom.with_suffix(".sym"))
    return rom


def profile(rom: Path, luna: str, frames: int) -> dict:
    out = rom.with_suffix(f".{frames}.json")
    subprocess.run([luna, "profile", str(rom), "--until-frame", str(frames), "--out", str(out)],
                   capture_output=True, text=True)
    cycles: dict = {}
    for e in json.loads(out.read_text())["entries"]:
        # The compiler numbers its block labels across the file, so the same
        # block of a function after the row (main's last loop) is
        # `main@while_body.235` in one ROM and `.250` in the other: without
        # the number they cancel instead of counting as a gain (2026-11-11,
        # 1 280 mclk a repetition read as the cost of a call).
        name = re.sub(r"(@[A-Za-z_]+)\.\d+$", r"\1", e["symbol"])
        cycles[name] = cycles.get(name, 0) + e["mclk"]
    return cycles


def finished(rom: Path, luna: str) -> bool:
    st = subprocess.run([luna, "state", "--until-frame", str(FRAMES), "--peek", "done:2", "--out", "-", str(rom)],
                        capture_output=True, text=True)
    return bytes.fromhex(json.loads(st.stdout)["peeks"][0]["bytes_hex"]) == DONE.to_bytes(2, "little")


def one(job):
    sdk, row, pvs, luna = job
    if row == 0:
        rom = build(sdk, 0, 0, pvs)
        a, b = profile(rom, luna, FRAMES), profile(rom, luna, 2 * FRAMES)
        busy = sum(b[s] - a.get(s, 0) for s in b if s not in IDLE_SYMBOLS)
        return ROWS[0], {"cycles": round(busy / FRAMES), "detail": {}}
    with_calls, without = build(sdk, row, 1, pvs), build(sdk, row, 0, pvs)
    for rom in (with_calls, without):
        if not finished(rom, luna):
            sys.exit(f"libbench: {sdk}/{ROWS[row]}: {rom.parent.name} did not finish in {FRAMES} frames")
    a, b = profile(with_calls, luna, FRAMES), profile(without, luna, FRAMES)
    gains = {s: m - b.get(s, 0) for s, m in a.items() if m - b.get(s, 0) > 0}
    top = dict(sorted(gains.items(), key=lambda kv: -kv[1])[:4])
    return ROWS[row], {"cycles": round(sum(gains.values()) / REPS), "detail": top}


def run_sdk(sdk: str, rows: list, pvs, luna: str, jobs: int) -> dict:
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        return dict(pool.map(one, [(sdk, r, pvs, luna) for r in rows]))


DOC = REPO / "docs" / "PERF.md"
LABEL = {"idle": "a frame that only waits (the SDK's vblank handler)"}


def write_doc() -> None:
    """Rewrite the table of docs/PERF.md between its two markers from the
    committed JSON files, so the page cannot drift from them."""
    b = json.loads(BASELINE.read_text())["rows"]
    ref = json.loads(REFERENCE.read_text())
    t = ref["rows"]
    out = ["| Row | What it asks for | PVSnesLib | OpenSNES | |", "|---|---|---:|---:|---:|"]
    for n in ROWS:
        o = b[n]["cycles"]
        what = LABEL.get(n) or WHAT[n][0]
        if n not in t:     # a row added after the reference was measured
            out.append(f"| `{n}` | {what} | — | {o:,} | |")
            continue
        p = t[n]["cycles"]
        out.append(f"| `{n}` | {what} | {p:,} | {o:,} | {100 * (o - p) / p:+.1f} % |")
    out.append("")
    both = [n for n in ROWS if n in t]
    out.append(f"OpenSNES costs no more than PVSnesLib on **{sum(b[n]['cycles'] <= t[n]['cycles'] for n in both)} "
               f"of the {len(both)} rows** both were measured on. PVSnesLib at `{ref['pvsneslib']}`.")
    text = DOC.read_text()
    a, z = "<!-- libbench:begin -->", "<!-- libbench:end -->"
    i, j = text.index(a) + len(a), text.index(z)
    DOC.write_text(text[:i] + "\n" + "\n".join(out) + "\n" + text[j:])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--only", help="comma-separated row names")
    ap.add_argument("--check", action="store_true", help="the gate: compare OpenSNES with baseline.json")
    ap.add_argument("--update", action="store_true", help="rewrite baseline.json (and the reference if PVSNESLIB_HOME is set)")
    ap.add_argument("--detail", action="store_true", help="name the symbols that carry each row")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    args = ap.parse_args()
    luna = find_luna()
    wanted = [n for n in ROWS if not args.only or n in args.only.split(",")]
    rows = [ROWS.index(n) for n in wanted]
    ours = run_sdk("opensnes", rows, None, luna, args.jobs)
    pvs = os.environ.get("PVSNESLIB_HOME")
    if pvs and (Path(pvs) / "devkitsnes" / "snes_rules").is_file() and not args.check:
        theirs = run_sdk("pvsneslib", rows, Path(pvs), luna, args.jobs)
        origin = f"PVSnesLib measured now ({sdkbench.git_head(Path(pvs))})"
        if args.update and not args.only:
            REFERENCE.write_text(json.dumps({"pvsneslib": sdkbench.git_head(Path(pvs)), "rows": theirs}, indent=2) + "\n")
    else:
        ref = json.loads(REFERENCE.read_text())
        theirs, origin = ref["rows"], f"PVSnesLib from {REFERENCE.name} ({ref['pvsneslib']})"
    if args.update and not args.only:
        BASELINE.write_text(json.dumps({"rows": ours}, indent=2) + "\n")
        write_doc()
        print(f"libbench: {BASELINE.name} and the table of docs/PERF.md rewritten")
    if args.check:
        base = json.loads(BASELINE.read_text())["rows"]
        worse = [f"{n}: {base[n]['cycles']:,} -> {ours[n]['cycles']:,} (limit +10 %)"
                 for n in wanted if ours[n]["cycles"] > base[n]["cycles"] * 1.10]
        for n in wanted:
            print(f"{n:8} {base[n]['cycles']:>10,} -> {ours[n]['cycles']:>10,}")
        for w in worse:
            print("PROBLEM " + w)
        print(f"libbench: {len(wanted) - len(worse)}/{len(wanted)} rows within 10 % of baseline.json")
        return 1 if worse else 0

    print(f"{'row':8} {'PVSnesLib':>10} {'OpenSNES':>10} {'':>9}   per call PVS / OSN")
    behind = []
    for n in wanted:
        calls = WHAT[n][1] if n in WHAT else 1
        o = ours[n]["cycles"]
        if n not in theirs:
            print(f"{n:8} {'—':>10} {o:>10,} {'':>9}   {'—':>8} / {o / calls:,.0f}")
            continue
        p = theirs[n]["cycles"]
        print(f"{n:8} {p:>10,} {o:>10,} {100 * (o - p) / p:>+8.1f} %   {p / calls:>8,.0f} / {o / calls:,.0f}")
        if args.detail:
            for who, d in (("PVS", theirs[n]["detail"]), ("OSN", ours[n]["detail"])):
                print("           " + who + ": " + ", ".join(f"{s} {m // REPS:,}" for s, m in d.items()))
        if o > p:
            behind.append(n)
    print(f"\n{origin}. Master cycles for one frame's worth of each request (an NTSC frame is about 357,370);")
    print("idle: the per-frame handler of a ROM that only waits.")
    print(f"OpenSNES is behind on: {', '.join(behind) if behind else 'nothing'}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
