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

Run:  python3 testing/nmi_budget.py           # gate (in `make tests`)
      python3 testing/nmi_budget.py --report  # print the numbers only
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
from lib import REPO_ROOT, find_luna  # noqa: E402

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
# Each entry carries its measured worst frame (luna v1.27.0, 2026-09-26,
# including the +378 mclk of the pad-presence read added that day): a
# handler that grows more than DRIFT over it fails, even far below CEILING —
# the single ceiling alone let map_scroll go from 7298 to 11999 unnoticed.
# Re-measure with --report after an intentional NMI change and update here,
# saying why in the commit.
DRIFT = 0.10
SUBSET = [
    ("games/breakout",        8490, "the corpus worst: sprites + tilemap + text"),
    ("games/likemario",       8170, "sprites, scrolling and audio together"),
    ("games/rpg",             8170, "map module, panel and text"),
    ("sprites/dynamic_sprite", 7676, "the dynamic-sprite VRAM queue flush"),
    ("maps/map_scroll",       7668, "the map module's scroll DMA"),
    ("audio/snesmod_music",   5650, "the audio driver's per-frame work"),
    # Super FX builds (2026-10-03): the handler is reached through the WRAM
    # vector blob, and its cost is the same order as everywhere else.
    ("chips/superfx_3d",      8272, "a Super FX build: the handler behind the WRAM vectors"),
    ("chips/superfx_game_skeleton", 8224, "the handler while gsuPresent runs"),
]

# Symbols other than NmiHandler that run in the vertical interrupt, with their
# own reference and NO absolute ceiling: only growth is gated. gsuPresent's
# step uploads half the framebuffer per interrupt and is meant to outlast
# VBlank — it runs inside the 40 + 40 line letterbox blank, and that every
# byte lands in blank is vram_dma_blank.py's check, not this one's.
EXTRA = [
    ("chips/superfx_game_skeleton", "gsu_present_step", 91956,
     "gsuPresent's half-frame upload, in the letterbox blank"),
    # The NmiHandler rows measure the handler's own time: the work it hands
    # to a hook (tilemapFlush, the dynamic-sprite queue) runs in VBlank too
    # and was invisible to the gate (library audit 2026-10-03, PF2). Each is
    # gated on growth from its own measured reference (2026-10-04).
    # A fifth field names a `luna test` manifest whose input script drives the
    # run (merged as rom_coverage does): without a button press these hooks
    # barely run — breakout's tilemapFlush never does in 150 idle frames.
    # A sixth field is the first frame measured. luna credits a symbol
    # whoever called it, and textInit() calls tilemapFlush() itself, in force
    # blank: since 2026-10-09 the boot is short enough for that call and the
    # hook's first one to fall in the same frame (3), which read as a
    # doubling. The row is about the hook, so it starts after the boot.
    ("basics/scene_stack", "tilemapFlush", 17652,
     "the text module's full-map DMA from the NMI hook (a title redraw)", "state_scene_stack", 10),
    ("sprites/dynamic_sprite", "oamDynamicNmiFlush", 296,
     "the dynamic-sprite engine's NMI step under the sprite manifest", "oam_dynamic_sprite"),
    ("sprites/dynamic_sprite", "oamVramQueueUpdate", 312,
     "the dynamic-sprite VRAM queue flush it calls", "oam_dynamic_sprite"),
]

BUDGET_RE = re.compile(r"budget: (\S+) max (\d+) mclk \(frame (\d+)\)")


def rom_for(key: str) -> Path:
    roms = sorted((REPO_ROOT / "examples" / key).glob("*.sfc"))
    if not roms:
        sys.exit(f"nmi-budget: no ROM in examples/{key} — build the examples first")
    return roms[0]


def folded_sym(rom: Path, out_dir: Path, symbol: str = "NmiHandler") -> Path:
    """The ROM's .sym with the child labels of `symbol` removed."""
    src = rom.with_suffix(".sym")
    if not src.is_file():
        sys.exit(f"nmi-budget: {src.relative_to(REPO_ROOT)} missing")
    dst = out_dir / f"{symbol}.{src.name}"
    dst.write_text("".join(l for l in src.read_text(encoding="utf-8").splitlines(keepends=True)
                           if f"{symbol}@" not in l))
    return dst


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--report", action="store_true",
                    help="print the measured worst frames without gating")
    args = ap.parse_args()
    luna = find_luna()
    over = 0
    with tempfile.TemporaryDirectory() as td:
        rows = [(key, "NmiHandler", ref, CEILING, why, None) for key, ref, why in SUBSET]
        rows = [r for r in rows if len(r) == 6] + \
               [(e[0], e[1], e[2], None, e[3], e[4] if len(e) > 4 else None) for e in EXTRA]
        first = {(e[0], e[1]): e[5] for e in EXTRA if len(e) > 5}
        for key, symbol, ref, limit, why, manifest in rows:
            rom = rom_for(key)
            sym = folded_sym(rom, Path(td), symbol)
            bound = ["--until-frame", str(FRAMES)]
            if manifest:
                from rom_coverage import manifest_runs
                found = [r for r in manifest_runs(rom) if r[0] == manifest]
                if not found:
                    sys.exit(f"nmi-budget: no manifest {manifest} drives examples/{key}")
                _, bound, script = found[0]
                if script:
                    bound = bound + ["--input", script]
            if (key, symbol) in first:
                bound = ["--from-frame", str(first[(key, symbol)])] + bound
            # --report still asks for a budget, with a ceiling nothing reaches:
            # luna then prints its `budget:` line (worst frame and its number)
            # and never gates. Reading the `--top` table instead broke the day
            # WaitForVBlank outranked the handler (2026-09-26 audit).
            ceiling = 10**9 if (args.report or limit is None) else limit
            cmd = [luna, "profile", str(rom), *bound,
                   "--sym", str(sym), "--top", "0", "--budget", f"{symbol}={ceiling}"]
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            if f"budget: {symbol} never ran in a completed frame" in proc.stdout:
                sys.exit(f"nmi-budget: {symbol} never ran in the measured frames of {key} — "
                         f"a row must name an example (and a manifest) that exercises its symbol")
            if proc.returncode == 2:
                sys.exit(f"nmi-budget: luna rejected the symbol on {key}: "
                         f"{proc.stdout.strip()[-200:]}")
            m = BUDGET_RE.search(proc.stdout)
            if not m:
                sys.exit(f"nmi-budget: luna printed no budget line for {key}: "
                         f"{(proc.stdout + proc.stderr).strip()[-200:]}")
            worst, frame = int(m.group(2)), m.group(3)
            grew = worst > ref * (1 + DRIFT)
            verdict = "OVER" if proc.returncode == 1 else ("GREW" if grew else "ok")
            if proc.returncode == 1 or (grew and not args.report):
                over += 1
            share = (f"{100.0 * worst / limit:4.0f}% of {limit}" if limit
                     else "no ceiling, growth only")
            name = key if symbol == "NmiHandler" else f"{key} [{symbol}]"
            print(f"  {verdict:4}  {name:24} {worst:6} mclk  ({share}, "
                  f"reference {ref}, worst frame {frame})  — {why}")
    total = len(SUBSET) + len(EXTRA)
    print(f"\nNMI VBlank budget: {total - over}/{total} within "
          f"{CEILING} master cycles and {int(DRIFT * 100)} % of their reference"
          + (f", {over} OVER or GREW" if over else ""))
    return 1 if over else 0


if __name__ == "__main__":
    sys.exit(main())
