#!/usr/bin/env python3
"""Golden-output tests for palplan (project shared-palette planner).

The committed manifest of BGR555 .pal fixtures → the generated C header and
combined CGRAM image, byte-compared. palplan is deterministic (slots are
assigned in manifest order). A second case asserts the over-subscription
hard-fail: nine distinct sprite palettes must exit non-zero (eight slots).

Fixture provenance: fixtures/*.pal are synthesized BGR555 palettes
generated deterministically in this repo (git history of this file,
tools/palplan/README.md) — not third-party assets. The manifest wires in
an identical pair (boss==hero), a two-colour-apart pair (hero~enemy), a
transparent-index-0-only pair (hero~ghost) and a 4-colour palette (hud).
The tool runs where the manifest is, so the path embedded in the header
("project.txt") is stable across machines.

Run:  python3 tools/palplan/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("palplan", __file__)
g.expect_outputs("project.txt -> project.h + project.pal",
                 ["-o", "project.h", "-b", "project.pal", "project.txt"], copy="*",
                 outputs=["project.h", "project.pal"])
g.expect_refused("over.txt (9 sprite palettes, 8 slots)", ["over.txt"], copy="*")
sys.exit(g.report())
