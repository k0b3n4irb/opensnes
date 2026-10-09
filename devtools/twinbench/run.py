#!/usr/bin/env python3
"""run.py — the examples both SDKs have, run side by side on luna.

`sdkbench` compares the compilers on C without a library, `libbench` the
libraries call by call. This one runs whole examples: for each example that
exists in PVSnesLib and has its twin here, both ROMs play the same input
script on luna and the script reports, per frame, the master cycles each
spends not waiting — everything the game and the SDK do in a frame.

The twins are ports, not the same source: a row says what the same example
costs under each SDK, and a row whose two ROMs do not do the same work says
nothing (see PAIRS for what was checked).

    PVSNESLIB_HOME=~/pvsneslib python3 devtools/twinbench/run.py
    python3 devtools/twinbench/run.py --only likemario,breakout

PVSnesLib's ROMs are the ones built in its tree (`make` in snes-examples).
"""
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
sys.path.insert(0, str(HERE.parent / "sdkbench"))
from lib import find_luna  # noqa: E402
import run as sdkbench  # noqa: E402

WORK = REPO / "build" / "twinbench"
RIGHT, START, A = "0x0100", "0x1000", "0x0080"
# Pairs read side by side and found NOT to do the same work: left out unless
# --all is given, and never a figure to quote.
NOT_TWINS = {
    "dynamicsprite": "ours draws 4 sprites and refreshes them every 8 frames, theirs 1 every 16",
    "metasprite": "ours redraws its metasprites every frame, theirs draws them once",
    "animatedsprite": "ours plays through the anim module (animPlay + animTick), theirs steps a counter",
}
# name, PVSnesLib directory, ours, input script (or None), first and last frame
PAIRS = [
    ("likemario", "games/likemario", "games/likemario", f"130:{RIGHT}", 200, 500),
    ("breakout", "games/breakout", "games/breakout", f"100:{START},110:0,150:{A},160:0", 200, 500),
    ("mapandobjects", "objects/mapandobjects", "games/mapandobjects", f"130:{RIGHT}", 200, 500),
    ("mapscroll", "maps/mapscroll", "maps/map_scroll", f"130:{RIGHT}", 200, 500),
    ("slopemario", "maps/slopemario", "maps/slope_collision", f"130:{RIGHT}", 200, 500),
    ("tiled", "maps/tiled", "maps/tiled", f"130:{RIGHT}", 200, 500),
    ("dynamicmap", "maps/DynamicMap", "maps/dynamic_map", None, 200, 500),
    ("animatedsprite", "graphics/Sprites/AnimatedSprite", "sprites/animated_sprite", f"130:{RIGHT}", 200, 500),
    ("dynamicsprite", "graphics/Sprites/DynamicSprite", "sprites/dynamic_sprite", None, 200, 500),
    ("dynamicmetasprite", "graphics/Sprites/DynamicEngineMetaSprite", "sprites/dynamic_metasprite", None, 200, 500),
    ("metasprite", "graphics/Sprites/MetaSprite", "sprites/metasprite", None, 200, 500),
    ("objectsize", "graphics/Sprites/ObjectSize", "sprites/sprite_sizes", None, 200, 500),
    ("simplesprite", "graphics/Sprites/SimpleSprite", "sprites/simple_sprite", None, 200, 500),
    ("mode1", "graphics/Backgrounds/Mode1", "backgrounds/mode1", None, 200, 500),
    ("continuousscroll", "graphics/Backgrounds/Mode1ContinuosScroll", "scrolling/continuous_scroll", f"130:{RIGHT}", 200, 500),
    ("mixedscroll", "graphics/Backgrounds/Mode1MixedScroll", "scrolling/mixed_scroll", None, 200, 500),
    ("mode7", "graphics/Backgrounds/Mode7", "mode7/rotate_scale", f"130:{A}", 200, 500),
    ("mode7perspective", "graphics/Backgrounds/Mode7Perspective", "mode7/perspective", None, 200, 500),
    ("parallax", "graphics/Effects/ParallaxScrolling", "scrolling/parallax_scroll", None, 200, 500),
    ("waves", "graphics/Effects/Waves", "hdma/hdma_wave", None, 200, 500),
    ("gradient", "graphics/Effects/GradientColors", "hdma/gradient_colors", None, 200, 500),
    ("music", "audio/music", "audio/snesmod_music", None, 200, 500),
    ("effects", "audio/effects", "audio/snesmod_sfx", f"210:{A},220:0,300:{A},310:0", 200, 500),
    ("controller", "input/controller", "input/controller", f"210:{A},260:{RIGHT},300:0", 200, 500),
    ("hello", "hello_world", "text/print_string", None, 200, 500),
    ("timer", "timer", "basics/timer", None, 200, 500),
]


def rom_in(d: Path) -> Path:
    roms = sorted(d.glob("*.sfc"))
    if not roms:
        sys.exit(f"twinbench: no ROM in {d} — build it first")
    return roms[0]


def measure(job):
    name, sdk, rom, script, first, last, luna = job
    d = WORK / sdk / name
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    local = d / rom.name
    shutil.copy(rom, local)
    sym = rom.with_suffix(".sym")
    if sym.is_file():
        shutil.copy(sym, local.with_suffix(".sym"))
        if sdk == "pvsneslib":
            sdkbench.wla_sym(local.with_suffix(".sym"))
    out = d / "profile.json"
    cmd = [luna, "profile", str(local), "--from-frame", str(first), "--until-frame", str(last), "--out", str(out)]
    if script:
        cmd += ["--input", script]
    subprocess.run(cmd, capture_output=True, text=True)
    p = json.loads(out.read_text())
    shot = d / "end.png"
    st = [luna, "state", "--until-frame", str(last), "--screenshot", str(shot), "--out", str(d / "state.json"), str(local)]
    if script:
        st += ["--input", script]
    subprocess.run(st, capture_output=True, text=True)
    idle = sum(e.get("idle_mclk", 0) for e in p["entries"])
    busy = sorted(((e["mclk"] - e.get("idle_mclk", 0)) // p["frames"], e["symbol"]) for e in p["entries"])[::-1]
    return name, sdk, {"busy": round((p["total_mclk"] - idle) / p["frames"]), "rom": rom.stat().st_size,
                       "top": [f"{s} {m:,}" for m, s in busy[:5]]}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--only")
    ap.add_argument("--detail", action="store_true")
    ap.add_argument("--all", action="store_true", help="also run the pairs listed in NOT_TWINS")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    args = ap.parse_args()
    pvs = os.environ.get("PVSNESLIB_HOME")
    if not pvs:
        sys.exit("twinbench: set PVSNESLIB_HOME to a PVSnesLib tree with its examples built")
    luna = find_luna()
    pairs = [p for p in PAIRS if (p[0] in args.only.split(",") if args.only else args.all or p[0] not in NOT_TWINS)]
    jobs = []
    for name, theirs, ours, script, first, last in pairs:
        jobs.append((name, "pvsneslib", rom_in(Path(pvs) / "snes-examples" / theirs), script, first, last, luna))
        jobs.append((name, "opensnes", rom_in(REPO / "examples" / ours), script, first, last, luna))
    got = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        for name, sdk, r in pool.map(measure, jobs):
            got[(name, sdk)] = r
    print(f"{'example':18} {'PVSnesLib':>10} {'OpenSNES':>10} {'':>9}")
    for name, *_ in pairs:
        p, o = got[(name, "pvsneslib")], got[(name, "opensnes")]
        print(f"{name:18} {p['busy']:>10,} {o['busy']:>10,} {100 * (o['busy'] - p['busy']) / p['busy']:>+8.1f} %"
              + (f"   NOT TWINS: {NOT_TWINS[name]}" if name in NOT_TWINS else ""))
        if args.detail:
            print("    PVS: " + "; ".join(p["top"]))
            print("    OSN: " + "; ".join(o["top"]))
    print("\nMaster cycles per frame spent not waiting (an NTSC frame is about 357,370), mean over the window.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
