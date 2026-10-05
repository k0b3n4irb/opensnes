#!/usr/bin/env python3
"""Golden tests for opensnes-palette (the CGRAM plan and the quantization of RGB art;
the 1.x successor of palplan and img2snes).

The family's contract: reproduce the absorbed tools' golden suites byte for
byte. The plan of palplan's manifest (project.toml lists the same .pal
files in the same order) must give palplan's 512-byte CGRAM image and its
PAL_<NAME>_CGRAM / _SLOT / _COLORS lines; the quantization of img2snes's
rgb.png must give its two indexed PNGs. The fixtures are copies of theirs.

Run:  python3 tools/opensnes-palette/tests/run_golden.py
"""
from __future__ import annotations

import filecmp
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-palette", __file__)
PALPLAN = g.here.parents[1] / "palplan" / "tests" / "golden"
IMG = g.here.parents[1] / "img2snes" / "tests" / "golden"

g.expect_outputs("plan project.toml (cgram = true): the CGRAM image == palplan -b", ["plan", "-q", "project.toml"], copy="*",
                 outputs=["project.pal"], want_dir=PALPLAN)
g.expect_outputs("plan project.toml: the glue (.inc, _data.as)", ["plan", "-q", "project.toml"], copy="*",
                 outputs=["project.inc", "project_data.as"])


def macros_match():
    """Every #define PAL_ line of the .inc is palplan -o's, in the same order."""
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["plan", "-q", "project.toml"], work, copy="*")
        if g.failure(proc):
            return [g.failure(proc)]
        got = [l for l in (work / "project.inc").read_text().splitlines() if l.startswith("#define PAL_")]
    want = [l for l in (PALPLAN / "project.h").read_text().splitlines() if l.startswith("#define PAL_")]
    return [] if got == want else [f"{len(got)} PAL_ macros differ from palplan's {len(want)}"]


g.check("plan project.toml: the PAL_ macros == palplan -o", "macros match", macros_match)
g.expect_stdout("plan project.toml (the table and the hints)", ["plan", "project.toml"], golden="plan.txt", copy="*")
g.expect_stdout("plan --json", ["plan", "--json", "project.toml"], golden="plan.json", copy="*")
g.expect_outputs("plan --bg --sprite --save writes the settings file", ["plan", "-q", "--bg", "bg1.pal", "--sprite", "hero.pal enemy.pal", "--save", "mine.toml"],
                 copy="*", outputs=["mine.toml", "mine.inc"])
g.expect_refused("plan over.toml (9 distinct sprite palettes)", ["plan", "over.toml"], copy="*", needles=["8 slots"], nothing_written=["over.inc"])
g.expect_refused("plan of a 256-colour .pal", ["plan", "--bg", "big.pal", "big.toml"], copy="*", write={"big.pal": bytes(512)},
                 needles=["whole CGRAM"], nothing_written=["big.inc"])
g.expect_refused("plan without a settings file", ["plan", "hero.pal"], copy="*", needles=["one settings file"])


def quantize_matches(flags, golden_name):
    def run():
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            proc = g.run(["quantize", "-q", *flags, "rgb.png"], work, copy=["rgb.png"])
            if g.failure(proc):
                return [g.failure(proc)]
            got = work / "rgb_indexed.png"
            if not got.is_file():
                return ["rgb_indexed.png: not produced"]
            if not filecmp.cmp(got, IMG / golden_name, shallow=False):
                return [f"rgb_indexed.png differs from img2snes's {golden_name}"]
        return []
    return run


g.check("quantize rgb.png == img2snes -c 16", "same bytes", quantize_matches([], "rgb_16.png"))
g.check("quantize --colors 4 --round == img2snes -c 4 --round-snes", "same bytes", quantize_matches(["--colors", "4", "--round"], "rgb_4_snes.png"))
g.expect_stdout("quantize --json --palette hero.pal", ["quantize", "--json", "--palette", "hero.pal", "rgb.png"], golden="quantize.json",
                copy=["rgb.png", "hero.pal"])
g.expect_refused("quantize --colors 1", ["quantize", "--colors", "1", "rgb.png"], copy=["rgb.png"], needles=["2 to 256"],
                 nothing_written=["rgb_indexed.png"])
g.expect_stdout("inspect --json", ["inspect", "--json", "hud.pal", "hero.pal"], golden="inspect.json", copy=["hud.pal", "hero.pal"])
sys.exit(g.report())
