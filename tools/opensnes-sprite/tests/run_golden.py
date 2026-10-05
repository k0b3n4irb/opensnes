#!/usr/bin/env python3
"""Golden tests for opensnes-sprite (sheets and Aseprite exports; the 1.x successor of gfx4snes -s/-T and aseprite2snes).

The family's contract: a tool that absorbs a 0.x tool reproduces that
tool's golden suite byte for byte. The `sheet` cases compare against
gfx4snes's goldens (`tools/gfx4snes/tests/golden`: spr, flip, two, three),
the `anim` case against aseprite2snes's hero_anim.h from its second line
(the first names the generator). Fixtures are copies of theirs.

Run:  python3 tools/opensnes-sprite/tests/run_golden.py
"""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-sprite", __file__)
GFX = g.here.parents[1] / "gfx4snes" / "tests" / "golden"
ASE = g.here.parents[1] / "aseprite2snes" / "tests" / "golden"
META = ["--colors", "16", "--metasprite", "32", "16", "--priority", "2"]

g.expect_outputs("sheet spr.png --size 16 == gfx4snes -s 16 -p", ["sheet", "-q", "--size", "16", "spr.png"],
                 copy=["spr.png"], outputs=["spr.pic", "spr.pal", "spr.inc", "spr_data.as"], want_dir=GFX)
g.expect_outputs("sheet flip.png --metasprite 32 16 --flip == gfx4snes -T -F", ["sheet", "-q", "--size", "16", *META, "--flip", "flip.png"],
                 copy=["flip.png"], outputs=["flip.pic", "flip.pal", "flip.inc", "flip_data.as", "flip_meta.inc"], want_dir=GFX)
g.expect_outputs("sheet two.png (char names, not block indices) == gfx4snes", ["sheet", "-q", "--size", "16", *META, "two.png"],
                 copy=["two.png"], outputs=["two_meta.inc"], want_dir=GFX)
g.expect_outputs("sheet three.png --flip (mirror drift) == gfx4snes", ["sheet", "-q", "--size", "16", "--colors", "16", "--metasprite", "48", "16", "--priority", "2", "--flip", "three.png"],
                 copy=["three.png"], outputs=["three_meta.inc"], want_dir=GFX)


def anim_matches():
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["anim", "-q", "--prefix", "hero", "hero.json"], work, copy=["hero.json"])
        if g.failure(proc):
            return [g.failure(proc)]
        got = (work / "hero_anim.h").read_text().splitlines()[1:]
        want = (ASE / "hero_anim.h").read_text().splitlines()[1:]
        if got != want:
            return ["hero_anim.h differs from aseprite2snes's golden after the generator line"]
        first = (work / "hero_anim.h").read_text().splitlines()[0]
        if "opensnes-sprite" not in first:
            return [f"generator line: {first!r}"]
    return []


g.check("anim hero.json == aseprite2snes hero_anim.h from line 2", "same clips, own generator line", anim_matches)
g.expect_stdout("sheet --json", ["sheet", "--json", "--size", "16", *META, "--flip", "flip.png"], golden="sheet.json", copy=["flip.png"])
g.expect_stdout("inspect --json", ["inspect", "--json", "--size", "16", "spr.png", "flip.png"], golden="inspect.json", copy=["spr.png", "flip.png"])
g.expect_outputs("sheet --save writes spr.png.toml", ["sheet", "-q", "--size", "16", "--colors", "16", "--save", "spr.png"],
                 copy=["spr.png"], outputs=["spr.png.toml"])
g.expect_refused("sheet index5_2bpp.png --bpp 2 (two palette banks in one tile)", ["sheet", "--size", "8", "--bpp", "2", "index5_2bpp.png"],
                 copy=["index5_2bpp.png"], needles=["one 4-colour palette per tile"])
g.expect_refused("sheet --size 12 (not an OBJ size)", ["sheet", "--size", "12", "spr.png"], copy=["spr.png"], needles=["8, 16, 32 and 64"])
g.expect_refused("sheet --metasprite 20 16 (not a multiple of the block)", ["sheet", "--size", "16", "--metasprite", "20", "16", "spr.png"],
                 copy=["spr.png"], needles=["multiples of the block size"], nothing_written=["spr_meta.inc"])
g.expect_refused("anim bad_range.json", ["anim", "bad_range.json"], copy=["bad_range.json"], needles=["out of bounds"], nothing_written=["bad_range_anim.h"])
g.expect_refused("sheet hero.json (not a sheet)", ["sheet", "hero.json"], copy=["hero.json"], needles=["not a .png or .bmp sheet"])
sys.exit(g.report())
