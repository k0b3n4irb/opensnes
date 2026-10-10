#!/usr/bin/env python3
"""Golden-output tests for aseprite2snes (Aseprite export -> anim.h clips).

The committed Aseprite JSON fixture → the generated C header on stdout,
compared with the committed golden. Frames and tags are emitted in export
order, so a diff is a behaviour change. A second case asserts the range
hard-fail: a tag whose frame range runs past the frame count exits non-zero.

Fixture provenance: fixtures/*.json are hand-authored exports in the exact
shape Aseprite writes with `--data --list-tags` — not third-party assets.
hero.json exercises a non-uniform-duration clip (walk), two uniform clips
(idle, hurt) and a one-shot pingpong (hurt, repeat=1). The tool runs where
the export is, so the "Source export:" path in the header is stable.

Run:  python3 tools/aseprite2snes/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("aseprite2snes", __file__)
g.expect_stdout("hero.json -> hero_anim.h", ["-p", "hero", "hero.json"], golden="hero_anim.h", copy="*")
g.expect_refused("bad_range.json (tag past the frame count)", ["-p", "bad", "bad_range.json"], copy="*")
sys.exit(g.report())
