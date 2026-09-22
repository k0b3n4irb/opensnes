#!/usr/bin/env python3
"""VBlank time budget: the NMI handler must fit in VBlank (gaps review R4).

VRAM, CGRAM and OAM are writable only during VBlank, which on NTSC is about
**51 800 master cycles** — and the NMI handler spends part of that before the
game gets its turn. Nothing measured that share until now: `make tests` proved
the handler was *correct*, never that it was *short enough*, so a new feature
inside it could eat the window and only show up as a dropped frame in someone
else's game.

`luna profile --budget SYMBOL=MCLK` is the gate: it compares the symbol's
**worst completed frame** against a ceiling and exits 1 when it is over (2 on
an unknown symbol, which makes a typo fail loudly). Exit status is the whole
contract — this script decides nothing, it only asks luna the right question
and reports what luna answered.

**Why the symbol file is rewritten.** WLA-DX emits the handler's internal
labels (`NmiHandler@oam_done`, `@mp5_done`, ... — 18 of them), and luna folds
each program counter onto the nearest label, so the handler's cost arrives
split across those names and `--budget NmiHandler=…` would measure only the
entry stub (652 mclk on sprite_swarm, against 7258 for the whole handler).
Summing the children would be wrong twice over: the maximum of a sum is not
the sum of maxima, and some children fall outside the printed rows. So we hand
luna a copy of the `.sym` with the child labels removed — `--sym` is a
documented luna input, and this is preparing that input, not computing the
answer ourselves. If luna ever folds child labels into their parent, this
rewriting step disappears and the rest stands.

Run:  python3 tools/luna-test/nmi_budget.py           # gate (in `make tests`)
      python3 tools/luna-test/nmi_budget.py --report  # print the numbers only
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from luna_runner import REPO_ROOT, find_luna  # noqa: E402

# Master cycles the handler may spend in its worst frame. NTSC VBlank is
# ~51 800 mclk; the corpus worst measured 2026-09-17 is 8112 (games/breakout),
# so this ceiling leaves the game about 5/6 of the window and still fails on a
# handler that doubles. Tighten it the way the bank-$00 ratchet is tightened:
# only after a measured improvement, never to make a red build green.
CEILING = 12000
FRAMES = 150

# A representative subset, not the whole corpus: each entry brings a distinct
# shape of NMI work, and the first four are the examples closest to the
# ceiling today. Profiling all 85 would cost minutes per push to re-measure
# handlers that do the same thing.
SUBSET = [
    ("games/breakout",        "the corpus worst (8112): sprites + tilemap + text"),
    ("games/likemario",       "sprites, scrolling and audio together"),
    ("games/rpg",             "map module, panel and text"),
    ("sprites/dynamic_sprite", "the dynamic-sprite VRAM queue flush"),
    ("maps/map_scroll",       "the map module's scroll DMA"),
    ("audio/snesmod_music",   "the audio driver's per-frame work"),
]

BUDGET_RE = re.compile(r"budget: (\S+) max (\d+) mclk \(frame (\d+)\)")


def rom_for(key: str) -> Path:
    roms = sorted((REPO_ROOT / "examples" / key).glob("*.sfc"))
    if not roms:
        sys.exit(f"nmi-budget: no ROM in examples/{key} — build the examples first")
    return roms[0]


def folded_sym(rom: Path, out_dir: Path) -> Path:
    """The ROM's .sym with the NMI handler's child labels removed."""
    src = rom.with_suffix(".sym")
    if not src.is_file():
        sys.exit(f"nmi-budget: {src.relative_to(REPO_ROOT)} missing")
    dst = out_dir / src.name
    dst.write_text("".join(l for l in src.read_text().splitlines(keepends=True)
                           if "NmiHandler@" not in l))
    return dst


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--report", action="store_true",
                    help="print the measured worst frames without gating")
    args = ap.parse_args()
    luna = find_luna()
    over = 0
    with tempfile.TemporaryDirectory() as td:
        for key, why in SUBSET:
            rom = rom_for(key)
            sym = folded_sym(rom, Path(td))
            cmd = [luna, "profile", str(rom), "--until-frame", str(FRAMES),
                   "--sym", str(sym), "--top", "1"]
            if not args.report:
                cmd += ["--budget", f"NmiHandler={CEILING}"]
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            if proc.returncode == 2:
                sys.exit(f"nmi-budget: luna rejected the symbol on {key}: "
                         f"{proc.stdout.strip()[-200:]}")
            m = BUDGET_RE.search(proc.stdout)
            if m:
                worst, frame = int(m.group(2)), m.group(3)
            else:  # --report mode: read the row luna printed
                row = [l for l in proc.stdout.splitlines() if l.rstrip().endswith("NmiHandler")]
                if not row:
                    sys.exit(f"nmi-budget: no NmiHandler row for {key} — did the "
                             f"handler get renamed?")
                worst, frame = int(row[0].split()[-2]), "?"
            pct = 100.0 * worst / CEILING
            verdict = "OVER" if proc.returncode == 1 else "ok"
            if proc.returncode == 1:
                over += 1
            print(f"  {verdict:4}  {key:24} {worst:6} mclk  ({pct:4.0f}% of {CEILING}, "
                  f"worst frame {frame})  — {why}")
    print(f"\nNMI VBlank budget: {len(SUBSET) - over}/{len(SUBSET)} within "
          f"{CEILING} master cycles" + (f", {over} OVER" if over else ""))
    return 1 if over else 0


if __name__ == "__main__":
    sys.exit(main())
