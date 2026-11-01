#!/usr/bin/env python3
"""run.py — sprites at fixed VRAM slots against the dynamic engine, on luna.

One ROM per row of VARIANTS, built under build/slotbench/. Each plays a
generated scenario (README.md): `mix` has the shape mean 4, median 3, p90 10,
max 18 changes per tick, `burst` has every sprite change at once every 32
ticks. Reports master cycles per frame by symbol (luna profile, the same
window for every ROM), how many frames a
burst took to reach VRAM, every VRAM or OAM write made outside the blank (the
PPU drops those), and whether every slot ended on the frame asked.
It gates nothing: a report for a decision.

    python3 devtools/slotbench/run.py [--json OUT]
"""
import argparse
import csv
import json
import random
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna  # noqa: E402

WORK = REPO / "build" / "slotbench"
NFRAMES = 130
FROM_FRAME, TO_FRAME = 300, 1324   # 512 frames are one pass of the table
END_FRAME = 2300                   # the changes stop at tick 1000, frame 2000
VARIANTS = {
    # name: fixed mode, sprites, frames per VBlank, FlushUntil
    "dyn16": (0, 16, 5, 0),
    "dyn18": (0, 18, 5, 0),
    "fix16": (1, 16, 5, 0),
    "fix18": (1, 18, 5, 0),
    "fix18a": (2, 18, 5, 0),
    "fix18u": (1, 18, 9, 1),
}
# changes in a tick -> ticks of 256: mean 4.08, median 3, p90 10, max 18
MIX = {0: 46, 1: 44, 2: 32, 3: 26, 4: 20, 5: 16, 6: 12, 7: 9, 8: 8, 9: 7, 10: 12,
       11: 6, 12: 5, 13: 4, 14: 3, 15: 2, 16: 2, 17: 1, 18: 1}
# symbols a row groups, by name or (ending in _) by prefix: the assembly's
# labels are symbols of their own. What the scenario and the bench spend is apart.
GROUPS = {
    "scenario": ("scenario_tick", "bench_stats"),
    "decide": ("slotsUpdate", "vramQueuePushSprite"),
    "place": ("slotsPlace", "oamSet"),
    "dyn_draw": ("oamDynamic32Draw", "_o32d_"),
    "dyn_nmi": ("oamDynamicNmiFlush", "oamVramQueueUpdate", "oamInitDynamicSpriteEndFrame",
                "_vqu_", "_endframe_"),
    "flush": ("vramQueueFlush", "vramQueueFlushUntil"),
    "row_col": ("vramQueuePush",),
}
VRAM_DATA, OAM_DATA = "2118 2119".split(), "2104"


def changes(scenario):
    if scenario == "burst":
        return [18 if t % 32 == 0 else 0 for t in range(256)]
    table = [v for v, n in MIX.items() for _ in range(n)]
    assert len(table) == 256
    random.Random(2026).shuffle(table)
    return table


def sheets():
    """The frames: 9 rasters 128 wide, every word of a strip names frame and strip."""
    blob = bytearray(9 * 8192)
    for f in range(NFRAMES):
        base = (f // 16) * 8192 + (f % 16 // 4) * 0x800 + (f % 4) * 0x80
        for strip in range(4):
            tag = 0x8000 | f << 2 | strip
            for w in range(64):
                at = base + strip * 0x200 + w * 2
                blob[at:at + 2] = tag.to_bytes(2, "little")
    return bytes(blob)


def write_sources(d, scenario, config):
    (d / "sheets.bin").write_bytes(sheets())
    asm = []
    for i in range(9):
        asm += [f'.SECTION ".sheet{i}" SUPERFREE', f"sheet{i}:",
                f'.INCBIN "sheets.bin" SKIP {i * 8192} READ 8192', ".ENDS"]
    (d / "sheets.asm").write_text("\n".join(asm) + "\n")
    ents = ", ".join(f"sheet{f // 16} + {(f % 16 // 4) * 0x800 + (f % 4) * 0x80}"
                     for f in range(NFRAMES))
    (d / "scenario.h").write_text(
        config + f"#define NFRAMES {NFRAMES}\n"
        + "".join(f"extern const u8 sheet{i}[];\n" for i in range(9))
        + f"const u8 *const frame_src[{NFRAMES}] = {{ {ents} }};\n"
        + "static const u8 tick_changes[256] = {"
        + ",".join(map(str, changes(scenario))) + "};\n")


def build(name, scenario):
    fixed, n, frames, until = VARIANTS[name]
    d = WORK / f"{name}_{scenario}"
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    for f in ("Makefile", "main.c") + (("slots.asm",) if fixed == 2 else ()):
        shutil.copy(HERE / f, d)
    write_sources(d, scenario,
                  f"#define BENCH_FIXED {fixed}\n#define BENCH_N {n}\n"
                  f"#define BENCH_FRAMES {frames}\n#define BENCH_UNTIL {until}\n")
    r = subprocess.run(["make", f"OPENSNES={REPO}"], cwd=d,
                       capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"{name}: build failed\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return d / "slotbench.sfc"


def profile(luna, rom):
    out = rom.with_suffix(".prof.json")
    subprocess.run([luna, "profile", str(rom), "--from-frame", str(FROM_FRAME),
                    "--until-frame", str(TO_FRAME), "--out", str(out)],
                   check=True, capture_output=True)
    return json.loads(out.read_text())


def peeks(luna, rom, n):
    """The bench's counters at END_FRAME, and each slot read back from VRAM."""
    fixed16 = ["bench_frames", "bench_drain_max", "bench_done"]
    tiles = [i * 4 % 16 + (i >> 2) * 0x40 if i < 16 else 0x100 + (i - 16) * 4 for i in range(n)]
    args = [luna, "state", str(rom), "--no-render", "--until-frame", str(END_FRAME)]
    for label in fixed16:
        args += ["--peek", f"{label}:#2"]
    args += ["--peek", f"slot_want:#{n}"]
    for t in tiles:
        for strip in range(4):
            args += ["--peek", f"VRAM:{t * 16 + strip * 0x100:x}:#128"]
    err = subprocess.run(args, capture_output=True, text=True, check=True).stderr
    blocks = []
    for chunk in re.split(r"^peek ", err, flags=re.M)[1:]:
        data = []
        for line in chunk.splitlines()[1:]:
            m = re.match(r"\s+\$[0-9A-Fa-f]+\s+((?:[0-9A-Fa-f]{2}\s*)+)$", line)
            if m:
                data += [int(b, 16) for b in m.group(1).split()]
        blocks.append(bytes(data))
    assert len(blocks) == len(fixed16) + 1 + 4 * n, len(blocks)
    res = {k: int.from_bytes(blocks[i], "little") for i, k in enumerate(fixed16)}
    want = blocks[len(fixed16)]
    bad = 0
    for i in range(n):
        for strip in range(4):
            data = blocks[len(fixed16) + 1 + i * 4 + strip]
            tag = (0x8000 | want[i] << 2 | strip).to_bytes(2, "little")
            bad += data != tag * 64
    res["bad_strips"] = bad
    return res


def unsafe_writes(luna, rom):
    """Every write to VRAM and OAM over the window, with the beam at that moment."""
    trace = rom.with_suffix(".trace.csv")
    subprocess.run([luna, "state", str(rom), "--no-render", "--until-frame", str(TO_FRAME),
                    "--mem-trace", str(trace), "--mem-trace-addr", "2104:2119",
                    "--mem-trace-max", "40000000"], check=True, capture_output=True)
    res = {"vram_lost": 0, "oam_lost": 0, "lost_frames": 0, "last_line": 0, "written": 0}
    frames = set()
    with open(trace, newline="") as f:
        for row in csv.DictReader(f):
            if int(row["frame_ntsc"]) < FROM_FRAME or not row["kind"].lower().startswith("w"):
                continue
            addr = row["addr"].lower()[-4:]
            if addr not in VRAM_DATA and addr != OAM_DATA:
                continue
            res["written"] += 1
            safe = row["blank"].lower() in ("1", "true") or row["force_blank"].lower() in ("1", "true")
            if safe:
                res["last_line"] = max(res["last_line"], int(row["line"]))
            else:
                res["vram_lost" if addr in VRAM_DATA else "oam_lost"] += 1
                frames.add(row["frame_ntsc"])
    res["lost_frames"] = len(frames)
    trace.unlink()
    return res


def measure(luna, name, scenario):
    rom = build(name, scenario)
    p = profile(luna, rom)
    frames = p["frames"]
    row = dict.fromkeys(GROUPS, 0)
    for e in p["entries"]:
        base = e["symbol"].split("@")[0]
        for g, names in GROUPS.items():
            if any(base == x or (x.endswith("_") and base.startswith(x)) for x in names):
                row[g] += e["mclk"] / frames
    row = {k: round(v) for k, v in row.items()}
    fs = p["frame_summary"]
    row.update(active_mean=fs["active_mean"], active_max=fs["active_max"], lag=fs["lag_frames"])
    row.update(peeks(luna, rom, VARIANTS[name][1]))
    row.update(unsafe_writes(luna, rom))
    return row


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--json")
    args = ap.parse_args()
    luna = find_luna()
    results = {f"{v}/{s}": measure(luna, v, s) for s in ("mix", "burst") for v in VARIANTS}
    cols = list(next(iter(results.values())))
    print("variant/scenario".ljust(14), *(c[:9].rjust(9) for c in cols))
    for k, r in results.items():
        print(k.ljust(14), *(str(r[c]).rjust(9) for c in cols))
    if args.json:
        Path(args.json).write_text(json.dumps(results, indent=1) + "\n")
