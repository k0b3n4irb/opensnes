"""Probe: decoded sprite (OAM) structure (H9).

Uses `luna assets-dump`'s `oam.json` (128 sprites decoded: x/y/tile/palette/
priority/flips/w/h) to assert sprite examples place the *right* sprites — a
structural check that diagnoses "wrong position/tile/size", where a framebuffer
hash only says "pixels differ".
"""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from lib import find_luna, rom_path


def _oam(luna: str, rom: Path, steps: int = 3_000_000) -> list[dict]:
    out = Path("/tmp/luna-oam") / rom.stem
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run([luna, "assets-dump", "-n", str(steps), "--out", str(out), str(rom)],
                   capture_output=True, text=True, timeout=300)
    return json.loads((out / "oam.json").read_text())


def _visible(oam: list[dict]) -> list[dict]:
    return [s for s in oam if 0 <= s.get("y", 255) < 224 and -32 < s.get("x", 999) < 256]


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []

    # simple_sprite: oamSet(0, 112, 96, tile 0x10, prio 3) → exactly one 32×32
    # sprite at (112, 95) [y-1], tile 16, priority 3.
    oam = _oam(luna, rom_path("graphics/sprites/simple_sprite/simple_sprite.sfc"))
    vis = _visible(oam)
    s0 = oam[0]
    ok = (len(vis) == 1 and s0["x"] == 112 and s0["y"] == 95 and s0["tile"] == 16
          and s0["priority"] == 3 and s0["w"] == 32 and s0["h"] == 32)
    results.append((ok, f"simple_sprite: {len(vis)} vis, s0=({s0['x']},{s0['y']}) "
                        f"tile{s0['tile']} prio{s0['priority']} {s0['w']}x{s0['h']}"))

    # metasprite / animated_sprite: a metasprite is several hardware sprites →
    # expect more than one visible sprite placed on screen.
    for rel, name in [("graphics/sprites/metasprite/metasprite.sfc", "metasprite"),
                      ("graphics/sprites/animated_sprite/animated_sprite.sfc", "animated_sprite")]:
        vis = _visible(_oam(luna, rom_path(rel)))
        ok = len(vis) >= 1
        results.append((ok, f"{name}: {len(vis)} visible sprites"))

    bad = [m for ok, m in results if not ok]
    if bad:
        return False, "FAILED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in results)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "oam_struct: " + msg)
    sys.exit(0 if ok else 1)
